#include "Pch.h"

#include "TerrainMesh.h"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace Kimgane::Engine
{
namespace
{
DirectX::XMFLOAT4 LerpColor(const DirectX::XMFLOAT4& lhs, const DirectX::XMFLOAT4& rhs, float t) noexcept
{
    t = std::clamp(t, 0.0F, 1.0F);
    return {lhs.x + (rhs.x - lhs.x) * t,
            lhs.y + (rhs.y - lhs.y) * t,
            lhs.z + (rhs.z - lhs.z) * t,
            lhs.w + (rhs.w - lhs.w) * t};
}

DirectX::XMFLOAT4 BuildTerrainColor(float heightM,
                                    const DirectX::BoundingBox& bounds,
                                    const TerrainMeshColors& colors) noexcept
{
    const float minHeightM = bounds.Center.y - bounds.Extents.y;
    const float maxHeightM = bounds.Center.y + bounds.Extents.y;
    const float t = (maxHeightM > minHeightM) ? (heightM - minHeightM) / (maxHeightM - minHeightM) : 0.0F;
    if (t < 0.55F)
    {
        return LerpColor(colors.lowColorLinear, colors.midColorLinear, t / 0.55F);
    }

    return LerpColor(colors.midColorLinear, colors.highColorLinear, (t - 0.55F) / 0.45F);
}
} // namespace

std::shared_ptr<Mesh> TerrainMeshBuilder::CreateMesh(ID3D12Device& device,
                                                     const TerrainHeightMap& heightMap,
                                                     const TerrainMeshColors& colors)
{
    const std::uint32_t width = heightMap.GetWidth();
    const std::uint32_t length = heightMap.GetLength();
    const float halfWidthM = heightMap.GetWorldWidthM() * 0.5F;
    const float halfLengthM = heightMap.GetWorldLengthM() * 0.5F;
    const DirectX::BoundingBox bounds = heightMap.GetCenteredLocalAabb();

    std::vector<Mesh::Vertex> vertices;
    vertices.reserve(static_cast<std::size_t>(width) * length);
    for (std::uint32_t z = 0; z < length; ++z)
    {
        const float sampleZM = static_cast<float>(z) * heightMap.GetCellSpacingM();
        for (std::uint32_t x = 0; x < width; ++x)
        {
            const float sampleXM = static_cast<float>(x) * heightMap.GetCellSpacingM();
            const float heightM = heightMap.SampleHeightM(sampleXM, sampleZM);
            vertices.push_back({{sampleXM - halfWidthM, heightM, sampleZM - halfLengthM},
                                heightMap.SampleNormal(sampleXM, sampleZM),
                                BuildTerrainColor(heightM, bounds, colors)});
        }
    }

    std::vector<std::uint32_t> indices;
    indices.reserve(static_cast<std::size_t>(width - 1U) * (length - 1U) * 6U);
    for (std::uint32_t z = 0; z < length - 1U; ++z)
    {
        for (std::uint32_t x = 0; x < width - 1U; ++x)
        {
            const std::uint32_t leftTop = z * width + x;
            const std::uint32_t rightTop = leftTop + 1U;
            const std::uint32_t leftBottom = (z + 1U) * width + x;
            const std::uint32_t rightBottom = leftBottom + 1U;

            indices.push_back(leftTop);
            indices.push_back(leftBottom);
            indices.push_back(rightTop);
            indices.push_back(rightTop);
            indices.push_back(leftBottom);
            indices.push_back(rightBottom);
        }
    }

    return Mesh::Create(device, vertices, indices);
}
} // namespace Kimgane::Engine
