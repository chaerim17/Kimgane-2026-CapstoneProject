"""Check terrain access to the exported placements without changing the RAW.

Requires numpy. Uses a conservative 0.90 gradient limit (about 42 degrees),
below the existing 45-degree walkable-slope threshold. This is a topology check;
native character tests separately exercise building doors and placed colliders.
"""
import json
from collections import deque
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "Assets/LunarKit"


def main():
    h = np.fromfile(ROOT / "Assets/Terrain/lunar_craters_r16.raw", dtype="<u2").reshape(513, 513) / 65535 * 48 - 16
    dz, dx = np.gradient(h, .75)
    # Require a capsule-width neighborhood of walkable cells, rather than a one-cell gap.
    walkable = dx*dx + dz*dz < .90**2
    safe = walkable.copy()
    for z in (-1, 0, 1):
        for x in (-1, 0, 1):
            safe &= np.roll(walkable, (z, x), (0, 1))
    safe[[0, -1], :] = False
    safe[:, [0, -1]] = False
    connected = np.zeros_like(safe)
    queue = deque([(248, 251)])  # Outside the central medical south door: x=-3.75,z=-6m.
    connected[248, 251] = True
    while queue:
        iz, ix = queue.popleft()
        for j, i in ((iz+1, ix), (iz-1, ix), (iz, ix+1), (iz, ix-1)):
            if 0 <= i < 513 and 0 <= j < 513 and safe[j, i] and not connected[j, i]:
                connected[j, i] = True
                queue.append((j, i))
    layout = json.loads((OUT / "layout.json").read_text())
    for entry in layout["instances"]:
        x, _, z = entry["positionM"]
        ix, iz = round((x+192)/.75), round((z+192)/.75)
        if not connected[iz, ix]:
            raise AssertionError(f"Terrain access isolated by steep slopes: {entry['id']}")
    print(f"L02_TERRAIN_ACCESS_PASSED: {len(layout['instances'])} placements on the landing-base walkable component")


if __name__ == "__main__":
    main()
