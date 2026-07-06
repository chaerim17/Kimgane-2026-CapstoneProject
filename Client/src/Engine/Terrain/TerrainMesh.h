#pragma once

#include "../Rendering/Mesh.h"
#include "TerrainHeightMap.h"

#include <DirectXMath.h>

#include <memory>

namespace Kimgane::Engine
{
struct TerrainMeshColors
{
    DirectX::XMFLOAT4 lowColorLinear = {0.18F, 0.34F, 0.16F, 1.0F};
    DirectX::XMFLOAT4 midColorLinear = {0.32F, 0.46F, 0.20F, 1.0F};
    DirectX::XMFLOAT4 highColorLinear = {0.56F, 0.55F, 0.48F, 1.0F};
};

class TerrainMeshBuilder final
{
public:
    static std::shared_ptr<Mesh> CreateMesh(ID3D12Device& device,
                                            const TerrainHeightMap& heightMap,
                                            const TerrainMeshColors& colors = {});

    TerrainMeshBuilder() = delete;
};
} // namespace Kimgane::Engine
