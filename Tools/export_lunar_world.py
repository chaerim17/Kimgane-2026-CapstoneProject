"""Blender Text Editor: export editable roads/geology, terrain, placements and collision.

Open L02_LunarKit_Layout.blend, edit L03_WORLD_Authoring and/or layout instances,
then run this script. Only authored collection instances become runtime props.
No third party Blender add-on or Pillow dependency is needed.
"""
import json
import math
from pathlib import Path
import sys
import tempfile

import bpy
import numpy as np


def project_root():
    root = Path(bpy.context.scene.get("lunar_project_root", ""))
    if not (root / "Shared/World/TestMapSettings.h").exists():
        raise ValueError("Set Scene.lunar_project_root to the repository directory")
    return root


def read_features(root):
    config = json.loads((root / "Assets/Terrain/lunar_craters.json").read_text(encoding="utf-8"))
    collection = bpy.data.collections.get("L03_WORLD_Authoring")
    if not collection:
        raise ValueError("Missing L03_WORLD_Authoring. Run author_lunar_base.py --apply-plan once.")
    config.update(roads=[], sites=[], mountains=[], craters=[], surface_zones=[])
    for obj in sorted(collection.objects, key=lambda item: item.name):
        kind = obj.get("world_kind")
        if kind is None: continue
        if obj.parent or max(abs(v) for v in obj.rotation_euler) > 1e-5:
            raise ValueError(f"{obj.name}: authoring guides must be unparented and unrotated")
        center = [obj.location.x, obj.location.y]
        if kind == "road":
            if obj.type != "CURVE" or len(obj.data.splines) != 1 or tuple(obj.scale) != (1, 1, 1):
                raise ValueError(f"{obj.name}: use one unscaled Bezier spline")
            spline = obj.data.splines[0]
            if spline.type != "BEZIER" or spline.use_cyclic_u or len(spline.bezier_points) < 2:
                raise ValueError(f"{obj.name}: use an open Bezier spline with at least two points")
            points = []
            for a, b in zip(spline.bezier_points, list(spline.bezier_points)[1:]):
                for t in np.linspace(0, 1, 17, endpoint=False):
                    t = float(t)
                    p = (1-t)**3*a.co + 3*(1-t)**2*t*a.handle_right + 3*(1-t)*t*t*b.handle_left + t**3*b.co
                    p = obj.matrix_world @ p
                    points.append([round(p.x, 5), round(p.y, 5)])
            p = obj.matrix_world @ spline.bezier_points[-1].co
            points.append([p.x, p.y])
            controls = []
            for bp in spline.bezier_points:
                controls.append(dict(co=list(obj.matrix_world @ bp.co),
                    left=list(obj.matrix_world @ bp.handle_left), right=list(obj.matrix_world @ bp.handle_right)))
            config["roads"].append(dict(id=obj.name, widthM=float(obj["width_m"]),
                                         blendM=float(obj["blend_m"]), pointsM=points, bezierPoints=controls))
        elif kind == "site":
            config["sites"].append(dict(id=obj["site_id"], label=obj["label"], centerXZ=center,
                halfSizeM=[abs(obj.scale.x), abs(obj.scale.y)], heightM=obj.location.z, blendM=float(obj["blend_m"])))
        elif kind == "mountain":
            config["mountains"].append(dict(id=obj.name, centerXZ=center,
                radiusM=[abs(obj.scale.x), abs(obj.scale.y)], heightM=abs(obj.scale.z)))
        elif kind == "crater":
            config["craters"].append(center+[abs(obj.scale.x), float(obj["depth_m"]),
                                                     float(obj["rim_m"]), float(obj["phase"])])
        elif kind == "zone":
            config["surface_zones"].append(dict(id=obj.name, centerXZ=center,
                radiusM=[abs(obj.scale.x), abs(obj.scale.y)], material=obj["material"]))
        else:
            raise ValueError(f"Unknown world feature: {kind}")
    if len(config["sites"]) != 4 or not config["roads"]:
        raise ValueError("The lunar world requires four base pads and connecting roads")
    if any(r["widthM"] <= 0 or r["blendM"] <= 0 for r in config["roads"]):
        raise ValueError("Road width and blend must be positive")
    if any(min(s["halfSizeM"]+[s["blendM"]]) <= 0 for s in config["sites"]):
        raise ValueError("Base pad size and blend must be positive")
    if any(min(h["radiusM"]+[h["heightM"]]) <= 0 for h in config["mountains"]):
        raise ValueError("Mountain radius and height must be positive")
    if any(c[2] <= 0 or min(c[3:5]) < 0 for c in config["craters"]):
        raise ValueError("Crater radius must be positive; depth and rim must be nonnegative")
    if any(min(s["radiusM"]) <= 0 or s["material"] not in ("gravel","rock") for s in config["surface_zones"]):
        raise ValueError("Surface zones require positive radii and gravel/rock material")
    return config


def restore_features(root):
    """Kit rebuilds retain the exported editable guides, including Bezier handles."""
    config = json.loads((root / "Assets/Terrain/lunar_craters.json").read_text())
    from author_lunar_base import clear_collection, guide
    col = clear_collection("L03_WORLD_Authoring"); col.hide_render = True
    for site in config["sites"]:
        obj = guide(col,"PAD_"+site["id"],"site",site["centerXZ"],(*site["halfSizeM"],1),site["heightM"])
        obj["site_id"] = site["id"]; obj["label"] = site["label"]; obj["blend_m"] = site["blendM"]
    for road in config["roads"]:
        data = bpy.data.curves.new(road["id"], "CURVE"); data.dimensions = "3D"
        spline = data.splines.new("BEZIER"); controls = road["bezierPoints"]
        spline.bezier_points.add(len(controls)-1)
        for bp,values in zip(spline.bezier_points,controls):
            bp.handle_left_type = bp.handle_right_type = "FREE"
            bp.co = values["co"]; bp.handle_left = values["left"]; bp.handle_right = values["right"]
        obj = bpy.data.objects.new(road["id"],data); col.objects.link(obj)
        obj["world_kind"] = "road"; obj["width_m"] = road["widthM"]; obj["blend_m"] = road["blendM"]
        obj["export_ignore"] = True; obj.show_in_front = True; obj.show_name = True
        data.bevel_depth = .08; data.bevel_resolution = 0
    for hill in config["mountains"]:
        guide(col,hill["id"],"mountain",hill["centerXZ"],(*hill["radiusM"],hill["heightM"]))
    for i,(x,z,r,depth,rim,phase) in enumerate(config["craters"]):
        obj = guide(col,f"CRATER_{i+1:02}","crater",(x,z),(r,r,1))
        obj["depth_m"] = depth; obj["rim_m"] = rim; obj["phase"] = phase
    for zone in config["surface_zones"]:
        obj = guide(col,zone["id"],"zone",zone["centerXZ"],(*zone["radiusM"],1)); obj["material"] = zone["material"]
    return config


def write_transport_routes(root, config):
    from place_lunar_settlements import height_sampler
    sample = height_sampler(); lines = ["# Blender Bezier centerlines; real capsule traversal in both directions"]
    for road in config["roads"]:
        lines.append("road "+road["id"])
        for x,z in road["pointsM"]:
            lines.append(f"point {x:.6f} {sample(x,z):.6f} {z:.6f}")
    (root / "Assets/LunarKit/transport-routes.txt").write_text("\n".join(lines)+"\n")


def refresh_terrain(root, heights):
    terrain = bpy.data.objects["REFERENCE_LunarTerrain_384m_R16"]
    for vertex, height in zip(terrain.data.vertices, heights.flat): vertex.co.z = float(height)
    terrain.data.update()
    mask_path = str(root / "Assets/Terrain/lunar_surface_mask.png")
    previous = bpy.data.images.get("L03_World_Surface_Mask")
    image = bpy.data.images.load(mask_path, check_existing=False)
    image.colorspace_settings.name = "Non-Color"; image.pack()
    if previous:
        previous.user_remap(image); bpy.data.images.remove(previous)
    image.name = "L03_World_Surface_Mask"
    mat = bpy.data.materials.get("L03_Authored_Lunar_Surface") or bpy.data.materials.new("L03_Authored_Lunar_Surface")
    mat.use_nodes = True; nodes, links = mat.node_tree.nodes, mat.node_tree.links
    nodes.clear()
    shader = nodes.new("ShaderNodeBsdfPrincipled"); shader.inputs["Roughness"].default_value = .94
    output = nodes.new("ShaderNodeOutputMaterial"); links.new(shader.outputs[0], output.inputs[0])
    geometry = nodes.new("ShaderNodeNewGeometry")
    scale = nodes.new("ShaderNodeVectorMath"); scale.operation = "SCALE"; scale.inputs[3].default_value = 1/384
    links.new(geometry.outputs["Position"], scale.inputs[0])
    add = nodes.new("ShaderNodeVectorMath"); add.operation = "ADD"; add.inputs[1].default_value = (.5,.5,0)
    links.new(scale.outputs[0], add.inputs[0])
    mask = nodes.new("ShaderNodeTexImage"); mask.image = image; mask.extension = "EXTEND"
    links.new(add.outputs[0], mask.inputs[0])
    split = nodes.new("ShaderNodeSeparateColor"); links.new(mask.outputs[0], split.inputs[0])
    texscale = nodes.new("ShaderNodeVectorMath"); texscale.operation = "SCALE"; texscale.inputs[3].default_value = .2
    links.new(geometry.outputs["Position"], texscale.inputs[0])
    colors = []
    for name in ("dust", "gravel", "rock"):
        tex = nodes.new("ShaderNodeTexImage")
        tex.image = bpy.data.images.load(str(root / f"Assets/Textures/Terrain/lunar_{name}_basecolor.png"), check_existing=True)
        tex.image.pack(); tex.projection = "BOX"; tex.projection_blend = .2
        links.new(texscale.outputs[0], tex.inputs[0]); colors.append(tex.outputs[0])
    mix = nodes.new("ShaderNodeMixRGB"); links.new(split.outputs["Green"], mix.inputs[0])
    links.new(colors[0], mix.inputs[1]); links.new(colors[1], mix.inputs[2])
    normal = nodes.new("ShaderNodeSeparateXYZ"); links.new(geometry.outputs["Normal"], normal.inputs[0])
    slope = nodes.new("ShaderNodeMapRange"); slope.inputs["From Min"].default_value = .98; slope.inputs["From Max"].default_value = .7
    links.new(normal.outputs["Z"], slope.inputs["Value"])
    maximum = nodes.new("ShaderNodeMath"); maximum.operation = "MAXIMUM"
    links.new(slope.outputs[0], maximum.inputs[0]); links.new(split.outputs["Blue"], maximum.inputs[1])
    rocks = nodes.new("ShaderNodeMixRGB"); links.new(maximum.outputs[0], rocks.inputs[0])
    links.new(mix.outputs[0], rocks.inputs[1]); links.new(colors[2], rocks.inputs[2])
    dark = nodes.new("ShaderNodeMixRGB"); dark.blend_type = "MULTIPLY"; dark.inputs[0].default_value = 1
    links.new(colors[1], dark.inputs[1]); dark.inputs[2].default_value = (.39,.43,.49,1)
    road = nodes.new("ShaderNodeMixRGB"); links.new(split.outputs["Red"], road.inputs[0])
    links.new(rocks.outputs[0], road.inputs[1]); links.new(dark.outputs[0], road.inputs[2])
    links.new(road.outputs[0], shader.inputs["Base Color"])
    terrain.data.materials.clear(); terrain.data.materials.append(mat)


def snap_and_report(root):
    from place_lunar_settlements import footprint, height_sampler, rotate
    out = root / "Assets/LunarKit"; sample = height_sampler()
    catalog = json.loads((out / "catalog.json").read_text())
    report, routes = [], ["# id startX startY startZ endX endY endZ; shared-world building traversal"]
    bounds = {}
    for obj in bpy.data.collections["L02_LAYOUT_Export_Instances"].objects:
        asset = obj.get("prefab_id") or obj.get("part_id")
        kind = "prefabs" if obj.get("prefab_id") else "parts"
        if not asset and obj.instance_type == "COLLECTION" and obj.instance_collection:
            for section in ("prefabs", "parts"):
                asset = next((name for name in catalog[section] if obj.instance_collection.name==f"L02_{name}_LOD0"),None)
                if asset:
                    kind = section
                    obj["prefab_id" if kind=="prefabs" else "part_id"] = asset
                    break
        if not asset: continue  # Invalid geometry is rejected by export_layout.
        yaw = -math.degrees(obj.rotation_euler.z); x, z = obj.location.xy
        key = (asset, round(yaw/90)%2)
        if key not in bounds: bounds[key] = footprint(dict(asset=asset, type=kind), catalog, yaw)
        hx, hz = bounds[key]; nx, nz = max(1, math.ceil(hx*3)), max(1, math.ceil(hz*3))
        heights = [sample(x-hx+i*2*hx/nx, z-hz+j*2*hz/nz) for i in range(nx+1) for j in range(nz+1)]
        low, high = min(heights), max(heights)
        if obj.get("terrain_snap", False): obj.location.z = low+.005
        y = obj.location.z; identity = obj.get("placement_id", obj.name)
        report.append(dict(id=identity, site=obj.get("site_id", "FIELD"), asset=asset,
                           groundMinM=low, groundMaxM=high, originHeightM=y))
        if asset in ("CargoDepot_Ruined", "CargoDepot_Intact", "PurificationStation", "ServiceCorridor"):
            door_x = 2 if asset in ("CargoDepot_Intact", "PurificationStation") else 0
            values = []
            for local_z in (-6,6):
                dx,dz = rotate(door_x,local_z,yaw); wx,wz = x+dx,z+dz
                values.extend((wx,max(y,sample(wx,wz)),wz))
            routes.append(identity+" "+" ".join(f"{v:.6f}" for v in values))
    (out / "placement-report.json").write_text(json.dumps(report, indent=2)+"\n")
    (out / "traversal-routes.txt").write_text("\n".join(routes)+"\n")


def export_world(save=True):
    root = project_root(); sys.path.insert(0, str(root / "Tools"))
    from bake_lunar_heightmap import bake, write_outputs
    from export_lunar_layout import export_layout
    config = read_features(root)
    bake(config)  # Validate the full height range before writing authoring/runtime files.
    # Reject unsupported instance transforms before changing terrain outputs.
    with tempfile.TemporaryDirectory(prefix="lunar-world-preflight-") as directory:
        staging = Path(directory)
        (staging / "Shared/World").mkdir(parents=True); (staging / "Shared/World/TestMapSettings.h").touch()
        (staging / "Assets/LunarKit").mkdir(parents=True)
        (staging / "Assets/LunarKit/catalog.json").write_bytes((root / "Assets/LunarKit/catalog.json").read_bytes())
        export_layout(staging)
    (root / "Assets/Terrain/lunar_craters.json").write_text(json.dumps(config, indent=2)+"\n", encoding="utf-8")
    heights = write_outputs(config, root)
    refresh_terrain(root, heights); snap_and_report(root); write_transport_routes(root,config)
    bpy.context.view_layer.update(); export_layout(root)
    from build_lunar_kit import collider_reference
    old = bpy.data.collections.get("L02_COLLIDERS_Reference_Toggle_Viewport")
    if old:
        for obj in list(old.objects): bpy.data.objects.remove(obj,do_unlink=True)
        bpy.data.collections.remove(old)
    collider_reference()
    for filename in ("export_lunar_world.py","export_lunar_layout.py"):
        text = bpy.data.texts.get(filename) or bpy.data.texts.new(filename)
        text.clear(); text.write((root / "Tools" / filename).read_text(encoding="utf-8"))
    if save:
        bpy.context.preferences.filepaths.save_version = 0
        bpy.ops.wm.save_as_mainfile(filepath=str(root / "Assets/LunarKit/source/L02_LunarKit_Layout.blend"))
    print("L03_WORLD_EXPORTED: editable roads / pads / mountains / craters / material regions + shared collision")
    return config


if __name__ == "__main__": export_world()
