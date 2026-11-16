"""Bake authored lunar terrain and surface regions. Requires numpy.

Run from any directory: python Tools/bake_lunar_heightmap.py
RAW = (world_height - height_min_m) / height_range_m * 65535.
The runtime reads only RAW; JSON and PNG are authoring/inspection assets.
"""
import json
import struct
import zlib
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1]


def write_png(path, pixels):
    """Small dependency-free PNG writer, also usable by Blender's bundled Python."""
    depth = 16 if pixels.dtype.itemsize == 2 else 8
    color = 0 if pixels.ndim == 2 else 6 if pixels.shape[2] == 4 else 2
    encoded = pixels.astype(">u2").tobytes() if depth == 16 else pixels.tobytes()
    stride = len(encoded)//pixels.shape[0]
    scanlines = b"".join(b"\0"+encoded[i:i+stride] for i in range(0, len(encoded), stride))
    def chunk(kind, data):
        return struct.pack(">I", len(data))+kind+data+struct.pack(">I", zlib.crc32(kind+data))
    path.write_bytes(b"\x89PNG\r\n\x1a\n"+chunk(b"IHDR", struct.pack(">IIBBBBB", pixels.shape[1], pixels.shape[0], depth, color, 0, 0, 0))
                     +chunk(b"IDAT", zlib.compress(scanlines))+chunk(b"IEND", b""))


def smooth(a, b, value):
    t = np.clip((value - a) / (b - a), 0, 1)
    return t * t * (3 - 2 * t)


def noise(x, z):
    ix, iz = np.floor(x), np.floor(z)
    fx, fz = smooth(0, 1, x - ix), smooth(0, 1, z - iz)

    def hashed(a, b):
        value = np.sin(a * 127.1 + b * 311.7) * 43758.5453
        return value - np.floor(value)

    top = hashed(ix, iz) * (1 - fx) + hashed(ix + 1, iz) * fx
    bottom = hashed(ix, iz + 1) * (1 - fx) + hashed(ix + 1, iz + 1) * fx
    return top * (1 - fz) + bottom * fz


def road_field(x, z, roads):
    """Distance to the authored centerlines, including rounded junctions/endcaps."""
    weight = np.zeros_like(x)
    for road in roads:
        distance = np.full_like(x, np.inf)
        for a, b in zip(road["pointsM"], road["pointsM"][1:]):
            vx, vz = b[0] - a[0], b[1] - a[1]
            length2 = vx * vx + vz * vz
            if length2 < 1e-8:
                continue
            t = np.clip(((x-a[0])*vx + (z-a[1])*vz) / length2, 0, 1)
            distance = np.minimum(distance, np.hypot(x-a[0]-t*vx, z-a[1]-t*vz))
        weight = np.maximum(weight, 1-smooth(road["widthM"]*.5,
                                           road["widthM"]*.5+road["blendM"], distance))
    return weight


def pad_field(x, z, site):
    cx, cz = site["centerXZ"]
    hx, hz = site["halfSizeM"]
    distance = np.hypot(np.maximum(abs(x-cx)-hx, 0), np.maximum(abs(z-cz)-hz, 0))
    return 1-smooth(0, site["blendM"], distance)


def surface_mask(config, size=1024):
    """Linear channels: R worn road, G gravel region, B exposed rock region."""
    extent = (config["width"]-1)*config["cell_spacing_m"]*.5
    z, x = np.meshgrid(np.linspace(-extent, extent, size), np.linspace(-extent, extent, size), indexing="ij")
    roads = config.get("roads", [])
    # A fine broken edge gives the road an abandoned appearance; the level bed remains wider.
    wear = .65*(noise(x/2.3+17, z/2.3)-.5)
    road = road_field(x+wear, z+wear, [dict(r, blendM=1.8) for r in roads])
    road *= .5+.5*smooth(.28, .68, noise(x/9-23, z/9+11))  # drifting dust over the abandoned road
    gravel, rock = np.zeros_like(x), np.zeros_like(x)
    for zone in config.get("surface_zones", []):
        cx, cz = zone["centerXZ"]; rx, rz = zone["radiusM"]
        radius = np.hypot((x-cx)/rx, (z-cz)/rz)
        region = 1-smooth(.68, 1.1, radius + .06*(noise(x/8, z/8)-.5))
        if zone["material"] == "gravel": gravel = np.maximum(gravel, region)
        else: rock = np.maximum(rock, region)
    for site in config.get("sites", []):
        gravel = np.maximum(gravel, pad_field(x, z, site)*.72)
    return np.uint8(np.clip(np.stack((road, gravel, rock, np.ones_like(x)), -1)*255, 0, 255))


def write_surface_dds(path, rgba):
    """Uncompressed linear RGBA8 with a full mip chain (no lossy mask compression)."""
    levels = [rgba]
    while levels[-1].shape[0] > 1:
        a = levels[-1].astype(np.float32); n = a.shape[0]//2
        levels.append(np.round(a.reshape(n, 2, n, 2, 4).mean(axis=(1, 3))).astype(np.uint8))
    size = rgba.shape[0]
    header = [124, 0x0002100F, size, size, size*4, 0, len(levels)] + [0]*11
    header += [32, 4, int.from_bytes(b"DX10", "little"), 0, 0, 0, 0, 0]
    header += [0x00401008, 0, 0, 0, 0]
    path.write_bytes(b"DDS " + struct.pack("<31I", *header) + struct.pack("<5I", 28, 3, 0, 1, 0)
                     + b"".join(level.tobytes() for level in levels))


def bake(config):
    z, x = np.mgrid[:config["length"], :config["width"]].astype(np.float64)
    x = (x - (config["width"] - 1) * 0.5) * config["cell_spacing_m"]
    z = (z - (config["length"] - 1) * 0.5) * config["cell_spacing_m"]
    h = config["landing_height_m"] + 2.8 * (noise(x / 53, z / 53) - 0.5)
    h += 1.0 * (noise(x / 17 + 31, z / 17 - 12) - 0.5)
    h += 0.22 * (noise(x / 3 + 9, z / 3 + 4) - 0.5)
    for hill in config.get("mountains", []):
        cx, cz = hill["centerXZ"]; rx, rz = hill["radiusM"]
        radius = np.hypot((x-cx)/rx, (z-cz)/rz)
        angle = np.arctan2(z-cz,x-cx); phase = (cx+cz)*.019
        radius *= 1+.12*np.sin(angle*3+phase)+.07*np.sin(angle*7-phase)
        # Rounded asymmetric ridges; radial detail fades at the summit to avoid needle peaks.
        ridge = (1-smooth(0,1,radius))**1.3
        h += hill["heightM"] * ridge * (.96+.06*noise(x/9+phase,z/9))
    for cx, cz, radius, depth, rim_height, phase in config["craters"]:
        angle = np.arctan2(z - cz, x - cx)
        irregularity = 1 + 0.045 * np.sin(angle * 5 + phase) + 0.024 * np.sin(angle * 11 - phase)
        r = np.hypot(x - cx, z - cz) / (radius * irregularity)
        bowl = 1 - smooth(0.48, 0.97, r)
        rim = np.exp(-((r - 1) / 0.14) ** 2)
        ejecta = np.exp(-((r - 1.18) / 0.28) ** 2)
        fluting = 0.75 + 0.25 * np.sin(angle * 23 + phase)
        h += -depth * bowl + rim_height * rim + rim_height * 0.16 * ejecta * fluting
        h += 0.16 * np.sin(angle * 31 + r * 12) * smooth(0.55, 0.8, r) * (1 - smooth(1.05, 1.4, r))
    landing = smooth(config["landing_radius_m"], config["landing_radius_m"] + 12, np.hypot(x, z))
    h = config["landing_height_m"] + (h - config["landing_height_m"]) * landing
    # Transport beds and building foundations take priority over geological detail.
    for site in config.get("sites", []):
        weight = pad_field(x, z, site)
        h = h*(1-weight)+site["heightM"]*weight
    roads = config.get("roads", [])
    if roads:
        weight = road_field(x, z, roads)
        h = h*(1-weight)+config["landing_height_m"]*weight
    encoded = (h - config["height_min_m"]) / config["height_range_m"]
    if encoded.min() < 0 or encoded.max() > 1:
        raise ValueError("Height range clips the terrain; update JSON and TestMapSettings.h together")
    return h, np.round(encoded * 65535).astype("<u2")


def write_outputs(config, root=ROOT):
    directory = root / "Assets/Terrain"
    heights, raw = bake(config)
    (directory / "lunar_craters_r16.raw").write_bytes(raw.tobytes())
    write_png(directory / "lunar_height_preview.png", raw)
    dz, dx = np.gradient(heights, config["cell_spacing_m"])
    normal = np.stack((-dx, np.ones_like(dx), -dz), -1)
    normal /= np.linalg.norm(normal, axis=-1, keepdims=True)
    light = np.array([-0.45, 0.78, -0.43])
    shade = np.clip(normal @ light, 0, 1) * 0.7 + 0.22
    color = shade[..., None] * np.array([170, 188, 215])
    write_png(directory / "lunar_layout_preview.png", np.uint8(np.clip(color, 0, 255)))
    if config.get("roads"):
        mask = surface_mask(config)
        # PNG/Blender UV v=0 addresses the bottom row; DirectX DDS v=0 the first row.
        write_png(directory / "lunar_surface_mask.png", mask[::-1])
        write_surface_dds(directory / "lunar_surface_mask.dds", mask)
    print(f"Baked {raw.shape[1]}x{raw.shape[0]} R16 RAW: {heights.min():.2f} .. {heights.max():.2f} m")
    # Blender reference uses the quantized runtime heights, not the pre-encoding floats.
    return raw.astype(np.float64)/65535*config["height_range_m"]+config["height_min_m"]


def main():
    write_outputs(json.loads((ROOT / "Assets/Terrain/lunar_craters.json").read_text()))


if __name__ == "__main__":
    main()
