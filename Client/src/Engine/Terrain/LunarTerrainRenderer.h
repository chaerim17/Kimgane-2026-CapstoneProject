#pragma once

#include "TerrainChunkGeometry.h"
#include "../Rendering/Texture.h"

namespace Kimgane::Engine
{
class Dx12Renderer;

struct TerrainDrawStats
{
    unsigned int totalChunks = 0;
    unsigned int visibleChunks = 0;
    unsigned int triangles = 0;
    unsigned int cliffDraws = 0;
    std::array<unsigned int, LunarRenderSettings::LOD_COUNT> lodChunks = {};
};

class LunarTerrainRenderer final
{
public:
    static constexpr unsigned int TEXTURE_COUNT = 10;
    static std::shared_ptr<LunarTerrainRenderer> Create(Dx12Renderer& renderer, const TerrainHeightMap& map);
    void Render(ID3D12GraphicsCommandList& commandList, const DirectX::XMFLOAT3& cameraM,
                const DirectX::XMFLOAT4X4& viewProjection);
    [[nodiscard]] const TerrainDrawStats& GetStats() const noexcept { return mStats; }

private:
    struct Chunk
    {
        std::array<std::shared_ptr<Mesh>, LunarRenderSettings::LOD_COUNT> meshes;
        std::array<unsigned int, LunarRenderSettings::LOD_COUNT> triangleCounts = {};
        std::shared_ptr<Mesh> cliff;
        unsigned int cliffTriangles = 0;
        DirectX::BoundingBox bounds;
        unsigned int lod = 0;
    };
    std::vector<Chunk> mChunks;
    std::array<std::shared_ptr<Texture>, TEXTURE_COUNT> mTextures;
    std::array<D3D12_GPU_DESCRIPTOR_HANDLE, TEXTURE_COUNT> mTextureHandles = {};
    TerrainDrawStats mStats;
};
} // namespace Kimgane::Engine
