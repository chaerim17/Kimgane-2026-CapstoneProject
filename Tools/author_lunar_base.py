"""Explicitly replace the old scattered settlements with the four-base sketch.

blender --background Assets/LunarKit/source/L02_LunarKit_Layout.blend
  --python-exit-code 1 --python Tools/author_lunar_base.py -- --apply-plan [--render]

Subsequent hand editing uses the embedded export_lunar_world.py, not this reset.
"""
import argparse
import json
import math
from pathlib import Path
import sys

import bpy

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "Assets/LunarKit"
sys.path.insert(0, str(ROOT / "Tools"))
import build_lunar_kit as kit
from export_lunar_world import export_world

SITES = [("A", "01 / Cargo Ruins", (-86,110), (29,26)),
         ("B", "02 / Habitat & Relay", (70,116), (30,27)),
         ("C", "03 / Salvage Works", (112,36), (29,26)),
         ("D", "04 / Landing & Decon", (0,0), (27,24))]
ROADS = [
    ("01_North_Link", 8, [(-65,110),(-42,109),(-18,113),(15,115),(46,116)]),
    ("02_West_Service", 8, [(-86,87),(-85,67),(-76,47),(-61,37),(-39,31),(-18,30),(1,27),(15,16),(16,0)]),
    ("03_East_Service", 8, [(-61,37),(-34,31),(-6,30),(23,31),(48,38),(74,40),(96,36)]),
    ("04_Relay_Spur", 7, [(69,94),(58,78),(51,59),(48,38)]),
    ("05_Old_South_Loop", 6, [(-76,47),(-85,25),(-81,4),(-66,-11),(-43,-18),(-18,-19),(0,-18)])]
MOUNTAINS = [(-146,91,35,40,24),(-109,170,43,25,21),(157,117,29,45,25),
             (7,79,27,25,23),(-147,-4,34,43,25),(108,-67,46,42,25),
             (-16,-131,48,34,23),(-107,-112,35,30,18),(174,-8,22,34,20)]
CRATERS = [(-56,-76,27,9,3.8), (49,-52,24,8,3.4),(-163,-66,18,6,2.4),
           (78,174,17,5,2.5),(-13,163,19,6,2.8),(151,-143,27,9,3.8),
           (-156,155,16,5,2.6),(-13,-53,9,2.5,1.2)]
ZONES = [("gravel",-87,107,46,42),("gravel",72,118,43,42),("gravel",112,34,44,41),
         ("gravel",0,0,39,34),("gravel",-65,-74,42,36),("gravel",61,-47,37,33),
         ("rock",-144,28,44,64),("rock",8,78,31,31),("rock",137,-66,54,52),
         ("rock",-20,-132,60,44),("rock",152,124,34,55)]


def clear_collection(name):
    col = bpy.data.collections.get(name)
    if col:
        for obj in list(col.objects): bpy.data.objects.remove(obj, do_unlink=True)
    else:
        col = bpy.data.collections.new(name); bpy.context.scene.collection.children.link(col)
    return col


def guide(col, name, kind, xy, scale, height=4.71):
    obj = bpy.data.objects.new(name, None); col.objects.link(obj)
    obj.location = (*xy,height); obj.scale = scale
    obj.empty_display_type = "CUBE" if kind=="site" else "SPHERE"
    obj.empty_display_size = 1; obj["world_kind"] = kind; obj["export_ignore"] = True
    obj.show_name = True; obj.color = (.9,.6,.15,1) if kind=="site" else (.4,.65,.8,1)
    return obj


def add_geology():
    catalog = json.loads((OUT / "catalog.json").read_text())
    kit.material_setup()
    for name in ("rock_large", "rock_medium", "rock_cluster"):
        counts = []
        for lod in range(4):
            old = bpy.data.collections.get(f"L02_{name}_LOD{lod}")
            if old:
                for obj in list(old.objects): bpy.data.objects.remove(obj, do_unlink=True)
                bpy.data.collections.remove(old)
            obj = kit.make_part(name,lod); kit.obj_export(OUT / f"obj/parts/{name}_LOD{lod}.obj",[obj])
            obj.data.calc_loop_triangles(); counts.append(len(obj.data.loop_triangles))
        catalog["parts"][name] = dict(lodMeshes=[f"obj/parts/{name}_LOD{i}.obj" for i in range(4)],
            triangles=counts, anchor=False, colliders=kit.COLLISIONS[name], fractureMeshes=[])
    (OUT / "catalog.json").write_text(json.dumps(catalog, indent=2)+"\n")


def author_features():
    col = clear_collection("L03_WORLD_Authoring")
    col.hide_render = True
    for key,label,xy,size in SITES:
        obj = guide(col,"PAD_"+key,"site",xy,(*size,1))
        obj["site_id"] = key; obj["label"] = label; obj["blend_m"] = 9.0
    for name,width,points in ROADS:
        data = bpy.data.curves.new("ROAD_"+name, "CURVE"); data.dimensions = "3D"
        spline = data.splines.new("BEZIER"); spline.bezier_points.add(len(points)-1)
        for bp,(x,z) in zip(spline.bezier_points,points):
            bp.co = (x,z,4.76); bp.handle_left_type = bp.handle_right_type = "AUTO"
        obj = bpy.data.objects.new("ROAD_"+name,data); col.objects.link(obj)
        obj["world_kind"] = "road"; obj["width_m"] = float(width); obj["blend_m"] = 7.0
        obj["export_ignore"] = True; obj.show_name = True; obj.show_in_front = True
        data.bevel_depth = .08; data.bevel_resolution = 0
    for i,(x,z,rx,rz,height) in enumerate(MOUNTAINS):
        guide(col,f"RIDGE_{i+1:02}","mountain",(x,z),(rx,rz,min(height,23)))
    for i,(x,z,r,depth,rim) in enumerate(CRATERS):
        obj = guide(col,f"CRATER_{i+1:02}","crater",(x,z),(r,r,1))
        obj["depth_m"] = depth; obj["rim_m"] = rim; obj["phase"] = .4+i*.77
    for i,(kind,x,z,rx,rz) in enumerate(ZONES):
        obj = guide(col,f"SURFACE_{i+1:02}_{kind}","zone",(x,z),(rx,rz,1))
        obj["material"] = kind
    bpy.context.view_layer.update()


def author_placements():
    layout = clear_collection("L02_LAYOUT_Export_Instances"); plan = []
    def place(asset,x,z,yaw=0,site="FIELD"):
        kind = "prefabs" if asset[0].isupper() else "parts"
        identity = f"L03_{site}_{len(plan):03}_{asset}"
        obj = kit.add_instance(bpy.data.collections[f"L02_{asset}_LOD0"],identity,(x,z,4.715),-yaw,layout)
        obj["prefab_id" if kind=="prefabs" else "part_id"] = asset
        obj["placement_id"] = identity; obj["site_id"] = site; obj["terrain_snap"] = True
        plan.append(dict(id=identity,asset=asset,type=kind,site=site,xzM=[x,z],yawDeg=yaw))
    # Existing playable landing base stays at the shared player/purifier coordinates.
    for asset,x,z,yaw in [("CargoDepot_Ruined",6,0,0),("PurificationStation",-6,0,0),
                         ("RelayPlatform",5,9,0),("ServiceCorridor",-4,8,0),
                         ("CoverCheckpoint",0,-10,0),("WreckWorkshop",-12,-8,90)]: place(asset,x,z,yaw,"D")
    groups = {
        "A": [("CargoDepot_Ruined",-98,115,0),("CargoDepot_Ruined",-75,116,0),
              ("WreckWorkshop",-103,96,0),("RelayPlatform",-72,96,0),
              ("ServiceCorridor",-86,97,0),("CoverCheckpoint",-86,85,0)],
        "B": [("CargoDepot_Ruined",56,122,0),("CargoDepot_Ruined",80,123,0),
              ("PurificationStation",81,104,0),("RelayPlatform",55,103,0),
              ("ServiceCorridor",68,115,90),("CoverCheckpoint",70,96,0)],
        "C": [("CargoDepot_Ruined",105,43,90),("CargoDepot_Ruined",125,46,0),
              ("WreckWorkshop",110,23,90),("RelayPlatform",128,24,0),
              ("ServiceCorridor",116,55,90),("CoverCheckpoint",95,29,90)]}
    for site,items in groups.items():
        for asset,x,z,yaw in items: place(asset,x,z,yaw,site)
    for site,_,(cx,cz),_ in SITES:
        for asset,dx,dz,yaw in [("solar_array",22,-12,0),("solar_array",22,-6,0),
            ("generator",-22,4,90),("oxygen_rack",-22,11,90),("cargo_container",18,17,0),
            ("debris_panel",-18,-17,90),("debris_beam",-20,-19,0),
            ("column_broken",18,-18,0),("wall_fragment",22,7,90)]:
            place(asset,cx+dx,cz+dz,yaw,site)
    # Paired surviving route markers sit on the shoulder, leaving the road center clear.
    for road_index,(_,width,points) in enumerate(ROADS):
        for i,(a,b) in enumerate(zip(points,points[1:])):
            dx,dz = b[0]-a[0],b[1]-a[1]; length = math.hypot(dx,dz)
            x,z = (a[0]+b[0])*.5,(a[1]+b[1])*.5
            if any(abs(x-cx)<hx+5 and abs(z-cz)<hz+5 for _,_,(cx,cz),(hx,hz) in SITES): continue
            for sign in (-1,1):
                offset = width*.5+1.4
                place("warning_beacon" if (i+road_index)%3==0 else "beacon",
                      x-sign*dz/length*offset,z+sign*dx/length*offset)
    rocks = [(-124,56),(-119,65),(-130,47),(-120,12),(-122,-20),(-132,-35),(-166,36),
             (-151,45),(-157,-43),(-119,-85),(-94,-85),(-87,-102),(-135,-124),
             (-72,-127),(-61,-141),(-36,-98),(3,-101),(18,-110),(21,-147),
             (65,-83),(72,-96),(88,-103),(142,-90),(149,-56),(150,-24),(169,38),
             (138,76),(145,82),(175,82),(139,146),(126,146),(38,82),(32,67),
             (-23,77),(-29,89),(-17,54),(1,51),(-131,126),(-134,139),(-67,153),
             (-80,157),(9,142),(29,148),(107,162),(174,-115),(-173,-126)]
    for i,(x,z) in enumerate(rocks): place(("rock_large","rock_medium","rock_cluster")[i%3],x,z,(i%4)*90)
    plan_config = dict(schemaVersion=2, coordinates="Engine X/Z ground, Y up; metres",
        heightRule="Blender-authored level pads + footprint minimum + 0.005m",
        sites=[dict(id=s,label=l,centerXZ=list(p)) for s,l,p,_ in SITES], placements=plan)
    (OUT / "settlements.json").write_text(json.dumps(plan_config,indent=2)+"\n")


def setup_views(render):
    scene = bpy.context.scene; camera = scene.camera
    scene.render.engine = "CYCLES"; scene.cycles.samples = 20; scene.cycles.use_denoising = True
    views = [("WorldOverview",(240,-330,390),(0,35,0),510,1600,1600),
             ("FourBase_Top",(0,20,450),(0,20,0),390,1600,1600),
             ("CargoRuins",(-47,59,42),(-86,110,5),72,1500,1000),
             ("RoadJunction",(-6,-6,28),(-42,33,5),95,1500,1000),
             ("CentralBase",(33,-44,34),(0,0,6),66,1500,1000)]
    for name,position,target,scale,width,height in views:
        camera.location = position; camera.data.ortho_scale = scale; camera.data.clip_end = 1500
        kit.point_at(camera,target)
        scene.render.resolution_x = width; scene.render.resolution_y = height
        scene.render.filepath = str(OUT / f"previews/L03_{name}.png")
        bookmark = bpy.data.objects.get("VIEW_L03_"+name)
        if bookmark is None:
            bookmark = bpy.data.objects.new("VIEW_L03_"+name,camera.data.copy()); scene.collection.objects.link(bookmark)
        bookmark.matrix_world = camera.matrix_world.copy(); bookmark.data.ortho_scale = scale; bookmark["export_ignore"] = True
        if render: bpy.ops.render.render(write_still=True)
    camera.location = views[0][1]; camera.data.ortho_scale = views[0][3]; kit.point_at(camera,views[0][2])
    scene.render.resolution_x = scene.render.resolution_y = 1600
    for screen in bpy.data.screens:
        for area in screen.areas:
            if area.type=="VIEW_3D":
                area.spaces.active.region_3d.view_distance = 310
                area.spaces.active.region_3d.view_location = (0,35,4.71)
    bpy.context.preferences.filepaths.save_version = 0
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT / "source/L02_LunarKit_Layout.blend"))


def main():
    parser = argparse.ArgumentParser(); parser.add_argument("--apply-plan", action="store_true", required=True)
    parser.add_argument("--render", action="store_true")
    args = parser.parse_args(sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else [])
    bpy.context.window.scene = bpy.data.scenes["L02_Layout"]
    bpy.context.scene["lunar_project_root"] = str(ROOT)
    add_geology(); author_features(); author_placements(); export_world(save=False)
    for filename in ("export_lunar_world.py", "export_lunar_layout.py"):
        old = bpy.data.texts.get(filename)
        if old: bpy.data.texts.remove(old)
        bpy.data.texts.load(str(ROOT / "Tools" / filename))
    text = bpy.data.texts.get("SETTLEMENTS_KO") or bpy.data.texts.new("SETTLEMENTS_KO")
    text.clear(); text.write("네 거점을 연결한 폐허 달 기지\n"
        "L03_WORLD_Authoring: ROAD_ Bezier 편집 / PAD_ 부지 / RIDGE_ 산 / CRATER_ / SURFACE_ 재질 구역\n"
        "도로 폭: width_m, 어깨: blend_m. 산은 스케일 XY=반경, Z=높이(m).\n"
        "PAD_ 스케일 XY=부지 반폭, 위치 Z=높이. 도로 노반 높이는 4.71m.\n"
        "시설은 L02_LAYOUT_Export_Instances에서 이동/복제. Z회전 90도, 스케일1 유지.\n"
        "export_lunar_world.py 실행: 지형 RAW + 표면 DDS + 배치 + 공용 충돌 + .blend 저장.\n"
        "terrain_snap 속성=True인 시설은 지형에 다시 밀착됩니다. 수동 높이 고정은 False.\n")
    start = bpy.data.texts.get("START_HERE_KO") or bpy.data.texts.new("START_HERE_KO")
    start.clear(); start.write(text.as_string())
    setup_views(args.render)
    print("L03_FOUR_BASE_AUTHORING_COMPLETE")


if __name__ == "__main__": main()
