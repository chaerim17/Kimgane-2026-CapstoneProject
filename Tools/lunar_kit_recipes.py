"""Editable L02 assembly recipes. Positions are Blender meters: X, depth Y, up Z.

No bpy dependency: shared by build, validation, and custom prefab authoring.
Each node remains an individually addressable destruction unit in the manifest.
"""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "Assets/LunarKit"


def node(part, x=0, y=0, z=0, yaw=0, support="foundation"):
    return {"part": part, "position": [x, y, z], "yaw": yaw, "support": support}


def room(nx, ny, ruined=False, medical=False):
    nodes = []
    for x in range(nx):
        for y in range(ny):
            nodes.append(node("floor_4m", (x-(nx-1)/2)*4, (y-(ny-1)/2)*4, support="anchor"))
            if not ruined or (x+y) % 3 != 0:
                nodes.append(node("roof_4m" if (x+y)%2 else "roof_skylight_4m",
                                  (x-(nx-1)/2)*4, (y-(ny-1)/2)*4, 4, support="frame"))
    for x in range(nx+1):
        for y in range(ny+1):
            if x in (0,nx) or y in (0,ny):
                nodes.append(node("column_4m", (x-nx/2)*4, (y-ny/2)*4, support="foundation"))
    for side in (-1,1):
        for x in range(nx):
            px, py = (x-(nx-1)/2)*4, side*ny*2
            door = x == nx//2
            if door:
                nodes.append(node("doorframe_4m", px, py, yaw=0 if side==-1 else 180, support="frame"))
            elif ruined and side==-1 and x==0:
                nodes.append(node("wall_breach_4m", px, py, support="frame"))
                nodes.append(node("debris_panel", px-.7, py-.9, yaw=90, support="loose"))
            else:
                # Independently removable 2m halves; horizontal seams align with crate panels.
                for dx in (-1,1):
                    nodes.append(node("wall_window_2m" if side==-1 else "wall_solid_2m",
                                      px+dx, py, support="frame"))
            nodes.append(node("beam_4m", px, py, 4, support="frame"))
        for y in range(ny):
            px, py = side*nx*2, (y-(ny-1)/2)*4
            if ruined and side==1 and y==0:
                nodes.append(node("wall_breach_4m", px, py, yaw=90, support="frame"))
            else:
                for dy in (-1,1):
                    nodes.append(node("wall_vent_2m" if y%2 else "wall_solid_2m", px, py+dy,
                                      yaw=90, support="frame"))
            nodes.append(node("beam_4m", px, py, 4, yaw=90, support="frame"))
    nodes.append(node("sign_medical" if medical else "sign_cargo", 2 if nx%2==0 else 0, -ny*2-.27, 3.15,
                      support="frame"))
    nodes.append(node("hvac_unit", -(nx-1)*2, (ny-1)*2, 4.22, support="roof"))
    nodes.append(node("oxygen_rack", -nx*2+1.1, ny*2-1, support="foundation"))
    nodes.append(node("console", nx*2-.75, ny*2-1.0, yaw=180, support="foundation"))
    if medical:
        nodes.append(node("purification_pod", -1.5, .4, support="foundation"))
        nodes.append(node("workbench", -2.6, -1.2, support="foundation"))
    else:
        nodes.append(node("cargo_container", -2.4, .5, support="foundation"))
        nodes.append(node("workbench", nx*2-1, 0, yaw=90, support="foundation"))
    return nodes


def defaults():
    return {
        "CargoDepot_Ruined": {"role": "salvage / multiple entrances / cover", "nodes": room(3,2,True)},
        "CargoDepot_Intact": {"role": "alternate assembly from the same kit", "nodes": room(2,2)},
        "PurificationStation": {"role": "purification facility visual placeholder", "nodes": room(2,2,False,True)},
        "ServiceCorridor": {"role": "4m wide open corridor / two way passage", "nodes": [
            node("floor_4m",0,y,support="anchor") for y in (-2,2)] + [
            node("wall_solid_2m",x,y,yaw=90) for x in (-2,2) for y in (-3,-1,1,3)] + [
            node("roof_skylight_4m",0,y,4,support="frame") for y in (-2,2)] + [
            node("column_4m",x,y) for x in (-2,2) for y in (-4,0,4)]},
        "RelayPlatform": {"role": "extraction landmark / accessible console", "nodes": [
            node("floor_4m",x,y,support="anchor") for x in (-2,2) for y in (-2,2)] + [
            node("relay_mast",0,1),node("console",0,-2),node("generator",2.5,1.6),
            node("railing_4m",-4,0,yaw=90),node("railing_4m",4,0,yaw=90),
            node("sign_relay",0,2.8,1.5),node("beacon",-3,-3),node("beacon",3,-3)]},
        "CoverCheckpoint": {"role": "PvPvE cover with a 3m central escape route", "nodes": [
            node("barricade_2m",x,0) for x in (-2.6,2.6)] + [
            node("cargo_container",-4,2),node("oxygen_rack",4,2),node("warning_beacon",4,-1)]},
        "WreckWorkshop": {"role": "damaged open maintenance frame / salvage", "nodes": [
            node("floor_4m",x,0,support="anchor") for x in (-2,2)] + [
            node("column_broken",-4,-2),node("column_4m",4,-2),node("column_4m",-4,2),
            node("column_broken",4,2),node("beam_4m",-2,2,4),node("wall_breach_4m",0,2),
            node("workbench",-2,0),node("generator",2,0),node("debris_beam",3,-3),
            node("debris_panel",-3,-3),node("wall_fragment",0,-2)]},
    }


def load_recipes():
    path = OUT / "prefabs/recipes.json"
    if not path.exists():
        path.parent.mkdir(parents=True,exist_ok=True)
        path.write_text(json.dumps(defaults(),indent=2)+"\n",encoding="utf-8")
    return json.loads(path.read_text(encoding="utf-8"))
