"""Encode generated PNG sources as GPU-ready DDS. Requires numpy and Pillow.

BC1 sRGB albedo, BC5 linear normals, BC1 linear AO/roughness/height (1024),
BC5 micro normal (512). Full mip chains; no runtime image conversion.
The small vectorized reference encoder favours reproducibility over BC7 quality.
"""
from pathlib import Path
import struct

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

ROOT = Path(__file__).resolve().parents[1] / "Assets/Textures/Terrain"


def blocks(image):
    h, w, c = image.shape
    image = np.pad(image, ((0, (-h) % 4), (0, (-w) % 4), (0, 0)), mode="edge")
    return image.reshape((h + 3) // 4, 4, (w + 3) // 4, 4, c).transpose(0, 2, 1, 3, 4).reshape(-1, 16, c)


def pack565(rgb):
    q = np.round(rgb * np.array([31, 63, 31]) / 255).astype(np.uint16)
    return (q[:, 0] << 11) | (q[:, 1] << 5) | q[:, 2]


def unpack565(value):
    return np.stack(((value >> 11) * 255 / 31, ((value >> 5) & 63) * 255 / 63,
                     (value & 31) * 255 / 31), axis=-1)


def bc1(image):
    b = blocks(image).astype(np.float32)
    c0, c1 = pack565(b.max(axis=1)), pack565(b.min(axis=1))
    equal = c0 == c1
    c1 = np.where(equal & (c0 == 65535), c1 - 1, c1).astype(np.uint16)
    c0 = np.where(equal & (c0 < 65535), c0 + 1, c0).astype(np.uint16)
    hi, lo = unpack565(c0), unpack565(c1)
    palette = np.stack((hi, lo, (hi * 2 + lo) / 3, (hi + lo * 2) / 3), axis=1)
    codes = ((b[:, :, None, :] - palette[:, None, :, :]) ** 2).sum(axis=-1).argmin(axis=-1)
    bits = (codes.astype(np.uint32) << (np.arange(16, dtype=np.uint32) * 2)).sum(axis=1, dtype=np.uint32)
    output = np.empty(len(b), dtype=[("a", "<u2"), ("b", "<u2"), ("i", "<u4")])
    output["a"], output["b"], output["i"] = c0, c1, bits
    return output.tobytes()


def bc4(channel):
    hi, lo = channel.max(axis=1), channel.min(axis=1)
    same = hi == lo
    lo = np.where(same & (hi == 255), lo - 1, lo)
    hi = np.where(same & (hi < 255), hi + 1, hi)
    hi, lo = hi.astype(np.float32), lo.astype(np.float32)
    palette = np.stack([hi, lo] + [((7 - i) * hi + i * lo) / 7 for i in range(1, 7)], axis=1)
    codes = np.abs(channel[:, :, None] - palette[:, None, :]).argmin(axis=-1).astype(np.uint64)
    bits = (codes << (np.arange(16, dtype=np.uint64) * 3)).sum(axis=1, dtype=np.uint64)
    result = np.empty((len(channel), 8), dtype=np.uint8)
    result[:, 0], result[:, 1] = hi, lo
    for i in range(6):
        result[:, i + 2] = (bits >> (i * 8)) & 255
    return result


def bc5(image):
    b = blocks(image)
    return np.concatenate((bc4(b[:, :, 0]), bc4(b[:, :, 1])), axis=1).tobytes()


def write_dds(path, mips, normal=False, srgb=True):
    size = mips[0].shape[0]
    block_size = 16 if normal else 8
    header = [124, 0x000A1007, size, size, ((size + 3) // 4) ** 2 * block_size, 0, len(mips)]
    header += [0] * 11
    header += [32, 4, int.from_bytes(b"DX10", "little"), 0, 0, 0, 0, 0]
    header += [0x00401008, 0, 0, 0, 0]
    dx10 = [83 if normal else (72 if srgb else 71), 3, 0, 1, 0]
    encode = bc5 if normal else bc1
    data = b"DDS " + struct.pack("<31I", *header) + struct.pack("<5I", *dx10)
    data += b"".join(encode(level) for level in mips)
    path.write_bytes(data)
    print(f"{path.name}: {size}x{size}, {len(mips)} mips, {len(data):,} bytes")


def downsample(data):
    h, w, c = data.shape
    return data.reshape(h // 2, 2, w // 2, 2, c).mean(axis=(1, 3))


def linear_to_srgb(data):
    return np.where(data <= 0.0031308, data * 12.92, 1.055 * np.maximum(data, 0) ** (1 / 2.4) - 0.055)


def data_mips(data, normal=False):
    result = []
    while True:
        if normal:
            data = data / np.maximum(np.linalg.norm(data, axis=-1, keepdims=True), 1e-8)
        encoded = data * 0.5 + 0.5 if normal else data
        result.append(np.uint8(np.clip(np.round(encoded * 255), 0, 255)))
        if len(data) == 1:
            return result
        data = downsample(data)


def normals_from_height(height, strength):
    padded = np.pad(height, 1, mode="symmetric")
    dx = (padded[1:-1, 2:] - padded[1:-1, :-2]) * strength
    dy = (padded[2:, 1:-1] - padded[:-2, 1:-1]) * strength
    return np.stack((-dx, -dy, np.ones_like(height)), axis=-1)


def main():
    for name in ("dust", "rock", "gravel"):
        image = Image.open(ROOT / "Source" / f"lunar_{name}.png").convert("RGB").resize((1024, 1024), Image.Resampling.LANCZOS)
        image.save(ROOT / f"lunar_{name}_basecolor.png")
        rgb = np.asarray(image).astype(np.float32) / 255
        linear = np.where(rgb <= 0.04045, rgb / 12.92, ((rgb + 0.055) / 1.055) ** 2.4)
        mips = []
        while True:
            mips.append(np.uint8(np.clip(np.round(linear_to_srgb(linear) * 255), 0, 255)))
            if len(linear) == 1:
                break
            linear = downsample(linear)
        write_dds(ROOT / f"lunar_{name}.dds", mips)

        height_image = Image.open(ROOT / "Source" / f"lunar_{name}_height.png").convert("L").resize((1024, 1024), Image.Resampling.LANCZOS)
        height = np.asarray(height_image).astype(np.float32) / 255
        strength = {"dust": 1.6, "rock": 4.0, "gravel": 3.0}[name]
        normal_mips = data_mips(normals_from_height(height, strength), normal=True)
        Image.fromarray(normal_mips[0]).save(ROOT / f"lunar_{name}_normal.png")
        write_dds(ROOT / f"lunar_{name}_normal.dds", normal_mips, normal=True)
        # Artistic material estimates: local depressions darken AO, exposed peaks
        # are slightly smoother. These are not physically measured PBR maps.
        local_mean = np.asarray(height_image.filter(ImageFilter.GaussianBlur(5))).astype(np.float32) / 255
        ao = np.clip(1 - np.maximum(local_mean - height, 0) * 1.5, 0.45, 1)
        roughness = np.clip({"dust": 0.97, "rock": 0.86, "gravel": 0.92}[name] - (height - 0.5) * 0.15, 0.65, 1)
        arh = np.stack((ao, roughness, height), axis=-1)
        for suffix, channel in (("height", height), ("ao", ao), ("roughness", roughness)):
            Image.fromarray(np.uint8(np.round(channel * 255))).save(ROOT / f"lunar_{name}_{suffix}.png")
        Image.fromarray(np.uint8(np.round(arh * 255))).save(ROOT / f"lunar_{name}_arh.png")
        write_dds(ROOT / f"lunar_{name}_arh.dds", data_mips(arh), srgb=False)

    height = np.asarray(Image.open(ROOT / "Source/lunar_detail_height.png").convert("L")
                        .resize((512, 512), Image.Resampling.LANCZOS)).astype(np.float32) / 255
    mips = data_mips(normals_from_height(height, 2.5), normal=True)
    Image.fromarray(mips[0]).save(ROOT / "lunar_detail_normal.png")
    write_dds(ROOT / "lunar_detail_normal.dds", mips, normal=True)

    uv = Image.new("RGB", (1024, 1024))
    draw = ImageDraw.Draw(uv)
    for y in range(8):
        for x in range(8):
            color = (65 + 20 * x, 65 + 20 * y, 185 if (x + y) % 2 else 85)
            draw.rectangle((x * 128, y * 128, (x + 1) * 128 - 1, (y + 1) * 128 - 1), fill=color, outline="white", width=2)
            draw.text((x * 128 + 8, y * 128 + 8), f"U{x} V{y}", fill="white", font_size=22)
    uv.save(ROOT / "lunar_uv_checker.png")
    # Neutral values are shader constants at runtime, avoiding six unused samples.
    for name in ("dust", "rock", "gravel"):
        Image.new("L", (4, 4), 0).save(ROOT / f"lunar_{name}_metallic.png")
        Image.new("RGB", (4, 4), (0, 0, 0)).save(ROOT / f"lunar_{name}_emissive.png")
        zero_mips = data_mips(np.zeros((4, 4, 3), dtype=np.float32))
        write_dds(ROOT / f"lunar_{name}_metallic.dds", zero_mips, srgb=False)
        write_dds(ROOT / f"lunar_{name}_emissive.dds", zero_mips)

    # Contact sheet is a diagnostic view of the generated asset set.
    sheet = Image.new("RGB", (5 * 224, 3 * 250), (20, 25, 34))
    draw = ImageDraw.Draw(sheet)
    for row, name in enumerate(("dust", "rock", "gravel")):
        for col, kind in enumerate(("albedo", "normal", "height", "roughness", "ao")):
            path = ROOT / "Source" / f"lunar_{name}.png" if kind == "albedo" else ROOT / f"lunar_{name}_{kind}.png"
            sheet.paste(Image.open(path).convert("RGB").resize((216, 216)), (col * 224 + 4, row * 250 + 28))
            draw.text((col * 224 + 4, row * 250 + 6), f"{name} / {kind}", fill="white", font_size=16)
    sheet.save(ROOT / "lunar_materials_preview.png")


if __name__ == "__main__":
    main()
