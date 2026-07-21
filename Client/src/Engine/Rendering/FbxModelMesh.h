#pragma once

#include "Mesh.h"

#include <DirectXMath.h>

#include <filesystem>
#include <memory>

namespace Kimgane::Engine
{
struct FbxModelMeshLoadOptions
{
    DirectX::XMFLOAT4 defaultColorLinear = {0.72F, 0.74F, 0.78F, 1.0F};
};

class FbxModelMesh final
{
public:
    static std::shared_ptr<Mesh> Load(ID3D12Device& device,
                                      const std::filesystem::path& filePath,
                                      const FbxModelMeshLoadOptions& options = {});

    FbxModelMesh() = delete;
};
} // namespace Kimgane::Engine
