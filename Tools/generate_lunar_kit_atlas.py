"""Create original, deterministic base-color art for L02 (Python + Pillow).

Run before build_lunar_outpost.py. No downloaded art or fonts are required.
The engine currently samples one DDS texture; every mesh uses this atlas.
"""

from pathlib import Path
import argparse
import random

from PIL import Image, ImageDraw, ImageFont
import numpy as np
from bake_lunar_textures import write_dds as write_bc_dds, data_mips, normals_from_height


TILE = 512
GRID = 4
PALETTE = [
    (202, 209, 207), (48, 58, 62), (100, 113, 116), (200, 150, 38),
    (180, 76, 36), (72, 188, 185), (27, 34, 39), (109, 105, 97),
    (31, 57, 75), (40, 51, 54), (178, 170, 145), (34, 44, 47),
    (38, 53, 55), (100, 110, 109), (200, 132, 41), (140, 150, 148),
]


def font(size):
    # Pillow's bundled font keeps the asset reproducible on other machines.
    return ImageFont.load_default(size=size)


def centered(draw, xy, value, size, fill):
    draw.text(xy, value, font=font(size), fill=fill, anchor="mm", stroke_width=0)


def make_atlas(output):
    output.mkdir(parents=True, exist_ok=True)
    atlas = Image.new("RGB", (TILE * GRID, TILE * GRID))
    rng = random.Random(31012)
    for index, color in enumerate(PALETTE):
        tile = Image.new("RGB", (TILE, TILE), color)
        pixels = tile.load()
        for y in range(TILE):
            dust = 13 * max(0, (y / TILE - .48) / .52)
            for x in range(TILE):
                grain = rng.randint(-5, 5)
                pixels[x, y] = tuple(max(0, min(255, int(c + grain - dust))) for c in color)
        d = ImageDraw.Draw(tile)
        if index in (0, 1, 2, 4, 7, 10, 13, 15):
            d.rounded_rectangle((12, 12, 499, 499), radius=14, outline=(32, 42, 45), width=6)
            d.line((24, 34, 482, 34), fill=tuple(min(255, c + 26) for c in color), width=3)
            d.line((24, 474, 482, 474), fill=(58, 64, 60), width=5)
            for x in (29, 482):
                for y in (29, 482):
                    d.ellipse((x-5, y-5, x+5, y+5), fill=(35, 43, 45), outline=(126, 136, 135), width=2)
            for _ in range(65):
                x, y = rng.randint(20, 490), rng.randint(20, 490)
                d.line((x, y, x + rng.randint(3, 24), y-rng.randint(0, 3)), fill=(96, 103, 100), width=1)
        if index == 0:
            d.line((34, 400, 478, 400), fill=(168, 126, 38), width=24)
            d.line((40, 95, 40, 330), fill=(69, 87, 89), width=10)
            d.text((62, 58), "NEREIA / LOGISTICS", font=font(18), fill=(53, 69, 71))
            d.text((312, 438), "L02 / 2036", font=font(15), fill=(60, 74, 75))
        elif index == 1:
            for y in range(66, 460, 45):
                d.rectangle((48, y, 464, y+9), fill=(25, 34, 37))
                d.line((48, y+10, 464, y+10), fill=(76, 86, 88), width=2)
        elif index == 3:
            for x in range(-TILE, TILE * 2, 110):
                d.polygon(((x, 0), (x+54, 0), (x+TILE+54, TILE), (x+TILE, TILE)), fill=(30, 38, 39))
            d.rectangle((0, 0, 511, 511), outline=(104, 91, 52), width=12)
        elif index == 5:
            d.rectangle((0, 0, 511, 511), fill=(29, 76, 80))
            d.rounded_rectangle((30, 60, 482, 452), radius=35, fill=(68, 180, 181))
            d.line((52, 112, 460, 112), fill=(151, 224, 205), width=12)
        elif index == 8:
            for x in range(8, TILE, 63):
                for y in range(8, TILE, 84):
                    d.rectangle((x, y, x+52, y+72), fill=(28, 48, 62), outline=(80, 100, 105), width=2)
                    d.line((x+26, y, x+26, y+72), fill=(94, 116, 123), width=2)
        elif index == 9:
            d.rectangle((16, 16, 496, 496), outline=(194, 152, 54), width=13)
            centered(d, (256, 105), "LUNAR", 61, (181, 190, 182))
            centered(d, (256, 206), "CARGO", 76, (211, 215, 197))
            centered(d, (256, 342), "03", 148, (208, 159, 49))
            centered(d, (256, 456), "CARGO / SERVICE", 23, (151, 169, 163))
        elif index == 10:
            d.polygon(((65, 112), (102, 75), (448, 421), (411, 458)), fill=(41, 55, 57))
        elif index == 11:
            d.rectangle((32,32,480,480),outline=(89,186,182),width=10)
            d.rectangle((224,58,288,222),fill=(165,222,210))
            d.rectangle((174,108,338,172),fill=(165,222,210))
            centered(d,(256,300),"DECON",76,(212,225,218))
            centered(d,(256,405),"02 / PURIFICATION",29,(113,190,180))
        elif index == 12:
            d.rectangle((20, 20, 492, 492), outline=(96, 181, 170), width=8)
            centered(d, (256, 96), "AUX / LINK", 40, (123, 204, 188))
            for y, value in ((178, "SIGNAL LOST"), (248, "O2  --  --"), (318, "GRID / OFFLINE")):
                d.text((54, y), value, font=font(27), fill=(80, 153, 145))
            for i in range(14):
                d.rectangle((54+i*28, 404, 72+i*28, 448), fill=(51, 98, 96))
        elif index == 14:
            centered(d,(256,115),"RELAY",89,(34,42,40))
            centered(d,(256,280),"04",200,(34,42,40))
            centered(d,(256,445),"UPLINK / EXTRACTION",26,(37,46,43))
        # Copy edge texels into an 8 px gutter for filtered atlas sampling.
        tile.paste(tile.crop((8, 8, 504, 504)).resize((512, 512)), (0, 0))
        atlas.paste(tile, ((index % GRID) * TILE, (index // GRID) * TILE))
    atlas.save(output / "L02_BaseColor.png")
    # Existing terrain encoder provides native BC1 sRGB + complete linear-light mips.
    rgb=np.asarray(atlas).astype(np.float32)/255
    linear=np.where(rgb<=.04045,rgb/12.92,((rgb+.055)/1.055)**2.4)
    levels=[]
    from bake_lunar_textures import downsample, linear_to_srgb
    while True:
        levels.append(np.uint8(np.clip(np.round(linear_to_srgb(linear)*255),0,255)))
        if len(linear)==1: break
        linear=downsample(linear)
    write_bc_dds(output/"L02_BaseColor.dds",levels,srgb=True)
    small=atlas.resize((1024,1024),Image.Resampling.LANCZOS)
    colors=np.asarray(small).astype(np.float32)/255
    rough=np.zeros((1024,1024),dtype=np.float32)
    metal=rough.copy(); emission=np.zeros((1024,1024,3),dtype=np.float32)
    for i in range(16):
        ys=slice((i//4)*256,(i//4+1)*256); xs=slice((i%4)*256,(i%4+1)*256)
        rough[ys,xs]=.48 if i in (2,5,8,12) else .77
        metal[ys,xs]=.8 if i in (1,2,6,13) else .08
        if i==5: emission[ys,xs]=colors[ys,xs]*.6
    normal=normals_from_height(colors.mean(axis=-1),.35)
    normal/=np.maximum(np.linalg.norm(normal,axis=-1,keepdims=True),1e-8)
    Image.fromarray(np.uint8((normal*.5+.5)*255)).save(output/"L02_NormalGL.png")
    normal_dx=normal.copy(); normal_dx[:,:,1]*=-1
    write_bc_dds(output/"L02_NormalDX.dds",data_mips(normal_dx,normal=True),normal=True,srgb=False)
    orm=np.stack((np.ones_like(rough),rough,metal),axis=-1)
    write_bc_dds(output/"L02_ORM.dds",data_mips(orm),srgb=False)
    for name,data in (("Roughness",rough),("Metallic",metal),("Emissive",emission)):
        Image.fromarray(np.uint8(data*255)).save(output/f"L02_{name}.png")
    Image.fromarray(np.uint8(orm*255)).save(output/"L02_ORM.png")
    crate=Path(__file__).resolve().parents[1]/"Assets/ItemBox/textures/S07_BaseColor.dds"
    Image.open(crate).convert("RGB").save(output/"REFERENCE_S07_BaseColor.png")
    print(f"L02 atlas and PBR maps -> {output}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path,
                        default=Path(__file__).resolve().parents[1] / "Assets/LunarKit/textures")
    make_atlas(parser.parse_args().output)
