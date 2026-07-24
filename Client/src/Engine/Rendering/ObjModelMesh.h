#pragma once

#include "Mesh.h"

#include <DirectXMath.h>

#include <filesystem>
#include <memory>

namespace Kimgane::Engine
{
    struct ObjModelMeshLoadOptions
    {
        DirectX::XMFLOAT4 defaultColorLinear = {0.72F, 0.74F, 0.78F, 1.0F};
    };

    class ObjModelMesh final
    {
    public:
        static std::shared_ptr<Mesh> Load(ID3D12Device& device,
                                          const std::filesystem::path& filePath,
                                          const ObjModelMeshLoadOptions& options = {});
        ObjModelMesh() = delete;
    };
} // namespace Kimgane::Engine
