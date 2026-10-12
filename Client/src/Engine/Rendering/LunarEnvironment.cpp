#include "Pch.h"
#include "LunarEnvironment.h"
#include "ObjModelMesh.h"
#include "MeshComponent.h"
#include "MaterialComponent.h"
#include "../Scene/Scene.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

namespace Kimgane::Engine
{
std::shared_ptr<LunarEnvironmentAssets> LunarEnvironmentAssets::Load(ID3D12Device& device)
{
    auto result = std::make_shared<LunarEnvironmentAssets>();
    std::unordered_map<std::string, std::array<std::shared_ptr<Mesh>, 4>> cache;
    for (auto placement : Shared::World::LoadEnvironmentLayout(L"Assets/LunarKit/layout.txt"))
    {
        auto [iter, inserted] = cache.try_emplace(placement.meshPrefix);
        if (inserted)
        {
            ObjModelMeshLoadOptions options;
            options.defaultColorLinear = {1, 1, 1, 1};
            for (unsigned int lod = 0; lod < 4; ++lod)
                iter->second[lod] = ObjModelMesh::Load(device,
                    placement.meshPrefix + "_LOD" + std::to_string(lod) + ".obj", options);
        }
        result->mInstances.push_back({std::move(placement), iter->second});
    }
    return result;
}

void LunarEnvironment::Build(Scene& scene, const std::shared_ptr<LunarEnvironmentAssets>& assets,
                             D3D12_GPU_DESCRIPTOR_HANDLE texture)
{
    mEntries.clear();
    if (!assets) throw std::invalid_argument("Lunar environment requires loaded meshes");
    for (const auto& item : assets->GetInstances())
    {
        auto& obj = scene.CreateObject(item.placement.id);
        const auto& p = item.placement.positionM;
        obj.GetTransform().SetPositionM({p.x, p.y, p.z});
        obj.GetTransform().SetRotationRad({0, DirectX::XMConvertToRadians(item.placement.yawDeg), 0});
        obj.AddComponent<MeshComponent>(item.lodMeshes[0]);
        auto& material = obj.AddComponent<MaterialComponent>(DirectX::XMFLOAT4{1, 1, 1, 1});
        material.GetMaterial().SetSurface(0.15F, 0.72F);
        material.GetMaterial().SetTextureGpuHandle(texture);
        const auto& extents = item.lodMeshes[0]->GetLocalAabb().Extents;
        const float radiusM = std::sqrt(extents.x * extents.x + extents.y * extents.y + extents.z * extents.z);
        mEntries.push_back({&obj, item.lodMeshes, 0, radiusM});
    }
}

void LunarEnvironment::UpdateLod(const DirectX::XMFLOAT3& cameraPositionM)
{
    for (auto& entry : mEntries)
    {
        const auto& p = entry.object->GetTransform().GetPositionM();
        const float dx = p.x - cameraPositionM.x, dy = p.y - cameraPositionM.y, dz = p.z - cameraPositionM.z;
        const float distanceM = std::max(0.0F, std::sqrt(dx * dx + dy * dy + dz * dz) - entry.radiusM);
        const auto next = Shared::World::SelectEnvironmentLod(distanceM, entry.lod);
        if (next != entry.lod)
        {
            entry.object->GetComponent<MeshComponent>()->SetMesh(entry.meshes[next]);
            entry.lod = next;
        }
    }
}
} // namespace Kimgane::Engine
