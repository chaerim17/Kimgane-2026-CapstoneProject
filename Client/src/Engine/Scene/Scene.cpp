#include "Pch.h"

#include "Scene.h"

#include "../Physics/ColliderComponent.h"
#include "../Physics/TerrainColliderComponent.h"
#include "../Rendering/MaterialComponent.h"
#include "../Rendering/MeshComponent.h"
#include "../Rendering/SceneRenderConstants.h"
#include "TestSceneSettings.h"

#include <DirectXMath.h>

#include <utility>

namespace Kimgane::Engine
{
GameObject& Scene::CreateObject(std::string name)
{
    auto object = std::make_unique<GameObject>(std::move(name));
    GameObject& reference = *object;
    objects_.push_back(std::move(object));
    return reference;
}

void Scene::Clear() noexcept
{
    objects_.clear();
}

void Scene::Update(float deltaTimeSec)
{
    for (const auto& object : objects_)
    {
        if (object)
        {
            object->Update(deltaTimeSec);
        }
    }
}

void Scene::Render(ID3D12GraphicsCommandList& commandList) const
{
    for (const auto& object : objects_)
    {
        if (!object || !object->IsActive())
        {
            continue;
        }

        const auto* meshComponent = object->GetComponent<MeshComponent>();
        const auto* materialComponent = object->GetComponent<MaterialComponent>();
        if (meshComponent == nullptr || materialComponent == nullptr)
        {
            continue;
        }

        DirectX::XMFLOAT4X4 worldMatrix = object->GetTransform().GetWorldMatrix4x4();
        commandList.SetGraphicsRoot32BitConstants(RenderRootParameter::kObject,
                                                  RenderRootParameter::kObjectWorld32BitCount,
                                                  &worldMatrix,
                                                  RenderRootParameter::kObjectWorld32BitOffset);

        const DirectX::XMFLOAT4& baseColor = materialComponent->GetBaseColor();
        commandList.SetGraphicsRoot32BitConstants(RenderRootParameter::kObject,
                                                  4,
                                                  &baseColor,
                                                  RenderRootParameter::kObjectColor32BitOffset);

        meshComponent->Render(commandList);
    }
}

const std::vector<std::unique_ptr<GameObject>>& Scene::GetObjects() const noexcept
{
    return objects_;
}

void TestScene::Build(std::shared_ptr<Mesh> cubeMesh,
                      std::shared_ptr<Mesh> terrainMesh,
                      std::shared_ptr<const TerrainHeightMap> terrainHeightMap)
{
    Clear();

    GameObject& terrain = CreateObject("Test Terrain");
    terrain.AddComponent<MeshComponent>(std::move(terrainMesh));
    terrain.AddComponent<MaterialComponent>(DirectX::XMFLOAT4{1.0F, 1.0F, 1.0F, 1.0F});
    terrain.AddComponent<TerrainColliderComponent>(std::move(terrainHeightMap));
    terrain_ = &terrain;

    GameObject& cube = CreateObject("Test Cube");
    cube.GetTransform().SetPositionM({TestSceneSettings::kCubeStartPositionM.x,
                                      TestSceneSettings::kCubeStartPositionM.y + 1.1F,
                                      TestSceneSettings::kCubeStartPositionM.z});
    cube.GetTransform().SetRotationRad({DirectX::XMConvertToRadians(24.0F), DirectX::XMConvertToRadians(36.0F), 0.0F});
    cube.AddComponent<MeshComponent>(std::move(cubeMesh));
    cube.AddComponent<MaterialComponent>(TestSceneSettings::kCubeBaseColorLinear);
    cube.AddComponent<BoxColliderComponent>(DirectX::XMFLOAT3{0.0F, 0.0F, 0.0F},
                                            DirectX::XMFLOAT3{TestSceneSettings::kCubeSizeM,
                                                              TestSceneSettings::kCubeSizeM,
                                                              TestSceneSettings::kCubeSizeM});

    testCube_ = &cube;
}

void TestScene::Update(float deltaTimeSec)
{
    if (testCube_ != nullptr)
    {
        cubeRotationRad_ += deltaTimeSec * 0.8F;
        testCube_->GetTransform().SetRotationRad({DirectX::XMConvertToRadians(24.0F), cubeRotationRad_, 0.0F});
    }

    Scene::Update(deltaTimeSec);
}
} // namespace Kimgane::Engine
