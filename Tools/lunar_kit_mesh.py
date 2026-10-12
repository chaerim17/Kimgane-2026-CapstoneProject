import math
import bpy
import bmesh
from mathutils import Vector
MATERIALS = []

def engine(v):
    return [round(v[0],6),round(v[2],6),round(v[1],6)]

def atlas_uv(obj, tile):
    mesh = obj.data
    # Primitives already have UVMap. Keep one UV layer so Blender and ObjLoader
    # sample the same atlas (otherwise the shader reads the primitive's old UVs).
    while mesh.uv_layers:
        mesh.uv_layers.remove(mesh.uv_layers[0])
    uv = mesh.uv_layers.new(name="L02_Atlas")
    mesh.uv_layers.active = uv
    column, row = tile % 4, tile // 4
    for polygon in mesh.polygons:
        # Project each planar face independently, keeping wall signage upright.
        normal = polygon.normal
        axis = max(range(3), key=lambda i: abs(normal[i]))
        axes = (1, 2) if axis == 0 else (0, 2) if axis == 1 else (0, 1)
        coords = [(mesh.vertices[mesh.loops[i].vertex_index].co[axes[0]],
                   mesh.vertices[mesh.loops[i].vertex_index].co[axes[1]]) for i in polygon.loop_indices]
        lo = [min(p[a] for p in coords) for a in range(2)]
        hi = [max(p[a] for p in coords) for a in range(2)]
        for i, p in zip(polygon.loop_indices, coords):
            u = (p[0]-lo[0])/max(hi[0]-lo[0], .00001)
            v = (p[1]-lo[1])/max(hi[1]-lo[1], .00001)
            if (axis == 1 and normal.y > 0) or (axis == 0 and normal.x < 0):
                u = 1-u
            uv.data[i].uv = ((column + .025 + .95*u)/4,
                             1-(row + .975 - .95*v)/4)
    mesh.materials.append(MATERIALS[tile])


def finish_shape(part, obj, tile, bevel=0):
    atlas_uv(obj, tile)
    if bevel:
        modifier = obj.modifiers.new("Manufactured edge / fracture thickness", "BEVEL")
        modifier.width = bevel
        modifier.segments = 1
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    part.objects.append(obj)
    return obj


def box(part, center, size, tile=0, bevel=.025, collide=False):
    bpy.ops.mesh.primitive_cube_add(size=1, location=center)
    obj = bpy.context.object
    obj.name = part.name + "_detail"
    obj.dimensions = size
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    # Avoid collapsed bevel faces on thin trim (Blender's overlap clamp can leave
    # zero-area triangles when the width reaches half the panel thickness).
    finish_shape(part, obj, tile, min(bevel, min(size)*.23))
    if collide:
        name = f"{part.name}_c{len(part.colliders):02}"
        entry = {"name": name, "partId": part.name, "centerM": engine(center),
                 "halfExtentsM": engine([s*.5 for s in size])}
        part.colliders.append(entry)
    return obj


def beam(part, start, end, width=.12, tile=2):
    start, end = Vector(start), Vector(end)
    obj = box(part, (start+end)*.5, (width, width, (end-start).length), tile, min(.02, width*.15))
    obj.rotation_euler = (end-start).to_track_quat("Z", "Y").to_euler()
    return obj


def cylinder(part, center, radius, depth, tile=2, vertices=12, axis="Z"):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=center)
    obj = bpy.context.object
    if axis == "Y":
        obj.rotation_euler.x = math.pi/2
    elif axis == "X":
        obj.rotation_euler.y = math.pi/2
    finish_shape(part, obj, tile, .012)
    return obj


def prism(part, points, bottom, thickness, tile=0):
    n = len(points)
    vertices = [(x, y, z) for z in (bottom, bottom+thickness) for x, y in points]
    faces = [tuple(reversed(range(n))), tuple(range(n, 2*n))]
    faces += [(i, (i+1)%n, (i+1)%n+n, i+n) for i in range(n)]
    mesh = bpy.data.meshes.new(part.name + "_closed_shell")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(part.name + "_torn", mesh)
    bpy.context.collection.objects.link(obj)
    return finish_shape(part, obj, tile)


