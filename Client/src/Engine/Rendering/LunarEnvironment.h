#pragma once

#include "Mesh.h"
#include "../../Shared/World/EnvironmentLayout.h"

#include <array>
#include <memory>
#include <vector>

namespace Kimgane::Engine
{
class Scene;
class GameObject;

// Immutable GPU meshes are shared between scene builds and repeated placements.
class LunarEnvironmentAssets final
{
public:
    struct Instance
    {
        Shared::World::EnvironmentPlacement placement;
        std::array<std::shared_ptr<Mesh>, 4> lodMeshes;
    };
    static std::shared_ptr<LunarEnvironmentAssets> Load(ID3D12Device& device);
    [[nodiscard]] const std::vector<Instance>& GetInstances() const noexcept { return mInstances; }

private:
    std::vector<Instance> mInstances;
};

class LunarEnvironment final
{
public:
    void Build(Scene& scene, const std::shared_ptr<LunarEnvironmentAssets>& assets,
               D3D12_GPU_DESCRIPTOR_HANDLE texture);
    void UpdateLod(const DirectX::XMFLOAT3& cameraPositionM);

private:
    struct Entry
    {
        GameObject* object;
        std::array<std::shared_ptr<Mesh>, 4> meshes;
        unsigned int lod = 0;
        float radiusM = 0;
    };
    std::vector<Entry> mEntries;
};
} // namespace Kimgane::Engine
