#include "Pch.h"

#include "Scene.h"

#include "../Physics/ColliderComponent.h"
#include "../Physics/TerrainColliderComponent.h"
#include "../Rendering/LightComponent.h"
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
    collisionManager_.ClearColliders();
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

    collisionManager_.Update(false);
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

        ObjectShaderConstants objectConstants = {};
        objectConstants.world = object->GetTransform().GetWorldMatrix4x4();
        objectConstants.baseColor = materialComponent->GetMaterial().GetBaseColorLinear();
        objectConstants.surface = {materialComponent->GetMaterial().GetMetallic(),
                                   materialComponent->GetMaterial().GetRoughness(),
                                   0.0F,
                                   0.0F};
        objectConstants.emission = materialComponent->GetMaterial().GetEmissionLinear();
        commandList.SetGraphicsRoot32BitConstants(RenderRootParameter::kObject,
                                                  RenderRootParameter::kObjectConstants32BitCount,
                                                  &objectConstants,
                                                  0);

        meshComponent->Render(commandList);
    }
}

const std::vector<std::unique_ptr<GameObject>>& Scene::GetObjects() const noexcept
{
    return objects_;
}

CollisionManager& Scene::GetCollisionManager() noexcept
{
    return collisionManager_;
}

const CollisionManager& Scene::GetCollisionManager() const noexcept
{
    return collisionManager_;
}

const DirectionalLight& Scene::GetDirectionalLight() const noexcept
{
    for (const auto& object : objects_)
    {
        if (!object || !object->IsActive())
        {
            continue;
        }

        const auto* lightComponent = object->GetComponent<DirectionalLightComponent>();
        if (lightComponent != nullptr)
        {
            return lightComponent->GetLight();
        }
    }

    return directionalLight_;
}

void TestScene::Build(std::shared_ptr<Mesh> cubeMesh,
                      std::shared_ptr<Mesh> terrainMesh,
                      std::shared_ptr<const TerrainHeightMap> terrainHeightMap)
{
    Clear();

    GameObject& lightObject = CreateObject("Directional Light");
    auto& lightComponent = lightObject.AddComponent<DirectionalLightComponent>();
    lightComponent.SetDirection({0.35F, -1.0F, 0.25F});
    lightComponent.SetColorLinear({1.0F, 0.96F, 0.86F});
    lightComponent.SetIntensity(1.05F);
    lightComponent.SetAmbientStrength(0.16F);

    GameObject& terrain = CreateObject("Test Terrain");
    terrain.AddComponent<MeshComponent>(std::move(terrainMesh));
    auto& terrainMaterial = terrain.AddComponent<MaterialComponent>(DirectX::XMFLOAT4{1.0F, 1.0F, 1.0F, 1.0F});
    terrainMaterial.GetMaterial().SetSurface(0.0F, 0.9F);
    auto& terrainCollider = terrain.AddComponent<TerrainColliderComponent>(std::move(terrainHeightMap));
    GetCollisionManager().AddCollider(terrainCollider);
    terrain_ = &terrain;

    GameObject& cube = CreateObject("Test Cube");
    cube.GetTransform().SetPositionM({TestSceneSettings::kCubeStartPositionM.x,
                                      TestSceneSettings::kCubeStartPositionM.y + 1.1F,
                                      TestSceneSettings::kCubeStartPositionM.z});
    cube.GetTransform().SetRotationRad({DirectX::XMConvertToRadians(24.0F), DirectX::XMConvertToRadians(36.0F), 0.0F});
    cube.AddComponent<MeshComponent>(std::move(cubeMesh));
    auto& cubeMaterial = cube.AddComponent<MaterialComponent>(TestSceneSettings::kCubeBaseColorLinear);
    cubeMaterial.GetMaterial().SetSurface(0.0F, 0.32F);
    cubeMaterial.GetMaterial().SetEmissionLinear({0.04F, 0.12F, 0.18F}, 0.35F);
    auto& boxCollider = cube.AddComponent<BoxColliderComponent>(DirectX::XMFLOAT3{0.0F, 0.0F, 0.0F},
                                                                DirectX::XMFLOAT3{TestSceneSettings::kCubeSizeM,
                                                                                  TestSceneSettings::kCubeSizeM,
                                                                                  TestSceneSettings::kCubeSizeM});
    GetCollisionManager().AddCollider(boxCollider);

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
