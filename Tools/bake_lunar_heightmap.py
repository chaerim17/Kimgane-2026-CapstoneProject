"""Bake the editable crater layout to headerless R16 RAW. Requires numpy and Pillow.

Run from any directory: python Tools/bake_lunar_heightmap.py
RAW = (world_height - height_min_m) / height_range_m * 65535.
The runtime reads only RAW; JSON and PNG are authoring/inspection assets.
"""
import json
from pathlib import Path

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]


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


def bake(config):
    z, x = np.mgrid[:config["length"], :config["width"]].astype(np.float64)
    x = (x - (config["width"] - 1) * 0.5) * config["cell_spacing_m"]
    z = (z - (config["length"] - 1) * 0.5) * config["cell_spacing_m"]
    h = config["landing_height_m"] + 2.8 * (noise(x / 53, z / 53) - 0.5)
    h += 1.0 * (noise(x / 17 + 31, z / 17 - 12) - 0.5)
    h += 0.22 * (noise(x / 3 + 9, z / 3 + 4) - 0.5)
    for cx, cz, radius, depth, rim_height, phase in config["craters"]:
        angle = np.arctan2(z - cz, x - cx)
        irregularity = 1 + 0.045 * np.sin(angle * 5 + phase) + 0.024 * np.sin(angle * 11 - phase)
        r = np.hypot(x - cx, z - cz) / (radius * irregularity)
        bowl = 1 - smooth(0.48, 0.97, r)
        rim = np.exp(-((r - 1) / 0.14) ** 2)
        ejecta = np.exp(-((r - 1.18) / 0.28) ** 2)
        fluting = 0.75 + 0.25 * np.sin(angle * 23 + phase)
        h += -depth * bowl + rim_height * rim + rim_height * 0.16 * ejecta * fluting
        h += 0.32 * np.sin(angle * 31 + r * 12) * smooth(0.55, 0.8, r) * (1 - smooth(1.05, 1.4, r))
    landing = smooth(config["landing_radius_m"], config["landing_radius_m"] + 12, np.hypot(x, z))
    h = config["landing_height_m"] + (h - config["landing_height_m"]) * landing
    encoded = (h - config["height_min_m"]) / config["height_range_m"]
    if encoded.min() < 0 or encoded.max() > 1:
        raise ValueError("Height range clips the terrain; update JSON and TestMapSettings.h together")
    return h, np.round(encoded * 65535).astype("<u2")


def main():
    directory = ROOT / "Assets/Terrain"
    config = json.loads((directory / "lunar_craters.json").read_text())
    heights, raw = bake(config)
    (directory / "lunar_craters_r16.raw").write_bytes(raw.tobytes())
    Image.fromarray(raw).save(directory / "lunar_height_preview.png")
    dz, dx = np.gradient(heights, config["cell_spacing_m"])
    normal = np.stack((-dx, np.ones_like(dx), -dz), -1)
    normal /= np.linalg.norm(normal, axis=-1, keepdims=True)
    light = np.array([-0.45, 0.78, -0.43])
    shade = np.clip(normal @ light, 0, 1) * 0.7 + 0.22
    color = shade[..., None] * np.array([170, 188, 215])
    Image.fromarray(np.uint8(np.clip(color, 0, 255))).save(directory / "lunar_layout_preview.png")
    print(f"Baked {raw.shape[1]}x{raw.shape[0]} R16 RAW: {heights.min():.2f} .. {heights.max():.2f} m")


if __name__ == "__main__":
    main()
