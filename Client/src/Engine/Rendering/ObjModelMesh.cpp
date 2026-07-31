#include "Pch.h"

#include "ObjModelMesh.h"

#include "../../Shared/Geometry/ObjLoader.h"

#include <cstddef>
#include <vector>

namespace Kimgane::Engine
{
namespace Geometry = Kimgane::Shared::Geometry;

namespace
{

DirectX::XMFLOAT3 ToXMFloat3(const Geometry::Vec3& value) noexcept // Shared Vec3를 DirectX 타입으로 변환
{
    return {value.x, value.y, value.z};
}

std::vector<Mesh::Vertex> BuildVertices(const Geometry::ObjGeometryData& geometry,
                                        const ObjModelMeshLoadOptions& options) // 순수 지오메트리에 색상을 붙여 GPU용 Vertex로 변환
{
    std::vector<Mesh::Vertex> vertices;
    vertices.reserve(geometry.positionsM.size());

    for (std::size_t i = 0; i < geometry.positionsM.size(); ++i)
    {
        vertices.push_back({ToXMFloat3(geometry.positionsM[i]),
                            ToXMFloat3(geometry.normals[i]),
                            options.defaultColorLinear});
    }

    return vertices;
}

} // namespace

std::shared_ptr<Mesh> ObjModelMesh::Load(ID3D12Device& device,
                                         const std::filesystem::path& filePath,
                                         const ObjModelMeshLoadOptions& options) // obj 파일을 로드하여 Mesh 객체를 생성
{
    const Geometry::ObjGeometryData geometry = Geometry::ObjLoader::Load(filePath);
    const std::vector<Mesh::Vertex> vertices = BuildVertices(geometry, options);

    return Mesh::Create(device, vertices, geometry.indices);
}

} // namespace Kimgane::Engine
