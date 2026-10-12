"""Run in Blender Text Editor after moving instances in L02_LAYOUT_Export_Instances.

Exports render placements AND existing Box colliders atomically after validating all
instances. Positive Blender Z yaw becomes negative engine Y yaw. No terrain export.
To add a custom prefab: duplicate a recipe in prefabs/recipes.json and rebuild the kit.
"""
import json
import math
from pathlib import Path
import os
import tempfile
import bpy
from mathutils import Matrix, Vector


def export_layout(root=None):
    root=Path(root or bpy.context.scene.get("lunar_project_root", ""))
    if not (root/"Shared/World/TestMapSettings.h").exists():
        raise ValueError("Set Scene custom property lunar_project_root to the repository path")
    out=root/"Assets/LunarKit"
    catalog=json.loads((out/"catalog.json").read_text(encoding="utf-8"))
    collection=bpy.data.collections.get("L02_LAYOUT_Export_Instances")
    if not collection: raise ValueError("Missing L02_LAYOUT_Export_Instances")
    records=[]; boxes=[]; lines=["lunar_layout 2"]
    identifiers=set()
    for obj in sorted(collection.objects,key=lambda o:o.name):
        if obj.get("export_ignore"): continue
        prefab=obj.get("prefab_id"); part=obj.get("part_id")
        if not prefab and not part and obj.instance_type=="COLLECTION":
            # Dragging from Blender's Asset Browser does not copy custom properties
            # onto the new instance object. Resolve its collection identity instead.
            collection_name=obj.instance_collection.name
            prefab=next((key for key in catalog["prefabs"] if collection_name==f"L02_{key}_LOD0"),None)
            part=next((key for key in catalog["parts"] if collection_name==f"L02_{key}_LOD0"),None)
        if bool(prefab)==bool(part) or obj.instance_type!="COLLECTION":
            raise ValueError(f"{obj.name}: use an L02 collection instance with prefab_id or part_id")
        key=prefab or part; section="prefabs" if prefab else "parts"
        if key not in catalog[section]: raise ValueError(f"Unknown asset {key}")
        if obj.instance_collection.name!=f"L02_{key}_LOD0":
            raise ValueError(f"{obj.name}: asset identity differs from the displayed LOD0 collection")
        identity=obj.get("placement_id",obj.name)
        if identity in identifiers:
            # Shift+D copies custom properties. The object name is unique and stable after save.
            identity=obj.name
        if any(c.isspace() for c in identity) or identity in identifiers:
            raise ValueError(f"Unique placement_id without spaces required: {obj.name}")
        identifiers.add(identity)
        pos,rotation,scale=obj.matrix_world.decompose(); euler=rotation.to_euler("XYZ")
        if max(abs(v-1) for v in scale)>1e-5 or max(abs(euler.x),abs(euler.y))>1e-5:
            raise ValueError(f"{obj.name}: scale must be 1, rotation only around Blender Z")
        yaw=math.degrees(euler.z)
        if abs(yaw-round(yaw/90)*90)>1e-4:
            raise ValueError(f"{obj.name}: existing Box colliders require 90 degree yaw increments")
        record={"id":identity,"asset":key,"type":section,"positionM":[pos.x,pos.z,pos.y],"yawDeg":-yaw,"nodes":[]}
        prefix=f"Assets/LunarKit/obj/{section}/{key}"
        lines.append("instance "+identity+" "+prefix+" "+" ".join(f"{v:.6f}" for v in record["positionM"]+[-yaw]))
        nodes=catalog["prefabs"][key]["nodes"] if prefab else [{"id":"000","part":key,"positionM":[0,0,0],"yawDeg":0,"supportRole":"loose"}]
        for node in nodes:
            engine=node["positionM"]
            matrix=obj.matrix_world @ Matrix.Translation((engine[0],engine[2],engine[1])) @ Matrix.Rotation(math.radians(-node["yawDeg"]),4,"Z")
            collider_indices=[]
            for i,collider in enumerate(catalog["parts"][node["part"]]["colliders"]):
                c=collider["centerM"]; half=collider["halfExtentsM"]
                world=matrix @ Vector((c[0],c[2],c[1]))
                world_half=matrix.to_3x3()
                h=[sum(abs(world_half[r][j])*v for j,v in enumerate((half[0],half[2],half[1]))) for r in range(3)]
                name=f"{identity}_{node['id']}_c{i:02}"
                index=len(boxes); collider_indices.append(index)
                boxes.append({"name":name,"centerM":[world.x,world.z-4.71,world.y],
                              "halfExtentsM":[h[0],h[2],h[1]],"placement":identity,"node":node["id"]})
            record["nodes"].append({"id":node["id"],"part":node["part"],"supportRole":node["supportRole"],
                                     "supportNodeIds":node.get("supportNodeIds",[]),
                                     "colliderFileIndices":collider_indices})
        records.append(record)
    if not records: raise ValueError("Layout is empty")
    collision=["# L02 existing Box. Local to TestMapSettings::HOUSE_POSITION_M=(0,4.71,0).",
               "# IDs = FIRST_HOUSE_COLLIDER_ID + file order; see layout.json node mapping."]
    for box in boxes:
        collision.append("box "+box["name"]+" "+" ".join(f"{v:.6f}" for v in box["centerM"]+box["halfExtentsM"])+" 0 0 0")
    contents={out/"layout.txt":"\n".join(lines)+"\n",
              out/"layout.json":json.dumps({"schemaVersion":2,"instances":records,"boxes":boxes},indent=2)+"\n",
              root/"Shared/Geometry/L02_LunarKit_collision.txt":"\n".join(collision)+"\n"}
    # Validate everything before touching outputs; replace each completed temporary file.
    for path,content in contents.items():
        path.parent.mkdir(parents=True,exist_ok=True)
        with tempfile.NamedTemporaryFile(mode="w",encoding="utf-8",dir=path.parent,delete=False,newline="\n") as f:
            f.write(content); temporary=f.name
        os.replace(temporary,path)
    print(f"L02 layout exported: {len(records)} instances, {len(boxes)} existing Box colliders")


if __name__=="__main__": export_layout()
