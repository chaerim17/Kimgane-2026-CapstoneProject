"""Headless authoring QA: Blender source, LOD catalog, terrain and placement exporter.

blender --background Assets/LunarKit/source/L02_LunarKit_Layout.blend --python-exit-code 1 --python Tools/validate_lunar_kit.py
Native mesh/DDS/character tests are in Client/Tests/LunarKitTests.vcxproj.
"""
import json
import math
from pathlib import Path
import shutil
import sys
import tempfile

import bpy
import bmesh
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/"Tools"))
from export_lunar_layout import export_layout


def require(value,message):
    if not value: raise AssertionError(message)


def main():
    out=ROOT/"Assets/LunarKit"
    catalog=json.loads((out/"catalog.json").read_text())
    for section in ("parts","prefabs"):
        for name,record in catalog[section].items():
            require(len(record["lodMeshes"])==4,f"Missing LOD: {name}")
            require(record["triangles"][0]>record["triangles"][3],f"LOD did not reduce {name}")
            require(all(a>=b for a,b in zip(record["triangles"],record["triangles"][1:])),f"Non-monotonic LOD: {name}")
            for lod,path in enumerate(record["lodMeshes"]):
                count=sum(line.startswith("f ") for line in (out/path).read_text().splitlines())
                require(count==record["triangles"][lod],f"Manifest triangle count: {path}")
            if section=="parts":
                for lod in range(4):
                    obj=bpy.data.objects[f"L02_{name}_LOD{lod}"]
                    bm=bmesh.new(); bm.from_mesh(obj.data)
                    require(all(e.is_manifold for e in bm.edges),f"Open/nonmanifold shell: {obj.name}")
                    require(bm.calc_volume()>0,f"Inverted shell: {obj.name}"); bm.free()
            else:
                nodes={n["id"]:n for n in record["nodes"]}
                def visit(identity,stack):
                    require(identity not in stack,f"Cyclic supports: {name}")
                    for support in nodes[identity].get("supportNodeIds",[]): visit(support,stack|{identity})
                for identity in nodes: visit(identity,set())
    used={m.name for obj in bpy.data.objects if obj.type=="MESH" for m in obj.data.materials if m}
    for material in bpy.data.materials:
        if material.name not in used or not material.use_nodes: continue
        for node in material.node_tree.nodes:
            if node.type=="TEX_IMAGE" and node.image:
                require(node.image.packed_file is not None,f"External texture: {node.image.name}")
    terrain=bpy.data.objects["REFERENCE_LunarTerrain_384m_R16"]
    require(len(terrain.data.vertices)==513*513,"Terrain resolution mismatch")
    import struct
    raw=struct.unpack("<263169H",(ROOT/"Assets/Terrain/lunar_craters_r16.raw").read_bytes())
    for index in (0,256,131584,263168):
        require(abs(terrain.data.vertices[index].co.z-(raw[index]/65535*48-16))<3e-6,"Terrain height mismatch")
    layout=bpy.data.collections["L02_LAYOUT_Export_Instances"]
    with tempfile.TemporaryDirectory(prefix="lunar-export-test-") as temporary:
        test=Path(temporary)
        (test/"Shared/World").mkdir(parents=True)
        (test/"Shared/World/TestMapSettings.h").touch()
        (test/"Assets/LunarKit").mkdir(parents=True)
        shutil.copyfile(out/"catalog.json",test/"Assets/LunarKit/catalog.json")
        export_layout(test)
        before=(test/"Assets/LunarKit/layout.txt").read_bytes()
        # Simulate Asset Browser drag with no custom id, plus a copied prefab ID.
        original=layout.objects[0]
        duplicate=original.copy(); layout.objects.link(duplicate)
        duplicate.location.x+=40
        bpy.context.view_layer.update(); export_layout(test)
        exported=json.loads((test/"Assets/LunarKit/layout.json").read_text())
        require(len(exported["instances"])==len(layout.objects),"Copied instance lost")
        bpy.data.objects.remove(duplicate,do_unlink=True)
        obj=bpy.data.objects.new("BrowserDrag",None); obj.instance_type="COLLECTION"
        obj.instance_collection=bpy.data.collections["L02_floor_4m_LOD0"]; layout.objects.link(obj)
        obj.location=(40,20,4.71); bpy.context.view_layer.update(); export_layout(test)
        require("BrowserDrag" in (test/"Assets/LunarKit/layout.txt").read_text(),"Asset Browser identity missing")
        obj.rotation_euler.z=math.radians(45); bpy.context.view_layer.update()
        unchanged=(test/"Assets/LunarKit/layout.txt").read_bytes()
        try: export_layout(test)
        except ValueError: pass
        else: raise AssertionError("Unsupported Box rotation accepted")
        require((test/"Assets/LunarKit/layout.txt").read_bytes()==unchanged,"Failed export modified outputs")
        obj.rotation_euler.z=math.pi/2; bpy.context.view_layer.update(); export_layout(test)
        obj.scale.x=2; bpy.context.view_layer.update()
        try: export_layout(test)
        except ValueError: pass
        else: raise AssertionError("Unsupported scaled collider accepted")
        bpy.data.objects.remove(obj,do_unlink=True)
    print(f"L02_AUTHORING_QA_PASSED: {len(catalog['parts'])} closed part types x4 LOD, {len(catalog['prefabs'])} prefabs, packed maps, RAW alignment, exporter")


if __name__=="__main__": main()
