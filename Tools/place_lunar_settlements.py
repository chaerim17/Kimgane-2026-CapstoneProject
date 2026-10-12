"""Apply the reviewed eight-site plan to the saved Blender lunar layout.

Run on L02_LunarKit_Layout.blend in a background Blender process, with --apply-plan.
This explicit operation replaces the placement collection from settlements.json.
The normal kit builder preserves subsequent manual edits through layout.json.
"""
import argparse
import array
import json
import math
from pathlib import Path
import sys

import bpy

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "Assets/LunarKit"
sys.path.insert(0, str(ROOT / "Tools"))
from build_lunar_kit import add_instance, collider_reference, point_at
from export_lunar_layout import export_layout


def height_sampler():
    raw = array.array("H")
    raw.frombytes((ROOT / "Assets/Terrain/lunar_craters_r16.raw").read_bytes())
    if sys.byteorder != "little":
        raw.byteswap()
    if len(raw) != 513 * 513:
        raise ValueError("Expected the current 513x513 lunar R16 terrain")

    def sample(x, z):
        fx, fz = (x + 192) / .75, (z + 192) / .75
        ix, iz = int(fx), int(fz)
        if not 0 <= ix < 512 or not 0 <= iz < 512:
            raise ValueError("Placement outside the current terrain")
        u, v = fx - ix, fz - iz
        a, b = raw[iz * 513 + ix], raw[iz * 513 + ix + 1]
        c, d = raw[(iz + 1) * 513 + ix], raw[(iz + 1) * 513 + ix + 1]
        return ((a * (1-u) + b*u) * (1-v) + (c * (1-u) + d*u) * v) / 65535 * 48 - 16

    return sample


def footprint(asset, catalog, yaw):
    path = OUT / catalog[asset["type"]][asset["asset"]]["lodMeshes"][0]
    points = [tuple(map(float, line.split()[1:4])) for line in path.read_text().splitlines()
              if line.startswith("v ")]
    hx, hz = (max(abs(p[i]) for p in points) for i in (0, 2))
    return (hz, hx) if round(yaw / 90) % 2 else (hx, hz)


def rotate(x, z, yaw):
    angle = math.radians(yaw)
    return x * math.cos(angle) + z * math.sin(angle), -x * math.sin(angle) + z * math.cos(angle)


def apply_plan():
    config = json.loads((OUT / "settlements.json").read_text())
    catalog = json.loads((OUT / "catalog.json").read_text())
    sample = height_sampler()
    prepared = []
    for entry in config["placements"]:
        x, z = entry["xzM"]
        hx, hz = footprint(entry, catalog, entry["yawDeg"])
        # Sample densely enough to catch the terrain vertices beneath each envelope.
        nx, nz = math.ceil(hx * 4), math.ceil(hz * 4)
        heights = [sample(x-hx+i*2*hx/nx, z-hz+j*2*hz/nz)
                   for i in range(nx+1) for j in range(nz+1)]
        low, high = min(heights), max(heights)
        y = entry.get("heightM", low + .005)
        collection = bpy.data.collections.get(f"L02_{entry['asset']}_LOD0")
        if collection is None:
            raise ValueError(f"Rebuild the kit first: missing {entry['asset']}")
        prepared.append((entry, collection, y, low, high))

    scene = bpy.data.scenes["L02_Layout"]
    bpy.context.window.scene = scene
    layout = bpy.data.collections["L02_LAYOUT_Export_Instances"]
    for obj in list(layout.objects):
        bpy.data.objects.remove(obj, do_unlink=True)
    report, routes = [], ["# id startX startY startZ endX endY endZ; independent shared-world traversal"]
    for entry, collection, y, low, high in prepared:
        x, z = entry["xzM"]
        obj = add_instance(collection, entry["id"], (x, z, y), -entry["yawDeg"], layout)
        obj["prefab_id" if entry["type"] == "prefabs" else "part_id"] = entry["asset"]
        obj["placement_id"] = entry["id"]
        obj["site_id"] = entry["site"]
        report.append({"id": entry["id"], "site": entry["site"], "asset": entry["asset"],
                       "groundMinM": low, "groundMaxM": high, "originHeightM": y})
        name = entry["asset"]
        if name in ("CargoDepot_Ruined", "CargoDepot_Intact", "PurificationStation", "ServiceCorridor"):
            door_x = 2 if name in ("CargoDepot_Intact", "PurificationStation") else 0
            points = []
            for local_z in (-6, 6):
                dx, dz = rotate(door_x, local_z, entry["yawDeg"])
                wx, wz = x + dx, z + dz
                points.extend((wx, max(y, sample(wx, wz)), wz))
            routes.append(entry["id"] + " " + " ".join(f"{v:.6f}" for v in points))
    bpy.context.view_layer.update()
    scene["lunar_project_root"] = str(ROOT)
    export_layout(ROOT)
    (OUT / "placement-report.json").write_text(json.dumps(report, indent=2) + "\n")
    (OUT / "traversal-routes.txt").write_text("\n".join(routes) + "\n")
    old = bpy.data.collections.get("L02_COLLIDERS_Reference_Toggle_Viewport")
    if old:
        for obj in list(old.objects):
            bpy.data.objects.remove(obj, do_unlink=True)
        bpy.data.collections.remove(old)
    collider_reference()
    return config


def render_views(config, render):
    scene = bpy.context.scene
    camera = scene.camera
    scene.cycles.samples = 24
    views = [
        ("WorldOverview", (235, -330, 370), (0, 0, -1), 550, 1800, 1800),
        ("CentralBase", (33, -44, 34), (0, 0, 6), 53, 1600, 1050),
        ("NWCargoYard", (-93, 15, 36), (-124, 55, 6), 63, 1600, 1050),
        ("EastSalvage", (104, -82, 40), (70, -30, 7), 70, 1600, 1050),
        ("SEMedicalOutpost", (180, -166, 41), (148, -115, 6), 65, 1600, 1050),
    ]
    for name, position, target, scale, width, height in views:
        camera.location = position
        camera.data.ortho_scale = scale
        camera.data.clip_end = 1500
        point_at(camera, target)
        scene.render.resolution_x, scene.render.resolution_y = width, height
        scene.render.filepath = str(OUT / f"previews/L02_{name}.png")
        if render:
            bpy.ops.render.render(write_still=True)
        # Save separate camera bookmarks in the .blend for every reviewed view.
        bookmark = bpy.data.objects.get("VIEW_" + name)
        if bookmark is None:
            bookmark = bpy.data.objects.new("VIEW_" + name, camera.data.copy())
            scene.collection.objects.link(bookmark)
        bookmark.matrix_world = camera.matrix_world.copy()
        bookmark.data.ortho_scale = scale
        bookmark["export_ignore"] = True
    # Open on the populated central base, with distant sites available in Outliner / camera bookmarks.
    camera.location = views[1][1]
    camera.data.ortho_scale = views[1][3]
    point_at(camera, views[1][2])
    scene.render.resolution_x, scene.render.resolution_y = 1600, 1050
    text = bpy.data.texts.get("SETTLEMENTS_KO") or bpy.data.texts.new("SETTLEMENTS_KO")
    text.clear()
    text.write("8개 거점 / 27개 조합 프리팹 + 73개 개별 설비·비콘\n"
               "L02_LAYOUT_Export_Instances에서 복제/이동 후 export_lunar_layout.py 실행.\n"
               "VIEW_ 카메라: 전체 지형, 중앙 기지, 북서 화물, 동쪽 잔해, 남동 정화시설.\n"
               "아래 거점 표기는 site_id Custom Property로도 확인할 수 있습니다.\n" +
               "\n".join(f"{site['id']}: {site['label']} {site['centerXZ']}" for site in config["sites"]))
    bpy.context.preferences.filepaths.save_version = 0
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT / "source/L02_LunarKit_Layout.blend"))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--apply-plan", action="store_true", required=True,
                        help="Explicitly replace placements with the reviewed settlements.json plan")
    parser.add_argument("--render", action="store_true")
    args = parser.parse_args(sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else [])
    render_views(apply_plan(), args.render)
    print("L02_SETTLEMENT_PLACEMENT_COMPLETE")
