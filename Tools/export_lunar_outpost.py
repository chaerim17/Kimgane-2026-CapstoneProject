"""Kimgane DX12 export: existing converted text meshes, RAW16 terrain and box/ramp collision.
Blender world (x,y,z) -> project (-x,z+16,-y), metres. Run on the SELENE scene.
"""
import bpy, math, re, json, sys
from pathlib import Path
from collections import defaultdict
from mathutils import Vector
import numpy as np

ROOT=Path(__file__).resolve().parent
import argparse
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output-root', required=True, help='Project root receiving Assets/ and Shared/ files')
parser.add_argument('--save-blend', help='Optional adapted blend copy; the loaded file is otherwise unchanged on disk')
args=parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
OUT=Path(args.output_root).resolve()
MODELS=OUT/'Assets/Models/LunarOutpost'
MAP=OUT/'Shared/Maps/LunarOutpost'
for p in [MODELS,MAP]: p.mkdir(parents=True,exist_ok=True)

def dx(v): return (-v[0],v[2]+16,-v[1])
def direction(v): return (-v[0],v[2],-v[1])
def safe(s): return re.sub('[^A-Za-z0-9_]+','_',s).strip('_')
def fmt(v): return ' '.join(f'{x:.6f}' for x in v)

def export():
    sc=bpy.context.scene; dg=bpy.context.evaluated_depsgraph_get()
    terrain=bpy.data.objects['LUNAR_SURFACE_280x240m']
    # Match the existing RAW16 loader. Extend the 240m visual terrain to a square 280m grid.
    samples={(round(v.co.x),round(v.co.y)):v.co.z for v in terrain.data.vertices}
    raw=[]
    for iz in range(281):
        for ix in range(281):
            bx=140-ix; by=140-iz
            z=samples.get((bx,max(-120,min(120,by))))
            if z is None: raise RuntimeError('Missing terrain sample')
            raw.append(round((z+16)/64*65535))
    data=np.asarray(raw,dtype='<u2'); data.tofile(MAP/'lunar_height.raw')
    # Existing collision ramps are axis aligned. Replace diagonal access ribbons with
    # 6m-wide X-axis branches; keep a deck-level mouth and a gradual descent to the rim.
    ramps=[o for o in sc.objects if o.name.startswith('Crater floor access ramp')]
    for sign,o in zip([-1,1],ramps):
        end=sign*31; z=samples[(end,-46)]+.18
        pts=[(sign*4.1,-49,3.45),(sign*4.1,-43,3.45),(end,-43,z),(end,-49,z)]
        for v,p in zip(o.data.vertices,pts): v.co=p
        o.data.update()
    bpy.context.view_layer.update()
    dg=bpy.context.evaluated_depsgraph_get()
    groups=defaultdict(lambda: [[],[],[],{}])
    collision=[]; boxes=0; ramp_count=0
    colors={}
    for m in bpy.data.materials:
        if not m.use_nodes: continue
        p=m.node_tree.nodes.get('Principled BSDF')
        if not p: continue
        color=tuple(p.inputs['Base Color'].default_value)
        if m.name=='Regolith': color=(.28,.30,.34,1)
        if m.name=='Compacted dust': color=(.12,.135,.16,1)
        if m.name=='Lunar rock': color=(.22,.235,.26,1)
        colors[m.name]=color
    terrain_heights=data.astype(np.float64)*64/65535
    def addbox(name,lo,hi):
        nonlocal boxes
        if min(hi[i]-lo[i] for i in range(3))<.015: return
        collision.append('box '+safe(name)+' '+fmt([(a+b)/2 for a,b in zip(lo,hi)])+' '+fmt([(b-a)/2 for a,b in zip(lo,hi)])); boxes+=1
    keep_collision=('Station / service apron','pressure hull','Command hub / lower pressure vessel','Main entry / vestibule','Hub / transfer corridor','Causeway / deck segment','Causeway pier','tapered mast','landing plinth','stepped foundation','CARGO / container','Regolith shield / waist cover','ROVER / chassis','ROVER / crew cab','UTILITIES / pressure tank')
    for o in list(sc.objects):
        if o.type not in {'MESH','CURVE','FONT'}: continue
        cn={c.name for c in o.users_collection}
        if cn & {'09_BACKDROP','10_CAMERAS','08_LIGHTING','01_TERRAIN'}: continue
        if o.name.startswith('Scale figure'): continue
        eo=o.evaluated_get(dg); me=eo.to_mesh(); me.calc_loop_triangles()
        normal_matrix=o.matrix_world.to_3x3().inverted().transposed()
        points=[dx(o.matrix_world@v.co) for v in me.vertices]
        for tri in me.loop_triangles:
            mat=me.materials[tri.material_index] if me.materials else None
            name=mat.name if mat else 'Titanium'; g=groups[name]; col=colors.get(name,(.5,.5,.5,1))
            for li in tri.loops:
                loop=me.loops[li]; p=points[loop.vertex_index]; nn=normal_matrix@me.corner_normals[li].vector; nn.normalize(); n=direction(nn)
                key=tuple(round(x,5) for x in (*p,*n,*col))
                idx=g[3].get(key)
                if idx is None: idx=len(g[0]); g[3][key]=idx; g[0].append(p); g[1].append(n)
                g[2].append(idx)
        low=[min(p[i] for p in points) for i in range(3)]; high=[max(p[i] for p in points) for i in range(3)]
        if 'ramp' in o.name.lower() and len(o.data.polygons)==1:
            # Project already has cardinal RampDirection; vertical offset equals mesh offset.
            if high[1]-low[1]>.03:
                highest=max(points,key=lambda v:v[1]); lowest=min(points,key=lambda v:v[1])
                vx=highest[0]-lowest[0]; vz=highest[2]-lowest[2]
                dr=('PositiveX' if vx>0 else 'NegativeX') if abs(vx)>abs(vz) else ('PositiveZ' if vz>0 else 'NegativeZ')
                collision.append('ramp '+safe(o.name)+' '+fmt([(a+b)/2 for a,b in zip(low,high)])+' '+fmt([b-a for a,b in zip(low,high)])+' '+dr); ramp_count+=1
            else:
                low[1]-=.2; addbox(o.name,low,high)
        elif any(key in o.name for key in keep_collision) or re.fullmatch(r'(Resource cache|Station supply)(\.\d+)?',o.name):
            addbox(o.name,low,high)
        elif o.name.startswith('Ejecta') and o.dimensions.z>1.1 and o.dimensions.x>1.1:
            # Simplified conservative rock collider; tiny scatter is decorative.
            addbox(o.name,low,high)
        elif o.name=='LANDING 03 / pad':
            # Circular landing deck approximated by narrow AABB strips, not a huge square.
            cx=(low[0]+high[0])/2; cz=(low[2]+high[2])/2; r=18
            for k in range(24):
                xa=-r+k*1.5; xb=xa+1.5; zz=math.sqrt(max(0,r*r-max(abs(xa),abs(xb))**2))
                addbox('Landing_slice_'+str(k),(cx+xa,low[1],cz-zz),(cx+xb,high[1]+.08,cz+zz))
        eo.to_mesh_clear()
    # Explicit playable boundary; terrain continues past this as a visual margin.
    for name,lo,hi in [('West_boundary',(-123,0,-103),(-120,80,103)),('East_boundary',(120,0,-103),(123,80,103)),('North_boundary',(-120,0,-103),(120,80,-100)),('South_boundary',(-120,0,100),(120,80,103))]: addbox(name,lo,hi)
    (MAP/'lunar_collision.txt').write_text('# world metres / +Y up; box half extents, ramp full size\n'+'\n'.join(collision)+'\n',encoding='ascii')
    specs=[]; triangles=0
    for name,(pos,norm,indices,cache) in sorted(groups.items()):
        filename=safe(name).lower()+'.txt'; col=colors.get(name,(.5,.5,.5,1)); triangles+=len(indices)//3
        with (MODELS/filename).open('w',encoding='ascii') as f:
            f.write('<Frame>: 0 Lunar_'+safe(name)+'\n<TransformMatrix>:\n1 0 0 0\n0 1 0 0\n0 0 1 0\n0 0 0 1\n')
            f.write(f'<Mesh>: {len(pos)} Lunar_{safe(name)}\n<Positions>: {len(pos)}\n'); f.writelines(fmt(p)+'\n' for p in pos)
            f.write(f'<Normals>: {len(norm)}\n'); f.writelines(fmt(n)+'\n' for n in norm)
            f.write(f'<Colors>: {len(pos)}\n'); f.writelines(fmt(col)+'\n' for _ in pos)
            # Axis conversion reflects handedness; retain project export_model_txt winding convention.
            f.write(f'<Indices>: {len(indices)}\n'); f.writelines(' '.join(str(i) for i in indices[k:k+3])+'\n' for k in range(0,len(indices),3)); f.write('</Mesh>\n</Frame>\n')
        mat=bpy.data.materials[name]; p=mat.node_tree.nodes.get('Principled BSDF')
        emission=tuple(p.inputs['Emission Color'].default_value)[:3]; intensity=p.inputs['Emission Strength'].default_value
        specs.append({'path':'Assets/Models/LunarOutpost/'+filename,'metallic':p.inputs['Metallic'].default_value,'roughness':p.inputs['Roughness'].default_value,'emission':emission,'intensity':intensity})
    # Generated, deterministic material batch table avoids a new runtime asset format.
    h=['#pragma once','#include <array>','namespace Kimgane::Shared::LunarMap {','struct MeshBatch { const wchar_t* path; float metallic; float roughness; float emissionR; float emissionG; float emissionB; float intensity; };',f'inline constexpr std::array<MeshBatch, {len(specs)}> MESH_BATCHES = {{{{']
    for s in specs: h.append('    {L"'+s['path']+'", '+', '.join(f'{v:.6f}F' for v in [s['metallic'],s['roughness'],*s['emission'],s['intensity']])+'},')
    h+=['}};','} // namespace Kimgane::Shared::LunarMap']
    (MAP/'LunarMeshBatches.h').write_text('\n'.join(h)+'\n',encoding='ascii')
    report={'render_batches':len(specs),'structure_triangles':triangles,'terrain_samples':[281,281],'cell_spacing_m':1,'height_scale_m':64,'vertical_offset_m':16,'raw_endian':'little','collision_boxes':boxes,'collision_ramps':ramp_count,'spawn':[0,float(terrain_heights[228*281+140]),88],'texture_mode':'vertex base colors, matching existing POSITION/NORMAL/COLOR renderer; no normal-map sampling'}
    (MAP/'export_report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    if args.save_blend: bpy.ops.wm.save_as_mainfile(filepath=str(Path(args.save_blend).resolve()))
    print(json.dumps(report))

if __name__=='__main__': export()
