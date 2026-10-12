"""Blender Text Editor: select L02 part instances, place 3D cursor at the pivot,
set Scene['lunar_new_prefab_name'] (e.g. CustomHabitat), then Run Script.
Writes a reusable recipe; rebuild with build_lunar_kit.py to generate all 4 LODs.
Saved layout.json placements are preserved by the builder unless --reset-layout.
"""
import json
import math
from pathlib import Path
import re
import bpy


def save_prefab():
    scene=bpy.context.scene
    root=Path(scene.get("lunar_project_root","")); out=root/"Assets/LunarKit"
    catalog=json.loads((out/"catalog.json").read_text())
    name=scene.get("lunar_new_prefab_name","CustomHabitat")
    if not re.fullmatch(r"[A-Za-z][A-Za-z0-9_]*",name): raise ValueError("Use an English prefab name without spaces")
    path=out/"prefabs/recipes.json"; recipes=json.loads(path.read_text())
    if name in recipes: raise ValueError("Choose a new lunar_new_prefab_name; existing recipes are preserved")
    pivot=scene.cursor.location
    nodes=[]
    for obj in bpy.context.selected_objects:
        part=obj.get("part_id") or obj.get("part_type")
        if not part and obj.instance_type=="COLLECTION":
            collection=obj.instance_collection.name
            part=next((p for p in catalog["parts"] if collection==f"L02_{p}_LOD0"),None)
        if part not in catalog["parts"]: raise ValueError(f"Select individual L02 parts: {obj.name}")
        pos,rotation,scale=obj.matrix_world.decompose(); euler=rotation.to_euler()
        if max(abs(v-1) for v in scale)>1e-5 or abs(euler.x)+abs(euler.y)>1e-5:
            raise ValueError("Keep scale 1 and rotate only around Z")
        yaw=math.degrees(euler.z)
        if abs(yaw/90-round(yaw/90))>1e-5: raise ValueError("Use 90 degree yaw increments")
        nodes.append({"part":part,"position":[round(v,6) for v in pos-pivot],
                      "yaw":round(yaw/90)*90,"support":obj.get("support_role","foundation")})
    if not nodes: raise ValueError("Select the parts of the new prefab")
    recipes[name]={"role":"custom Blender assembly","nodes":nodes}
    path.write_text(json.dumps(recipes,indent=2)+"\n",encoding="utf-8")
    print(f"Saved {name}: {len(nodes)} nodes. Rebuild the kit in a fresh background Blender process.")


if __name__=="__main__": save_prefab()
