#include "Pch.h"
#include "TerrainChunkGeometry.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace Kimgane::Engine
{
namespace
{
Mesh::Vertex SampleVertex(const TerrainHeightMap& map, unsigned int x, unsigned int z)
{
    const float sx = x * map.GetCellSpacingM();
    const float sz = z * map.GetCellSpacingM();
    return {{sx - map.GetWorldWidthM() * 0.5F, map.SampleHeightM(sx, sz), sz - map.GetWorldLengthM() * 0.5F},
            map.SampleNormal(sx, sz), {1, 1, 1, 1}, {sx, sz}};
}
}

TerrainGeometry BuildTerrainChunk(const TerrainHeightMap& map, unsigned int startX,
                                 unsigned int startZ, unsigned int cellsX, unsigned int cellsZ,
                                 unsigned int lod)
{
    if (!cellsX || !cellsZ || lod >= LunarRenderSettings::LOD_COUNT ||
        startX >= map.GetWidth() - 1 || startZ >= map.GetLength() - 1 ||
        cellsX > map.GetWidth() - 1 - startX || cellsZ > map.GetLength() - 1 - startZ)
        throw std::invalid_argument("Invalid terrain chunk extent or LOD");

    TerrainGeometry result;
    std::vector<int> cached(static_cast<std::size_t>(cellsX + 1) * (cellsZ + 1), -1);
    auto vertex = [&](unsigned int x, unsigned int z)
    {
        auto& index = cached[static_cast<std::size_t>(z) * (cellsX + 1) + x];
        if (index < 0)
        {
            index = static_cast<int>(result.vertices.size());
            result.vertices.push_back(SampleVertex(map, startX + x, startZ + z));
        }
        return static_cast<std::uint32_t>(index);
    };
    const unsigned int requestedStep = 1U << lod;
    // A partial final chunk falls back to the fine grid when needed. This also
    // prevents T-junctions where an odd tail would meet a coarse interior cell.
    const unsigned int step = cellsX % requestedStep == 0 && cellsZ % requestedStep == 0 ? requestedStep : 1U;
    for (unsigned int z = 0; z < cellsZ; z += step)
    {
        for (unsigned int x = 0; x < cellsX; x += step)
        {
            const auto right = std::min(x + step, cellsX);
            const auto bottom = std::min(z + step, cellsZ);
            const bool boundary = x == 0 || z == 0 || right == cellsX || bottom == cellsZ;
            // Odd-sized tail cells use fine triangles, preserving arbitrary RAW map extents.
            if (step == 1 || right - x != step || bottom - z != step)
            {
                for (auto fz = z; fz < bottom; ++fz)
                    for (auto fx = x; fx < right; ++fx)
                        result.indices.insert(result.indices.end(), {vertex(fx, fz), vertex(fx, fz + 1),
                            vertex(fx + 1, fz), vertex(fx + 1, fz), vertex(fx, fz + 1), vertex(fx + 1, fz + 1)});
            }
            else if (!boundary)
            {
                result.indices.insert(result.indices.end(), {vertex(x, z), vertex(x, bottom), vertex(right, z),
                    vertex(right, z), vertex(x, bottom), vertex(right, bottom)});
            }
            else
            {
                std::vector<std::uint32_t> ring;
                for (auto p = z; p < bottom; p += x == 0 ? 1U : step) ring.push_back(vertex(x, p));
                for (auto p = x; p < right; p += bottom == cellsZ ? 1U : step) ring.push_back(vertex(p, bottom));
                for (auto p = bottom; p > z; p -= right == cellsX ? 1U : step) ring.push_back(vertex(right, p));
                for (auto p = right; p > x; p -= z == 0 ? 1U : step) ring.push_back(vertex(p, z));
                const auto center = vertex(x + step / 2, z + step / 2);
                for (std::size_t i = 0; i < ring.size(); ++i)
                    result.indices.insert(result.indices.end(), {center, ring[i], ring[(i + 1) % ring.size()]});
            }
        }
    }
    return result;
}

TerrainGeometry BuildCliffChunk(const TerrainHeightMap& map, unsigned int startX,
                               unsigned int startZ, unsigned int cellsX, unsigned int cellsZ)
{
    TerrainGeometry result;
    // Faceted rock shells follow the collision surface on steep crater walls.
    // Offset is only 3 cm; these are surface details, not uncoupled collision walls.
    auto face = [&](Mesh::Vertex a, Mesh::Vertex b, Mesh::Vertex c)
    {
        const auto pa = DirectX::XMLoadFloat3(&a.positionM);
        const auto pb = DirectX::XMLoadFloat3(&b.positionM);
        const auto pc = DirectX::XMLoadFloat3(&c.positionM);
        DirectX::XMFLOAT3 normal;
        DirectX::XMStoreFloat3(&normal, DirectX::XMVector3Normalize(DirectX::XMVector3Cross(
            DirectX::XMVectorSubtract(pb, pa), DirectX::XMVectorSubtract(pc, pa))));
        if (normal.y > 0.78F) return;
        for (auto v : {a, b, c})
        {
            v.positionM.y += 0.03F;
            v.normal = normal;
            result.indices.push_back(static_cast<std::uint32_t>(result.vertices.size()));
            result.vertices.push_back(v);
        }
    };
    for (auto z = startZ; z < startZ + cellsZ; ++z)
        for (auto x = startX; x < startX + cellsX; ++x)
        {
            const auto a = SampleVertex(map, x, z);
            if (std::sin(a.positionM.x * 0.18F + a.positionM.z * 0.13F) < -0.3F) continue;
            const auto b = SampleVertex(map, x, z + 1);
            const auto c = SampleVertex(map, x + 1, z);
            const auto d = SampleVertex(map, x + 1, z + 1);
            face(a, b, c);
            face(c, b, d);
        }
    return result;
}

unsigned int SelectTerrainLod(float distanceM, unsigned int previousLod) noexcept
{
    auto lod = std::min(previousLod, LunarRenderSettings::LOD_COUNT - 1);
    while (lod + 1 < LunarRenderSettings::LOD_COUNT &&
           distanceM > LunarRenderSettings::LOD_DISTANCE_M[lod] + LunarRenderSettings::LOD_HYSTERESIS_M) ++lod;
    while (lod > 0 && distanceM < LunarRenderSettings::LOD_DISTANCE_M[lod - 1] - LunarRenderSettings::LOD_HYSTERESIS_M) --lod;
    return lod;
}

bool IsTerrainBoundsVisible(const DirectX::BoundingBox& bounds,
                            const DirectX::XMFLOAT4X4& viewProjection) noexcept
{
    DirectX::XMFLOAT3 corners[8];
    bounds.GetCorners(corners);
    const auto matrix = DirectX::XMLoadFloat4x4(&viewProjection);
    unsigned int outside = 63;
    for (const auto& corner : corners)
    {
        DirectX::XMFLOAT4 p;
        DirectX::XMStoreFloat4(&p, DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&corner), matrix));
        const unsigned int mask = (p.x < -p.w ? 1U : 0U) | (p.x > p.w ? 2U : 0U)
            | (p.y < -p.w ? 4U : 0U) | (p.y > p.w ? 8U : 0U)
            | (p.z < 0 ? 16U : 0U) | (p.z > p.w ? 32U : 0U);
        outside &= mask;
    }
    return outside == 0;
}
} // namespace Kimgane::Engine
