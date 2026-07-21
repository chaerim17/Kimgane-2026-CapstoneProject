# Blender에서 씬을 Kimgane 클라이언트의 변환 텍스트 메시 포맷(.fbx.txt)으로 내보내는 스크립트입니다.
# 대상 파서: Client/src/Engine/Rendering/FbxModelMesh.cpp
#
# 사용법 (Blender 3.x / 4.x):
#   1. Blender에서 File > Import > FBX 로 믹사모 FBX를 임포트합니다.
#   2. Scripting 탭에서 이 파일을 열고 OUTPUT_PATH를 원하는 경로로 수정합니다.
#   3. Run Script를 누르면 OUTPUT_PATH에 텍스트 모델이 생성됩니다.
#
# 커맨드라인 (선택):
#   blender --background --python Tools/export_model_txt.py -- ^
#       "C:/path/to/model.fbx" "C:/Kimgane-2026-CapstoneProject/Assets/Models/Model.fbx.txt"
#
# 좌표계: Blender(오른손, Z-up) -> DirectX(왼손, Y-up), (x, y, z) -> (-x, z, -y).
# y/z 스왑에 up 축 기준 180도 회전을 합성해, Blender에서 -Y(정면 뷰)를 바라보는
# 믹사모 캐릭터가 DX에서 +Z(yaw=0 정면)를 바라보게 합니다.
# 행렬은 파서의 row-vector 규약(world = local * parent, 4행에 이동)으로 변환해 기록합니다.

import sys

import bpy

OUTPUT_PATH = r"C:\Kimgane-2026-CapstoneProject\Assets\Models\Model.fbx.txt"

# 내보낼 오브젝트 타입. 카메라/라이트 등은 제외합니다.
EXPORTED_OBJECT_TYPES = {"MESH", "ARMATURE", "EMPTY"}

# Blender 축 -> DX 축 매핑에 쓰는 인덱스 순열 (x, z, y 스왑)과 부호 (up 축 180도 회전).
AXIS_SWAP = (0, 2, 1, 3)
AXIS_SIGN = (-1.0, 1.0, -1.0, 1.0)


def sanitize_name(name):
    # 파서가 이름을 공백 단위로 읽으므로 공백류 문자를 밑줄로 바꿉니다.
    cleaned = "".join("_" if character.isspace() else character for character in name)
    return cleaned if cleaned else "Unnamed"


def swap_axes(vector):
    return (-vector[0], vector[2], -vector[1])


def to_dx_row_matrix(blender_matrix):
    # 변환 T = R * S(S: y/z 스왑, R: up 축 180도 회전)에 대해 M_dx = (T * L * T^-1)^T.
    # 성분으로는 M[i][j] = sign(i) * sign(j) * L[swap(j)][swap(i)].
    return [
        [
            AXIS_SIGN[row] * AXIS_SIGN[column] * blender_matrix[AXIS_SWAP[column]][AXIS_SWAP[row]]
            for column in range(4)
        ]
        for row in range(4)
    ]


def format_floats(values):
    return " ".join(f"{value:.6f}" for value in values)


def build_mesh_data(mesh_object, depsgraph):
    # 모디파이어(아머추어 포함)가 적용된 평가 결과 메시를 사용합니다.
    evaluated_object = mesh_object.evaluated_get(depsgraph)
    mesh = evaluated_object.to_mesh()
    if mesh is None or len(mesh.polygons) == 0:
        if mesh is not None:
            evaluated_object.to_mesh_clear()
        return None

    mesh.calc_loop_triangles()
    if hasattr(mesh, "calc_normals_split"):
        # Blender 4.1부터는 제거된 API라서 있을 때만 호출합니다(이후 버전은 자동 계산).
        mesh.calc_normals_split()

    positions = []
    normals = []
    indices = []
    vertex_cache = {}

    for triangle in mesh.loop_triangles:
        for loop_index in triangle.loops:
            loop = mesh.loops[loop_index]
            normal = loop.normal
            key = (
                loop.vertex_index,
                round(normal[0], 4),
                round(normal[1], 4),
                round(normal[2], 4),
            )
            cached_index = vertex_cache.get(key)
            if cached_index is None:
                cached_index = len(positions)
                vertex_cache[key] = cached_index
                positions.append(swap_axes(mesh.vertices[loop.vertex_index].co))
                normals.append(swap_axes(normal))
            indices.append(cached_index)

    evaluated_object.to_mesh_clear()

    if not positions or not indices:
        return None

    return positions, normals, indices


def write_frame(stream, scene_object, frame_index, depsgraph, indent):
    pad = "    " * indent
    stream.write(f"{pad}<Frame>: {frame_index} {sanitize_name(scene_object.name)}\n")

    if scene_object.parent is not None:
        local_matrix = scene_object.parent.matrix_world.inverted() @ scene_object.matrix_world
    else:
        local_matrix = scene_object.matrix_world

    dx_matrix = to_dx_row_matrix(local_matrix)
    stream.write(f"{pad}<TransformMatrix>:\n")
    for row in dx_matrix:
        stream.write(f"{pad}{format_floats(row)}\n")

    if scene_object.type == "MESH":
        mesh_data = build_mesh_data(scene_object, depsgraph)
        if mesh_data is not None:
            positions, normals, indices = mesh_data
            stream.write(f"{pad}<Mesh>: {len(positions)} {sanitize_name(scene_object.data.name)}\n")

            stream.write(f"{pad}<Positions>: {len(positions)}\n")
            for position in positions:
                stream.write(f"{pad}{format_floats(position)}\n")

            stream.write(f"{pad}<Normals>: {len(normals)}\n")
            for normal in normals:
                stream.write(f"{pad}{format_floats(normal)}\n")

            stream.write(f"{pad}<Indices>: {len(indices)}\n")
            for start in range(0, len(indices), 3):
                triangle = indices[start:start + 3]
                stream.write(f"{pad}{' '.join(str(index) for index in triangle)}\n")

            stream.write(f"{pad}</Mesh>\n")

    children = [child for child in scene_object.children if child.type in EXPORTED_OBJECT_TYPES]
    if children:
        # 파서가 개수만큼 <Frame>: 토큰을 기대하므로 개수와 블록 수가 정확히 일치해야 합니다.
        stream.write(f"{pad}<Children>: {len(children)}\n")
        next_index = frame_index + 1
        for child in children:
            next_index = write_frame(stream, child, next_index, depsgraph, indent + 1)

        stream.write(f"{pad}</Frame>\n")
        return next_index

    stream.write(f"{pad}</Frame>\n")
    return frame_index + 1


def resolve_cli_arguments():
    if "--" not in sys.argv:
        return None, OUTPUT_PATH

    extra_arguments = sys.argv[sys.argv.index("--") + 1:]
    fbx_path = extra_arguments[0] if len(extra_arguments) >= 1 else None
    output_path = extra_arguments[1] if len(extra_arguments) >= 2 else OUTPUT_PATH
    return fbx_path, output_path


def export_scene(output_path):
    depsgraph = bpy.context.evaluated_depsgraph_get()
    root_objects = [
        scene_object
        for scene_object in bpy.context.scene.objects
        if scene_object.parent is None and scene_object.type in EXPORTED_OBJECT_TYPES
    ]

    if not root_objects:
        raise RuntimeError("내보낼 오브젝트가 없습니다. FBX를 먼저 임포트했는지 확인하세요.")

    frame_index = 0
    with open(output_path, "w", encoding="ascii") as stream:
        for root_object in root_objects:
            frame_index = write_frame(stream, root_object, frame_index, depsgraph, 0)

    print(f"내보내기 완료: {output_path} (프레임 {frame_index}개)")


def main():
    fbx_path, output_path = resolve_cli_arguments()
    if fbx_path is not None:
        bpy.ops.import_scene.fbx(filepath=fbx_path)

    export_scene(output_path)


main()
