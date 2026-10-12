"""Build original L02 modular lunar environment in a fresh Blender process.

blender --background --factory-startup --python-exit-code 1 --python Tools/build_lunar_kit.py -- --render
Generate textures first with generate_lunar_kit_atlas.py. No external Blender add-ons.
"""
import argparse
import json
import math
import os
import re
from pathlib import Path
import sys

sys.path.insert(0,str(Path(__file__).resolve().parent))
import bpy
import bmesh
import numpy as np
from mathutils import Matrix, Vector
from mathutils.bvhtree import BVHTree
import lunar_kit_mesh as meshutil
from lunar_kit_recipes import ROOT, OUT, load_recipes

TYPES = ["floor_4m","floor_2m","roof_4m","roof_skylight_4m","wall_solid_2m",
         "wall_window_2m","wall_vent_2m","wall_half_2m","wall_breach_4m","wall_fragment",
         "doorframe_4m","column_4m","column_broken","beam_4m","beam_2m","railing_4m",
         "barricade_2m","cargo_container","oxygen_rack","generator","hvac_unit","console",
         "workbench","purification_pod","relay_mast","solar_array","beacon","warning_beacon",
         "sign_cargo","sign_medical","sign_relay","debris_panel","debris_beam","pipe_straight",
         "pipe_elbow","wall_shard_a","wall_shard_b","wall_shard_c","wall_shard_d"]
ANCHORS = {"floor_4m","floor_2m"}
MESHES = {}
COLLISIONS = {}
CATALOG = {"schemaVersion":2,"units":"meters","gridM":4,"wallSubgridM":2,
           "coordinates":"Engine LH X/Y-up/Z-depth; source Blender X/Y-depth/Z-up",
           "objUv":"V preflipped for existing DirectX ObjLoader; Blender standard UV",
           "lodDistancesM":[22,55,110],"parts":{},"prefabs":{}}


def collect(name):
    col=bpy.data.collections.new(name)
    col.use_fake_user=True
    return col


def material_setup():
    images={}
    for name in ("BaseColor","Roughness","Metallic","NormalGL","Emissive"):
        image=bpy.data.images.load(str(OUT/f"textures/L02_{name}.png"))
        if name not in ("BaseColor","Emissive"):
            image.colorspace_settings.name="Non-Color"
        image.pack()
        images[name]=image
    for tile in range(16):
        mat=bpy.data.materials.new(f"L02_Atlas_{tile:02}")
        mat.use_nodes=True
        nodes,links=mat.node_tree.nodes,mat.node_tree.links
        shader=nodes.get("Principled BSDF")
        for name,socket in (("BaseColor","Base Color"),("Roughness","Roughness"),
                            ("Metallic","Metallic"),("Emissive","Emission Color")):
            tex=nodes.new("ShaderNodeTexImage"); tex.image=images[name]
            links.new(tex.outputs["Color"],shader.inputs[socket])
        normal=nodes.new("ShaderNodeNormalMap")
        tex=nodes.new("ShaderNodeTexImage"); tex.image=images["NormalGL"]
        links.new(tex.outputs["Color"],normal.inputs["Color"])
        links.new(normal.outputs["Normal"],shader.inputs["Normal"])
        shader.inputs["Emission Strength"].default_value=.4
        meshutil.MATERIALS.append(mat)


class Unit:
    def __init__(self,name,lod):
        self.name=name; self.objects=[]; self.colliders=[]; self.lod=lod

    def box(self,c,s,tile=0,collide=False,bevel=.035,detail=3):
        if self.lod>detail:
            return None
        bevel=bevel if self.lod<2 else 0
        return meshutil.box(self,c,s,tile,bevel,collide and self.lod==0)

    def cyl(self,c,r,d,tile=2,axis="Z",detail=3):
        if self.lod>detail:
            return None
        obj=meshutil.cylinder(self,c,r,d,tile,[16,12,8,6][self.lod],axis)
        return obj

    def beam(self,a,b,width=.12,tile=2,detail=2):
        if self.lod<=detail:
            return meshutil.beam(self,a,b,width,tile)


def wall_polygon(p,outline,depth=.3,tile=0):
    n=len(outline)
    verts=[(x,y,z) for y in (-depth/2,depth/2) for x,z in outline]
    faces=[tuple(reversed(range(n))),tuple(range(n,2*n))]
    faces += [(i,(i+1)%n,(i+1)%n+n,i+n) for i in range(n)]
    mesh=bpy.data.meshes.new("Closed_fracture_shell")
    mesh.from_pydata(verts,[],faces); mesh.update()
    bm=bmesh.new(); bm.from_mesh(mesh)
    bmesh.ops.recalc_face_normals(bm,faces=bm.faces); bm.to_mesh(mesh); bm.free()
    obj=bpy.data.objects.new("Fracture",mesh); bpy.context.scene.collection.objects.link(obj)
    meshutil.finish_shape(p,obj,tile)
    return obj


def attach_fasteners(part):
    # Use a real face on the primary shell. A global bounding-box height can belong
    # to a distant antenna or exhaust, leaving small details suspended in space.
    bpy.context.view_layer.update()
    shell=part.objects[0]
    matrix=shell.matrix_world
    vertices=[matrix @ v.co for v in shell.data.vertices]
    # Include panel trim and equipment too, so a supported bolt is not hidden
    # beneath a second surface on the primary shell.
    all_vertices=[]; all_polygons=[]
    for obj in part.objects:
        offset=len(all_vertices)
        all_vertices.extend(obj.matrix_world @ v.co for v in obj.data.vertices)
        all_polygons.extend([v+offset for v in polygon.vertices] for polygon in obj.data.polygons)
    body=BVHTree.FromPolygons(all_vertices,all_polygons)
    candidates=[]; radius=.028; depth=.03; embed=.003
    for polygon in shell.data.polygons:
        points=[vertices[i] for i in polygon.vertices]
        normal=(matrix.to_3x3() @ polygon.normal).normalized()
        if normal.z<-.5: continue
        edge=max((b-a for a,b in zip(points,points[1:]+points[:1])),key=lambda e:e.length_squared)
        tangent=edge.normalized(); center=sum(points,Vector())/len(points)
        locations=[center+tangent*offset for offset in (-.03,.03)]
        supported=True
        for point in locations:
            for i in range(16):
                angle=i*math.tau/16
                offset=(tangent*math.cos(angle)+normal.cross(tangent)*math.sin(angle))*(radius+.001)
                hit,_,_,distance=body.ray_cast(point+offset+normal*.1,-normal,.2)
                if hit is None or abs(distance-.1)>.0001:
                    supported=False; break
            if not supported: break
        if supported:
            candidates.append((polygon.area*(1.01 if normal.z>.9 else 1),normal,locations))
    if not candidates:
        raise ValueError(f"No supported fastening face: {part.name}")
    _,normal,locations=max(candidates,key=lambda item:item[0])
    anchors=[]
    for surface in locations:
        obj=part.cyl(surface+normal*(depth/2-embed),radius,depth,2,detail=0)
        obj.rotation_euler=normal.to_track_quat("Z","Y").to_euler()
        attribute=obj.data.attributes.new("L02_Fastener","INT","POINT")
        for value in attribute.data: value.value=1
        anchors.append({"surfaceM":list(surface),"normal":list(normal),"radiusM":radius})
    return anchors


def make_part(name,lod):
    p=Unit(name,lod); B=p.box; C=p.cyl
    if name.startswith("floor_") or name.startswith("roof_"):
        sx=2 if name=="floor_2m" else 4
        roof=name.startswith("roof_")
        if name=="roof_skylight_4m":
            for x in (-1.4,1.4): B((x,0,.11),(1.2,4,.22),0,True)
            for y in (-1.4,1.4): B((0,y,.11),(1.6,1.2,.22),0,True)
            # Open skylight: no invisible glass or collider across the opening.
            for x in (-.85,.85): B((x,0,.23),(.12,1.85,.12),3,detail=2)
        else:
            B((0,0,.11 if roof else -.11),(sx,4,.22),0 if roof else 1,True)
        for x in (-sx/2+.16,sx/2-.16):
            B((x,0,.23 if roof else .008),(.06,3.7,.016),2,detail=1,bevel=0)
        if not roof:
            for y in (-1.8,1.8): B((0,y,.008),(sx-.4,.08,.016),3,detail=2,bevel=0)
    elif name in ("wall_solid_2m","wall_vent_2m","wall_half_2m","wall_window_2m"):
        height=1.2 if name=="wall_half_2m" else 4
        if name=="wall_window_2m":
            B((0,0,.65),(2,.32,1.3),0,True)
            B((0,0,3.5),(2,.32,1),0,True)
            for x in (-.87,.87): B((x,0,2.15),(.26,.32,1.7),2,True)
            for z in (1.3,3): B((0,-.19,z),(1.5,.12,.1),3,detail=2)
        else:
            B((0,0,height/2),(2,.32,height),0,True)
        for x in (-.86,.86): B((x,-.20,height/2),(.13,.1,height-.22),2,detail=2)
        B((0,-.195,.32),(1.7,.055,.10),3,detail=2,bevel=.01)
        if name=="wall_vent_2m":
            B((0,-.21,2.3),(1.35,.12,1.15),6,detail=2)
            for z in (1.9,2.1,2.3,2.5,2.7): B((0,-.3,z),(1.2,.12,.08),2,detail=1)
        elif name=="wall_solid_2m":
            B((0,-.19,2.2),(1.5,.055,2.3),15,detail=1)
            B((-.67,-.23,2.5),(.075,.04,.65),5,detail=2)
    elif name=="doorframe_4m":
        # 2.8m clear width, 3.1m clear height, no threshold.
        for x in (-1.7,1.7):
            B((x,0,1.55),(.6,.5,3.1),2,True)
            B((x,-.29,1.7),(.12,.08,1.9),5,detail=2)
            for z in (.2,2.9): B((x,-.27,z),(.56,.1,.2),3,detail=1)
        B((0,0,3.55),(4,.5,.9),0,True)
        B((0,-.28,3.38),(2.7,.08,.12),3,detail=2)
    elif name=="wall_breach_4m":
        wall_polygon(p,[(-2,0),(-1.4,0),(-1.4,1.1),(-1.7,1.5),(-1.45,1.8),(-2,2.1)])
        wall_polygon(p,[(1.4,0),(2,0),(2,4),(1.6,4),(1.8,3.3),(1.4,2.8)])
        wall_polygon(p,[(-2,3.5),(-1.3,3.7),(-.7,3.5),(.1,3.8),(.8,3.4),(2,3.5),(2,4),(-2,4)])
        # Colliders follow occupied strips, keeping a full 2.8m-wide breach clear.
        if lod==0:
            for c,s in (((-1.7,0,.55),(.6,.3,1.1)),((1.7,0,1.4),(.6,.3,2.8)),((0,0,3.9),(4,.3,.2))):
                meshutil.box(p,c,s,0,0,True).hide_render=True
                obj=p.objects.pop(); bpy.data.objects.remove(obj,do_unlink=True)
        p.beam((-1.85,-.02,2),(-1.2,-.2,2.4),.09)
        B((1.75,-.2,1),(.12,.07,1.4),3,detail=2)
    elif name.startswith("wall_shard_"):
        i="abcd".index(name[-1]); x=-1 if i%2==0 else 0; z=0 if i<2 else 2
        wall_polygon(p,[(x,z),(x+1,z),(x+1,z+1.75),(x+.65,z+2),(x+.2,z+1.8),(x,z+2)],tile=0)
        B((x+.5,-.19,z+.25),(.85,.05,.08),3,detail=1)
    elif name=="wall_fragment":
        wall_polygon(p,[(-1,0),(1,0),(.9,.55),(.3,.8),(-.1,.5),(-.5,1.4),(-1,1.2)])
        B((-.75,0,.5),(.5,.32,1),0,True,bevel=0,detail=3)
        p.beam((-.8,0,1.2),(-.35,-.2,1.7),.07)
    elif name.startswith("column_"):
        h=1.9 if name=="column_broken" else 4
        B((0,0,h/2),(.3,.42,h),2,True)
        for z in (.18,h-.18): B((0,0,z),(.52,.6,.36),1,True)
        for z in (.55,h-.55): B((0,-.24,z),(.32,.06,.16),3,detail=2)
        B((0,-.24,1.25),(.07,.04,.5),5,detail=1)
        if name=="column_broken": p.beam((0,0,h),(0,-.4,h+.65),.09)
    elif name.startswith("beam_"):
        width=2 if name=="beam_2m" else 4
        B((0,0,.12),(width,.38,.24),1,True)
        B((0,-.21,.12),(width-.3,.06,.08),3,detail=2)
    elif name=="railing_4m":
        for x in (-1.8,0,1.8): B((x,0,.6),(.12,.12,1.2),2,True)
        for z in (.45,1.2): B((0,0,z),(4,.12,.12),3,True)
    elif name=="barricade_2m":
        B((0,0,.6),(2,.65,1.2),1,True,bevel=.1)
        B((0,-.35,.8),(1.8,.12,.65),0,detail=2)
        B((0,-.42,.32),(1.8,.04,.12),3,detail=2)
        for x in (-.8,.8): B((x,0,.12),(.35,1.0,.24),2,True)
    elif name=="cargo_container":
        B((0,0,.8),(2.8,1.5,1.6),1,True,bevel=.12)
        B((0,0,1.62),(2.9,1.55,.25),0,detail=3,bevel=.1)
        for x in (-1.15,1.15):
            B((x,-.79,.8),(.18,.12,1.2),5,detail=2)
            B((x,-.83,1.25),(.32,.18,.18),3,detail=1)
        for z in (.4,.65,.9,1.15): B((0,-.8,z),(1.6,.08,.06),2,detail=1)
    elif name=="oxygen_rack":
        B((0,0,.15),(1.8,.8,.3),1,True)
        B((0,.3,1.1),(1.8,.16,2.2),2,True)
        for x in (-.55,0,.55):
            C((x,0,1.1),.22,1.7,0)
            C((x,0,2.03),.085,.18,3,detail=2)
            B((x,-.23,1.3),(.12,.06,.5),5,detail=2)
        # Stable Box approximation to the tank silhouettes, unchanged across LODs.
        if lod==0:
            B((0,0,1.1),(1.6,.55,2),6,True,detail=0)
            obj=p.objects.pop(); bpy.data.objects.remove(obj,do_unlink=True)
    elif name in ("generator","hvac_unit"):
        h=1.35 if name=="generator" else .7
        B((0,0,h/2),(2,1.35,h),1,True,bevel=.08)
        B((0,0,h+.08),(2.1,1.45,.16),0,detail=3)
        B((0,-.72,h*.6),(1.5,.1,h*.6),6,detail=2)
        for x in (-.55,-.28,0,.28,.55): B((x,-.8,h*.6),(.08,.08,h*.55),2,detail=1)
        B((-.8,-.74,h*.5),(.12,.08,.4),5,detail=2)
        if name=="generator": C((.75,.1,h+.25),.13,.6,2,detail=2)
    elif name=="console":
        B((0,0,.6),(1.2,.7,1.2),1,True)
        B((0,0,1.4),(1.35,.75,.55),0,True)
        B((0,-.405,1.42),(1.1,.055,.36),12,detail=3)
        for x in (-.4,0,.4): C((x,-.45,.9),.055,.08,3,axis="Y",detail=1)
    elif name=="workbench":
        B((0,0,.95),(2.5,1,.18),0,True)
        for x in (-1,1): B((x,0,.46),(.22,.8,.92),1,True)
        B((0,.4,1.35),(2.5,.16,.7),1,True)
        B((-.6,.28,1.4),(.85,.08,.5),12,detail=2)
        B((.65,-.05,1.15),(.55,.45,.3),2,detail=1)
    elif name=="purification_pod":
        B((0,0,.15),(1.65,1.65,.3),1,True,bevel=.1)
        B((0,.6,1.5),(1.5,.35,2.7),0,True,bevel=.1)
        for x in (-.65,.65):
            B((x,0,1.4),(.24,1.1,2.5),2,True)
            B((x,-.58,1.4),(.08,.05,1.9),5,detail=3)
        B((0,0,2.85),(1.65,1.65,.25),0,True,bevel=.1)
        B((0,.39,1.45),(.8,.07,1.8),12,detail=2)
        C((0,0,.4),.55,.15,2,detail=1)
    elif name=="relay_mast":
        B((0,0,.3),(1.6,1.6,.6),1,True)
        B((0,0,3.3),(.34,.34,6),2,True)
        for z in (1,3,5): B((0,0,z),(.5,.5,.18),3,detail=2)
        for x in (-1,1):
            B((x,0,5.4),(.4,.5,2.0),0,True)
            B((x,-.28,5.4),(.13,.06,1.6),5,detail=2)
        B((0,0,5),(2.4,.18,.18),2,detail=2)
        C((0,0,6.8),.12,1.1,3)
        for x in (-.7,.7): p.beam((x,0,.5),(0,0,2.8),.1)
    elif name=="solar_array":
        B((0,0,.3),(.5,1.8,.6),2,True)
        B((0,0,.9),(3.6,2,.15),8,True)
        for x in (-1.2,0,1.2): B((x,0,1),(.04,2,.03),2,detail=1)
    elif name in ("beacon","warning_beacon"):
        B((0,0,.12),(.5,.5,.24),1,True)
        B((0,0,.8),(.18,.18,1.4),2,True)
        C((0,0,1.6),.22,.4,5 if name=="beacon" else 4)
        C((0,0,1.85),.26,.1,2,detail=2)
    elif name.startswith("sign_"):
        B((0,0,0),(2.2,.16,.72),2,True)
        B((0,-.1,0),(2.08,.04,.6),{"sign_cargo":9,"sign_medical":11,"sign_relay":14}[name],detail=3)
    elif name=="debris_panel":
        meshutil.prism(p,[(-.8,-.5),(.5,-.6),(.95,-.1),(.6,.4),(0,.6),(-.9,.25)],0,.16,0)
        B((0,0,.06),(1.2,.7,.12),0,True,bevel=0)
        p.beam((-.5,-.2,.2),(.6,.2,.2),.07)
    elif name=="debris_beam":
        B((0,0,.12),(2.5,.24,.24),2,True)
        B((0,-.14,.12),(1.5,.05,.1),3,detail=1)
    elif name in ("pipe_straight","pipe_elbow"):
        C((0,0,.3),.18,2,2,axis="X")
        for x in (-.8,.8): C((x,0,.3),.25,.12,3,axis="X",detail=2)
        if lod==0:
            B((0,0,.3),(2,.36,.36),2,True,detail=0)
            obj=p.objects.pop(); bpy.data.objects.remove(obj,do_unlink=True)
        if name=="pipe_elbow": C((.85,.65,.3),.18,1.3,2,axis="Y")
    # Small fastening geometry disappears first. LOD1 retains the structural chamfers.
    anchors=attach_fasteners(p) if lod==0 and p.objects else []
    bpy.ops.object.select_all(action="DESELECT")
    for obj in p.objects: obj.select_set(True)
    bpy.context.view_layer.objects.active=p.objects[0]
    if len(p.objects)>1: bpy.ops.object.join()
    obj=bpy.context.object; obj.name=f"L02_{name}_LOD{lod}"
    # Collection assets and prefab copies share a canonical unrotated local frame.
    # This also keeps pipe-cylinder rotations from being overwritten by prefab yaw.
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    bpy.context.scene.cursor.location=(0,0,0); bpy.ops.object.origin_set(type="ORIGIN_CURSOR")
    obj["part_type"]=name; obj["lod"]=lod; obj["anchor"]=name in ANCHORS
    if anchors: obj["fastener_anchors"]=json.dumps(anchors)
    col=collect(obj.name); col.objects.link(obj)
    for old in list(obj.users_collection):
        if old!=col: old.objects.unlink(obj)
    if lod==0:
        col.asset_mark(); col.asset_data.description=f"L02 / {name} / 4m grid / independent closed shells"
        COLLISIONS[name]=p.colliders
    MESHES[(name,lod)]=(obj,col)
    return obj


def obj_export(path,objects):
    path.parent.mkdir(parents=True,exist_ok=True)
    with path.open("w",encoding="ascii",newline="\n") as out:
        mtl=Path(os.path.relpath(OUT/"obj/L02_Atlas.mtl",path.parent)).as_posix()
        out.write(f"# L02 RH Y-up; UV V preflipped for DirectX.\nmtllib {mtl}\nusemtl L02_Atlas\n")
        offset=1
        for obj in objects:
            data=obj.data; data.calc_loop_triangles(); uv=data.uv_layers.active.data
            matrix=obj.matrix_world; normals=matrix.to_3x3().inverted().transposed()
            verts=[]; lookup={}; tris=[]
            for tri in data.loop_triangles:
                n=(normals @ data.polygons[tri.polygon_index].normal).normalized()
                ids=[]
                for loop in tri.loops:
                    v=matrix @ data.vertices[data.loops[loop].vertex_index].co; t=uv[loop].uv
                    key=tuple(round(a,6) for a in (v.x,v.z,-v.y,t.x,1-t.y,n.x,n.z,-n.y))
                    if key not in lookup: lookup[key]=offset+len(verts); verts.append(key)
                    ids.append(lookup[key])
                tris.append(ids)
            out.write(f"o {obj.name}\n")
            for v in verts: out.write("v "+" ".join(map(str,v[:3]))+"\n")
            for v in verts: out.write(f"vt {v[3]} {v[4]}\n")
            for v in verts: out.write("vn "+" ".join(map(str,v[5:]))+"\n")
            for t in tris: out.write("f "+" ".join(f"{i}/{i}/{i}" for i in t)+"\n")
            offset+=len(verts)


def build_library():
    (OUT/"obj").mkdir(parents=True,exist_ok=True)
    (OUT/"obj/L02_Atlas.mtl").write_text("newmtl L02_Atlas\nKd 1 1 1\nmap_Kd ../textures/L02_BaseColor.png\n",encoding="ascii")
    for name in TYPES:
        stats=[]
        for lod in range(4):
            obj=make_part(name,lod)
            path=f"obj/parts/{name}_LOD{lod}.obj"; obj_export(OUT/path,[obj])
            obj.data.calc_loop_triangles(); stats.append(len(obj.data.loop_triangles))
        CATALOG["parts"][name]={"lodMeshes":[f"obj/parts/{name}_LOD{i}.obj" for i in range(4)],
            "triangles":stats,"anchor":name in ANCHORS,"colliders":COLLISIONS[name],
            "fractureMeshes":[f"wall_shard_{c}" for c in "abcd"] if name=="wall_solid_2m" else []}
    recipes=load_recipes()
    prefab_cols={}
    for name,recipe in recipes.items():
        if not re.fullmatch(r"[A-Za-z][A-Za-z0-9_]*",name) or name in TYPES:
            raise ValueError(f"Use a unique English prefab name: {name}")
        counts=[]; manifest_nodes=[]
        for lod in range(4):
            col=collect(f"L02_{name}_LOD{lod}"); prefab_cols[(name,lod)]=col
            for index,n in enumerate(recipe["nodes"]):
                if n["part"] not in TYPES or n["yaw"]%90:
                    raise ValueError(f"Invalid snapped prefab node: {name} {n}")
                source=MESHES[(n["part"],lod)][0]
                obj=source.copy(); obj.data=source.data; obj.name=f"{index:03}_{n['part']}"
                obj.location=n["position"]; obj.rotation_euler=(0,0,math.radians(n["yaw"]))
                col.objects.link(obj); obj["node_id"]=f"{name}/{index:03}"; obj["support_role"]=n["support"]
                if lod==0:
                    manifest_nodes.append({"id":f"{index:03}","part":n["part"],
                        "positionM":meshutil.engine(n["position"]),"yawDeg":-n["yaw"],
                        "supportRole":n["support"],"destructible":n["part"] not in ANCHORS})
            bpy.context.view_layer.update()
            # Unlinked asset collections still need explicit object matrix updates.
            for obj in col.objects: obj.matrix_world=Matrix.Translation(obj.location) @ obj.rotation_euler.to_matrix().to_4x4()
            obj_export(OUT/f"obj/prefabs/{name}_LOD{lod}.obj",list(col.objects))
            counts.append(sum(len(o.data.loop_triangles) for o in col.objects))
            if lod==0:
                col.asset_mark(); col.asset_data.description=f"L02 prefab / {recipe['role']} / editable nodes"
        # Acyclic structural graph: ground -> column -> beam -> roof. Walls and
        # equipment attach to that graph, so future destruction can detach supports.
        for n in manifest_nodes:
            role=n["supportRole"]; part=n["part"]
            if role in ("anchor","loose"): candidates=[]
            else:
                prefix="floor_"
                if part.startswith("beam_") or (role=="frame" and not part.startswith("roof_")): prefix="column_"
                if part.startswith("roof_"): prefix="beam_"
                if role=="roof": prefix="roof_"
                candidates=[p for p in manifest_nodes if p["part"].startswith(prefix) and p["id"]!=n["id"]]
                if not candidates and prefix=="beam_": candidates=[p for p in manifest_nodes if p["part"].startswith("column_")]
            candidates.sort(key=lambda p:sum((a-b)**2 for a,b in zip(n["positionM"],p["positionM"])))
            n["supportNodeIds"]=[p["id"] for p in candidates[:2]]
        CATALOG["prefabs"][name]={"lodMeshes":[f"obj/prefabs/{name}_LOD{i}.obj" for i in range(4)],
            "triangles":counts,"role":recipe["role"],"nodes":manifest_nodes}
    (OUT/"catalog.json").write_text(json.dumps(CATALOG,indent=2)+"\n",encoding="utf-8")
    return prefab_cols


def terrain():
    config=json.loads((ROOT/"Assets/Terrain/lunar_craters.json").read_text())
    raw=np.fromfile(ROOT/"Assets/Terrain/lunar_craters_r16.raw",dtype="<u2").reshape(config["length"],config["width"])
    h=raw.astype(float)/65535*config["height_range_m"]+config["height_min_m"]
    n=config["width"]; step=config["cell_spacing_m"]
    vertices=[((x-(n-1)/2)*step,(y-(n-1)/2)*step,float(h[y,x])) for y in range(n) for x in range(n)]
    faces=[]
    # Render reference uses the same diagonal as the engine terrain mesh.
    for y in range(n-1):
        for x in range(n-1):
            a=y*n+x; faces.extend(((a,a+1,a+n),(a+1,a+n+1,a+n)))
    data=bpy.data.meshes.new("R16_513x513_exact"); data.from_pydata(vertices,[],faces); data.update()
    obj=bpy.data.objects.new("REFERENCE_LunarTerrain_384m_R16",data); bpy.context.scene.collection.objects.link(obj)
    obj["export_ignore"]=True; obj["raw_source"]="Assets/Terrain/lunar_craters_r16.raw"
    # Existing terrain maps in Blender. Slope-based mix is a visual reference, engine terrain shader unchanged.
    mat=bpy.data.materials.new("Existing_Lunar_Dust_Gravel_Rock"); mat.use_nodes=True
    nodes,links=mat.node_tree.nodes,mat.node_tree.links; shader=nodes.get("Principled BSDF")
    texcoord=nodes.new("ShaderNodeTexCoord"); scale=nodes.new("ShaderNodeVectorMath"); scale.operation="SCALE"; scale.inputs[3].default_value=.18
    links.new(texcoord.outputs["Object"],scale.inputs[0])
    colors=[]
    for name in ("dust","gravel","rock"):
        image=bpy.data.images.load(str(ROOT/f"Assets/Textures/Terrain/lunar_{name}_basecolor.png")); image.pack()
        tex=nodes.new("ShaderNodeTexImage"); tex.image=image; tex.projection="BOX"; tex.projection_blend=.2
        links.new(scale.outputs["Vector"],tex.inputs["Vector"]); colors.append(tex)
    geo=nodes.new("ShaderNodeNewGeometry"); sep=nodes.new("ShaderNodeSeparateXYZ"); links.new(geo.outputs["Normal"],sep.inputs[0])
    ramp=nodes.new("ShaderNodeMapRange"); ramp.inputs["From Min"].default_value=.72; ramp.inputs["From Max"].default_value=.98
    links.new(sep.outputs["Z"],ramp.inputs["Value"])
    mix=nodes.new("ShaderNodeMixRGB"); links.new(ramp.outputs["Result"],mix.inputs[0]); links.new(colors[2].outputs[0],mix.inputs[1]); links.new(colors[0].outputs[0],mix.inputs[2])
    noise=nodes.new("ShaderNodeTexNoise"); noise.inputs["Scale"].default_value=.8; links.new(scale.outputs[0],noise.inputs[0])
    gravel=nodes.new("ShaderNodeMixRGB"); links.new(noise.outputs["Fac"],gravel.inputs[0]); links.new(mix.outputs[0],gravel.inputs[1]); links.new(colors[1].outputs[0],gravel.inputs[2])
    links.new(gravel.outputs[0],shader.inputs["Base Color"]); shader.inputs["Roughness"].default_value=.95
    obj.data.materials.append(mat)
    return h


def sample(h,x,y):
    fx=(x+192)/.75; fy=(y+192)/.75
    ix,iy=int(fx),int(fy); u,v=fx-ix,fy-iy
    a,b,c,d=h[iy,ix],h[iy,ix+1],h[iy+1,ix],h[iy+1,ix+1]
    return float((a*(1-u)+b*u)*(1-v)+(c*(1-u)+d*u)*v)


def add_instance(collection,name,location,yaw=0,parent=None):
    obj=bpy.data.objects.new(name,None); obj.instance_type="COLLECTION"; obj.instance_collection=collection
    obj.location=location; obj.rotation_euler.z=math.radians(yaw)
    (parent or bpy.context.scene.collection).objects.link(obj)
    obj.empty_display_size=.7
    return obj


def stage():
    scene=bpy.context.scene; scene.render.engine="CYCLES"; scene.cycles.samples=24; scene.cycles.use_denoising=True
    scene.render.resolution_x=1500; scene.render.resolution_y=1000; scene.render.resolution_percentage=100
    if scene.world is None: scene.world=bpy.data.worlds.new("L02_Review_World")
    scene.world.color=(.16,.16,.16); scene.view_settings.view_transform="AgX"
    data=bpy.data.lights.new("Moon_Sun","SUN"); data.energy=3; data.angle=.08
    sun=bpy.data.objects.new("Moon_Sun",data); scene.collection.objects.link(sun); sun.rotation_euler=(.5,-.4,-.5)
    light=bpy.data.lights.new("Review_Fill","AREA"); light.energy=8500; light.size=24
    obj=bpy.data.objects.new("Review_Fill",light); obj.location=(12,-20,28); scene.collection.objects.link(obj); point_at(obj,(0,0,5))
    camera=bpy.data.objects.new("L02_Review_Camera",bpy.data.cameras.new("L02_Review_Camera")); scene.collection.objects.link(camera)
    camera.data.type="ORTHO"; camera.data.ortho_scale=40; scene.camera=camera
    camera.location=(31,-40,36); camera.data.ortho_scale=50; point_at(camera,(0,1,6.5))
    for screen in bpy.data.screens:
        for area in screen.areas:
            if area.type=="VIEW_3D":
                area.spaces.active.region_3d.view_distance=45; area.spaces.active.region_3d.view_location=(0,0,5)
                area.spaces.active.shading.type="MATERIAL"
    return camera


def point_at(obj,target):
    obj.rotation_euler=(Vector(target)-obj.location).to_track_quat("-Z","Y").to_euler()


def crate_reference():
    mat=bpy.data.materials.new("REFERENCE_Existing_S07_Crate"); mat.use_nodes=True
    tex=mat.node_tree.nodes.new("ShaderNodeTexImage")
    tex.image=bpy.data.images.load(str(OUT/"textures/REFERENCE_S07_BaseColor.png")); tex.image.pack()
    shader=mat.node_tree.nodes.get("Principled BSDF"); shader.inputs["Roughness"].default_value=.58
    mat.node_tree.links.new(tex.outputs["Color"],shader.inputs["Base Color"])
    for part,z in (("Body",0),("Lid",.64)):
        bpy.ops.wm.obj_import(filepath=str(ROOT/f"Assets/ItemBox/obj/S07_{part}_LOD0.obj"),forward_axis="NEGATIVE_Z",up_axis="Y")
        for obj in bpy.context.selected_objects:
            obj.location=(4,-1.8,4.68+z); obj.name=f"REFERENCE_S07_{part}"
            obj.data.materials.clear(); obj.data.materials.append(mat); obj["export_ignore"]=True
            for layer in obj.data.uv_layers:
                for uv in layer.data: uv.uv.y=1-uv.uv.y


def collider_reference():
    layout=json.loads((OUT/"layout.json").read_text())
    col=collect("L02_COLLIDERS_Reference_Toggle_Viewport"); bpy.context.scene.collection.children.link(col)
    col.hide_render=True
    for index,box in enumerate(layout["boxes"]):
        c=box["centerM"]; h=box["halfExtentsM"]
        bpy.ops.mesh.primitive_cube_add(size=1,location=(c[0],c[2],c[1]+4.71))
        obj=bpy.context.object; obj.dimensions=(h[0]*2,h[2]*2,h[1]*2)
        obj.name=f"COL_{10001+index}_{box['name']}"; obj.display_type="WIRE"; obj.color=(.1,1,.4,1)
        obj["collider_id"]=10001+index; obj["export_ignore"]=True
        for old in list(obj.users_collection): old.objects.unlink(obj)
        col.objects.link(obj)
    col.hide_viewport=True


def review_scenes(cols,render):
    layout_scene=bpy.context.scene
    for kind in ("Parts","LOD"):
        scene=bpy.data.scenes.new(f"L02_Review_{kind}"); bpy.context.window.scene=scene
        camera=stage()
        if kind=="Parts":
            for i,name in enumerate(TYPES):
                x=(i%8-3.5)*6; y=(i//8)*8
                add_instance(MESHES[(name,0)][1],name,(x,y,0))
                text=bpy.data.curves.new(name,"FONT"); text.body=name; text.size=.34; text.align_x="CENTER"
                label=bpy.data.objects.new("LABEL_"+name,text); scene.collection.objects.link(label); label.location=(x,y-2.4,.02)
            camera.location=(30,-45,65); point_at(camera,(0,15,1)); camera.data.ortho_scale=62
            scene.render.resolution_x=1900; scene.render.resolution_y=1500
        else:
            for lod in range(4):
                x=(lod-1.5)*17
                add_instance(cols[("CargoDepot_Ruined",lod)],f"LOD{lod}",(x,0,0))
                text=bpy.data.curves.new(f"LOD{lod}","FONT"); text.body=f"LOD{lod} / {CATALOG['prefabs']['CargoDepot_Ruined']['triangles'][lod]:,} tris"; text.size=.7; text.align_x="CENTER"
                label=bpy.data.objects.new(f"LABEL_LOD{lod}",text); scene.collection.objects.link(label); label.location=(x,-7,.02)
            camera.location=(15,-45,35); point_at(camera,(0,0,1)); camera.data.ortho_scale=75
            scene.render.resolution_x=2200; scene.render.resolution_y=800
        ground=bpy.data.materials.get("ReviewGround") or bpy.data.materials.new("ReviewGround")
        ground.diffuse_color=(.12,.14,.16,1)
        bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.23)); bpy.context.object.data.materials.append(ground)
        if render:
            scene.render.filepath=str(OUT/f"previews/L02_{kind}.png"); bpy.ops.render.render(write_still=True)
    bpy.context.window.scene=layout_scene


def main():
    parser=argparse.ArgumentParser(); parser.add_argument("--render",action="store_true")
    parser.add_argument("--reset-layout",action="store_true",help="Explicitly replace saved placements with the demo layout")
    args=parser.parse_args(sys.argv[sys.argv.index("--")+1:] if "--" in sys.argv else [])
    bpy.ops.object.select_all(action="SELECT"); bpy.ops.object.delete(use_global=False)
    bpy.context.scene.name="L02_Layout"
    bpy.context.scene.unit_settings.system="METRIC"; bpy.context.preferences.filepaths.save_version=0
    material_setup(); cols=build_library(); h=terrain()
    layout=collect("L02_LAYOUT_Export_Instances"); bpy.context.scene.collection.children.link(layout)
    placements=[("CargoDepot_Ruined",6,0,0),("PurificationStation",-6,0,0),
                ("RelayPlatform",5,9,0),("ServiceCorridor",-4,8,0),
                ("CoverCheckpoint",0,-10,0),("WreckWorkshop",-12,-8,90),
                ("CargoDepot_Intact",-56,48,180)]
    for i,(name,x,y,yaw) in enumerate(placements):
        # Floor contact is 5mm above the sampled center on the landing plateau;
        # the existing capsule solver clears this lip while the floor stays visible.
        ground=sample(h,x,y)+.005
        obj=add_instance(cols[(name,0)],f"L02_{i:02}_{name}",(x,y,ground),yaw,layout)
        obj["prefab_id"]=name; obj["placement_id"]=f"L02_{i:02}"
    # Loose modular objects are exported through the same instance path.
    for i,(name,x,y) in enumerate((("solar_array",10,9),("pipe_straight",-14,15),("pipe_elbow",-12,15))):
        obj=add_instance(MESHES[(name,0)][1],f"L02_loose_{i}_{name}",(x,y,sample(h,x,y)))
        # Link to export layout, not root.
        bpy.context.scene.collection.objects.unlink(obj); layout.objects.link(obj)
        obj["part_id"]=name; obj["placement_id"]=f"L02_prop_{i}"
    if (OUT/"layout.json").exists() and not args.reset_layout:
        saved=json.loads((OUT/"layout.json").read_text())
        for obj in list(layout.objects): bpy.data.objects.remove(obj,do_unlink=True)
        for entry in saved["instances"]:
            name=entry["asset"]; pos=entry["positionM"]
            col=cols[(name,0)] if entry["type"]=="prefabs" else MESHES[(name,0)][1]
            obj=add_instance(col,entry["id"],(pos[0],pos[2],pos[1]),-entry["yawDeg"],layout)
            obj["prefab_id" if entry["type"]=="prefabs" else "part_id"]=name
            obj["placement_id"]=entry["id"]
            if entry.get("site"):
                obj["site_id"]=entry["site"]
    bpy.context.view_layer.update()
    from export_lunar_layout import export_layout
    export_layout(ROOT)
    crate_reference(); collider_reference(); camera=stage()
    (OUT/"source").mkdir(exist_ok=True); (OUT/"previews").mkdir(exist_ok=True)
    # Embedded editor instructions survive moving this .blend to another machine.
    text=bpy.data.texts.new("START_HERE_KO")
    text.write("L02 달 기지 키트\n미터 단위, 바닥 4m / 벽 2m, Z 위 방향.\n"
               "L02_LAYOUT_Export_Instances 컬렉션에서 인스턴스를 복제하고 이동하세요.\n"
               "회전은 Z축 90도 단위, 스케일 1.0 유지. Asset Browser: Current File에서 파츠/프리팹 사용.\n"
               "export_lunar_layout.py를 Text Editor에서 실행하면 엔진 배치+콜라이더를 함께 저장합니다.\n"
               "기존 상자 모델과 달 지형은 참조용입니다. 파괴 동기화/파편 물리는 후속 구현입니다.\n")
    exporter=bpy.data.texts.load(str(ROOT/"Tools/export_lunar_layout.py"))
    bpy.data.texts.load(str(ROOT/"Tools/save_lunar_prefab.py"))
    scene=bpy.context.scene; scene["lunar_project_root"]=str(ROOT)
    scene.tool_settings.use_snap=True; scene.tool_settings.snap_elements={"INCREMENT"}
    scene.tool_settings.use_snap_grid_absolute=True
    # Drop unused materials from the crate importer; all referenced images are packed.
    for mat in list(bpy.data.materials):
        if mat.users==0: bpy.data.materials.remove(mat)
    for image in list(bpy.data.images):
        if image.users==0: bpy.data.images.remove(image)
    review_scenes(cols,args.render)
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT/"source/L02_LunarKit_Layout.blend"))
    if args.render:
        scene.render.filepath=str(OUT/"previews/L02_Settlement.png"); bpy.ops.render.render(write_still=True)
        camera.location=(21,-17,17); camera.data.ortho_scale=23; point_at(camera,(6,0,6.6))
        scene.render.filepath=str(OUT/"previews/L02_CargoDepot.png"); bpy.ops.render.render(write_still=True)
        camera.location=(8,-18,18); camera.data.ortho_scale=19; point_at(camera,(-6,0,6.5))
        # Cutaway review reveals the medical equipment and the unobstructed corridor.
        medical=cols[("PurificationStation",0)]
        roofs=[o for o in medical.objects if o.get("part_type","").startswith("roof_")]
        for obj in roofs: obj.hide_render=True
        scene.render.filepath=str(OUT/"previews/L02_Purification.png"); bpy.ops.render.render(write_still=True)
        for obj in roofs: obj.hide_render=False
    print("L02_BUILD_COMPLETE",len(TYPES),"part types",len(cols)//4,"prefabs")


if __name__=="__main__": main()
