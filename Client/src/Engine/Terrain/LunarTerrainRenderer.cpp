#include "Pch.h"
#include "LunarTerrainRenderer.h"
#include "../Rendering/Dx12Renderer.h"
#include "../Rendering/SceneRenderConstants.h"
#include "../../Shared/World/TestMapSettings.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace Kimgane::Engine
{
std::shared_ptr<LunarTerrainRenderer> LunarTerrainRenderer::Create(Dx12Renderer& renderer, const TerrainHeightMap& map)
{
    auto terrain = std::make_shared<LunarTerrainRenderer>();
    constexpr std::array<const wchar_t*, TEXTURE_COUNT> PATHS = {
        L"Assets/Textures/Terrain/lunar_dust.dds", L"Assets/Textures/Terrain/lunar_rock.dds",
        L"Assets/Textures/Terrain/lunar_gravel.dds",
        L"Assets/Textures/Terrain/lunar_dust_normal.dds", L"Assets/Textures/Terrain/lunar_rock_normal.dds",
        L"Assets/Textures/Terrain/lunar_gravel_normal.dds", L"Assets/Textures/Terrain/lunar_dust_arh.dds",
        L"Assets/Textures/Terrain/lunar_rock_arh.dds", L"Assets/Textures/Terrain/lunar_gravel_arh.dds",
        L"Assets/Textures/Terrain/lunar_detail_normal.dds"};
    for (std::size_t i = 0; i < PATHS.size(); ++i)
    {
        terrain->mTextures[i] = Texture::Load(renderer.GetDevice(), renderer.GetCommandQueue(), PATHS[i]);
        const auto desc = terrain->mTextures[i]->GetResource().GetDesc();
        const auto expected = i < 3 ? DXGI_FORMAT_BC1_UNORM_SRGB
            : (i < 6 || i == 9 ? DXGI_FORMAT_BC5_UNORM : DXGI_FORMAT_BC1_UNORM);
        if (desc.Format != expected || desc.MipLevels < 2)
            throw std::runtime_error("Lunar terrain requires BC1 sRGB / BC5 DDS textures with mipmaps");
        terrain->mTextureHandles[i] = renderer.CreateTextureView(terrain->mTextures[i]->GetResource());
    }
    const auto translation = Kimgane::Shared::World::TestMapSettings::TERRAIN_POSITION_M;
    for (unsigned int z = 0; z < map.GetLength() - 1; z += LunarRenderSettings::CHUNK_CELLS)
        for (unsigned int x = 0; x < map.GetWidth() - 1; x += LunarRenderSettings::CHUNK_CELLS)
        {
            Chunk chunk;
            const auto cellsX = std::min(LunarRenderSettings::CHUNK_CELLS, map.GetWidth() - 1 - x);
            const auto cellsZ = std::min(LunarRenderSettings::CHUNK_CELLS, map.GetLength() - 1 - z);
            for (unsigned int lod = 0; lod < LunarRenderSettings::LOD_COUNT; ++lod)
            {
                const auto geometry = BuildTerrainChunk(map, x, z, cellsX, cellsZ, lod);
                chunk.meshes[lod] = Mesh::Create(renderer.GetDevice(), geometry.vertices, geometry.indices,
                                               MeshPrimitiveTopology::TriangleList, false);
                chunk.triangleCounts[lod] = static_cast<unsigned int>(geometry.indices.size() / 3);
            }
            chunk.bounds = chunk.meshes[0]->GetLocalAabb();
            chunk.bounds.Center.x += translation.x;
            chunk.bounds.Center.y += translation.y;
            chunk.bounds.Center.z += translation.z;
            chunk.bounds.Extents.y += 0.05F;
            const auto cliff = BuildCliffChunk(map, x, z, cellsX, cellsZ);
            if (!cliff.indices.empty())
            {
                chunk.cliff = Mesh::Create(renderer.GetDevice(), cliff.vertices, cliff.indices,
                                          MeshPrimitiveTopology::TriangleList, false);
                chunk.cliffTriangles = static_cast<unsigned int>(cliff.indices.size() / 3);
            }
            terrain->mChunks.push_back(std::move(chunk));
        }
    return terrain;
}

void LunarTerrainRenderer::Render(ID3D12GraphicsCommandList& commandList, const DirectX::XMFLOAT3& cameraM,
                                  const DirectX::XMFLOAT4X4& viewProjection)
{
    mStats = {};
    mStats.totalChunks = static_cast<unsigned int>(mChunks.size());
    // CreateTextureView allocated these ten SRVs consecutively during Create.
    commandList.SetGraphicsRootDescriptorTable(RenderRootParameter::TERRAIN_TEXTURES, mTextureHandles[0]);
    ObjectShaderConstants constants;
    const auto translation = Kimgane::Shared::World::TestMapSettings::TERRAIN_POSITION_M;
    DirectX::XMStoreFloat4x4(&constants.world, DirectX::XMMatrixTranslation(translation.x, translation.y, translation.z));
    for (auto& chunk : mChunks)
    {
        if (!IsTerrainBoundsVisible(chunk.bounds, viewProjection)) continue;
        const auto& c = chunk.bounds.Center;
        const auto& e = chunk.bounds.Extents;
        const float dx = std::max(std::abs(cameraM.x - c.x) - e.x, 0.0F);
        const float dy = std::max(std::abs(cameraM.y - c.y) - e.y, 0.0F);
        const float dz = std::max(std::abs(cameraM.z - c.z) - e.z, 0.0F);
        const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
        chunk.lod = SelectTerrainLod(distance, chunk.lod);
        constants.surface = {0, 0.95F, 0, 0};
        commandList.SetGraphicsRoot32BitConstants(RenderRootParameter::OBJECT,
            RenderRootParameter::OBJECT_CONSTANTS_32BIT_COUNT, &constants, 0);
        chunk.meshes[chunk.lod]->Render(commandList);
        ++mStats.visibleChunks;
        ++mStats.lodChunks[chunk.lod];
        mStats.triangles += chunk.triangleCounts[chunk.lod];
        // Distant rock is represented by the slope material, avoiding extra draws.
        if (chunk.cliff && chunk.lod == 0)
        {
            constants.surface.z = 1;
            commandList.SetGraphicsRoot32BitConstants(RenderRootParameter::OBJECT,
                RenderRootParameter::OBJECT_CONSTANTS_32BIT_COUNT, &constants, 0);
            chunk.cliff->Render(commandList);
            ++mStats.cliffDraws;
            mStats.triangles += chunk.cliffTriangles;
        }
    }
}
} // namespace Kimgane::Engine
