#pragma once

#include "TerrainMesh.h"
#include <array>

namespace Kimgane::Client
{
namespace LunarRenderSettings
{
inline constexpr unsigned int CHUNK_CELLS = 64;
inline constexpr unsigned int LOD_COUNT = 4;
inline constexpr std::array<float, 3> LOD_DISTANCE_M = {60.0F, 120.0F, 220.0F};
inline constexpr float LOD_HYSTERESIS_M = 6.0F;
inline constexpr float FAR_CLIP_M = 650.0F;
}

struct TerrainGeometry
{
    std::vector<Mesh::Vertex> vertices;
    std::vector<std::uint32_t> indices;
};

// 모든 LOD가 같은 고해상도 테두리를 유지함. 테두리와 저해상도 내부를 부채꼴 삼각형으로 이어서
// 틈이나 스커트 없이, 이웃 청크를 갱신하지 않고도 맞물림.
TerrainGeometry BuildTerrainChunk(const TerrainHeightMap& map, unsigned int startX,
                                 unsigned int startZ, unsigned int cellsX, unsigned int cellsZ,
                                 unsigned int lod);
TerrainGeometry BuildCliffChunk(const TerrainHeightMap& map, unsigned int startX,
                               unsigned int startZ, unsigned int cellsX, unsigned int cellsZ);
unsigned int SelectTerrainLod(float distanceM, unsigned int previousLod) noexcept;
bool IsTerrainBoundsVisible(const DirectX::BoundingBox& bounds,
                            const DirectX::XMFLOAT4X4& viewProjection) noexcept;
} // namespace Kimgane::Client
