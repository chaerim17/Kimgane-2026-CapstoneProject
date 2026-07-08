#include "Pch.h"

#include "Scene.h"

#include "../Gameplay/PlayerControllerComponent.h"
#include "../Physics/ColliderComponent.h"
#include "../Physics/TerrainColliderComponent.h"
#include "../Physics/RigidbodyComponent.h"
#include "../Rendering/LightComponent.h"
#include "../Rendering/MaterialComponent.h"
#include "../Rendering/MeshComponent.h"
#include "../Rendering/SceneRenderConstants.h"
#include "../Math/VectorMath.h"
#include "../Network/NetworkSettings.h"
#include "TestSceneSettings.h"

#include <DirectXMath.h>

#include <utility>

namespace Kimgane::Engine
{
namespace
{
const DirectX::XMFLOAT3 NO_EMISSION_LINEAR = {0.0F, 0.0F, 0.0F};

GameObject& CreateMaterialProbe(Scene& scene,
                                const std::shared_ptr<Mesh>& cubeMesh,
                                std::string name,
                                const DirectX::XMFLOAT3& positionM,
                                const DirectX::XMFLOAT4& baseColorLinear,
                                float metallic,
                                float roughness,
                                const DirectX::XMFLOAT3& emissionColorLinear,
                                float emissionIntensity)
{
    GameObject& probe = scene.CreateObject(std::move(name));
    probe.GetTransform().SetPositionM(positionM);
    probe.AddComponent<MeshComponent>(cubeMesh);
    auto& material = probe.AddComponent<MaterialComponent>(baseColorLinear);
    material.GetMaterial().SetSurface(metallic, roughness);
    material.GetMaterial().SetEmissionLinear(emissionColorLinear, emissionIntensity);
    return probe;
}
} // namespace

GameObject& Scene::CreateObject(std::string name)
{
    auto object = std::make_unique<GameObject>(std::move(name));
    GameObject& reference = *object;
    mObjects.push_back(std::move(object));
    return reference;
}

void Scene::Clear() noexcept
{
    mCollisionManager.ClearColliders();
    mObjects.clear();
}

void Scene::Update(float deltaTimeSec)
{
    for (const auto& object : mObjects)
    {
        if (object)
        {
            object->Update(deltaTimeSec);
        }
    }

    mCollisionManager.Update(false);
}

void Scene::Render(ID3D12GraphicsCommandList& commandList) const
{
    for (const auto& object : mObjects)
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
        commandList.SetGraphicsRoot32BitConstants(RenderRootParameter::OBJECT,
                                                  RenderRootParameter::OBJECT_CONSTANTS_32BIT_COUNT,
                                                  &objectConstants,
                                                  0);

        meshComponent->Render(commandList);
    }
}

const std::vector<std::unique_ptr<GameObject>>& Scene::GetObjects() const noexcept
{
    return mObjects;
}

CollisionManager& Scene::GetCollisionManager() noexcept
{
    return mCollisionManager;
}

const CollisionManager& Scene::GetCollisionManager() const noexcept
{
    return mCollisionManager;
}

const DirectionalLight& Scene::GetDirectionalLight() const noexcept
{
    for (const auto& object : mObjects)
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

    return mDirectionalLight;
}

void TestScene::Build(std::shared_ptr<Mesh> cubeMesh,
                      std::shared_ptr<Mesh> terrainMesh,
                      std::shared_ptr<const TerrainHeightMap> terrainHeightMap,
                      const InputManager& inputManager,
                      const Camera& gameplayCamera)
{
    Clear();
    mNetworkPlayers.clear();
    mPlayerMesh = cubeMesh;

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
    mTerrain = &terrain;

    GameObject& cube = CreateObject("Test Cube");
    cube.GetTransform().SetPositionM({TestSceneSettings::CUBE_START_POSITION_M.x,
                                      TestSceneSettings::CUBE_START_POSITION_M.y + 1.1F,
                                      TestSceneSettings::CUBE_START_POSITION_M.z});
    cube.GetTransform().SetRotationRad({DirectX::XMConvertToRadians(24.0F), DirectX::XMConvertToRadians(36.0F), 0.0F});
    cube.AddComponent<MeshComponent>(cubeMesh);
    auto& cubeMaterial = cube.AddComponent<MaterialComponent>(TestSceneSettings::CUBE_BASE_COLOR_LINEAR);
    cubeMaterial.GetMaterial().SetSurface(0.0F, 0.32F);
    cubeMaterial.GetMaterial().SetEmissionLinear({0.04F, 0.12F, 0.18F}, 0.35F);
    auto& boxCollider = cube.AddComponent<BoxColliderComponent>(DirectX::XMFLOAT3{0.0F, 0.0F, 0.0F},
                                                                DirectX::XMFLOAT3{TestSceneSettings::CUBE_SIZE_M,
                                                                                  TestSceneSettings::CUBE_SIZE_M,
                                                                                  TestSceneSettings::CUBE_SIZE_M});
    GetCollisionManager().AddCollider(boxCollider);

    mTestCube = &cube;

    GameObject& localPlayer = CreateObject("Local Player");
    localPlayer.GetTransform().SetPositionM(TestSceneSettings::PLAYER_START_POSITION_M);
    localPlayer.AddComponent<MeshComponent>(cubeMesh);
    auto& playerMaterial = localPlayer.AddComponent<MaterialComponent>(TestSceneSettings::PLAYER_BASE_COLOR_LINEAR);
    playerMaterial.GetMaterial().SetSurface(0.0F, 0.42F);
    auto& playerController = localPlayer.AddComponent<PlayerControllerComponent>(inputManager);
    playerController.SetCamera(&gameplayCamera);
    auto& playerRigidbody = localPlayer.AddComponent<RigidbodyComponent>();
    playerRigidbody.SetUseGravity(false);
    playerRigidbody.SetDragPerSec(0.0F);
    playerRigidbody.SetGroundFrictionPerSec(0.0F);
    playerRigidbody.SetGrounded(true);
    auto& playerCollider =
        localPlayer.AddComponent<CapsuleColliderComponent>(DirectX::XMFLOAT3{0.0F, 0.0F, 0.0F},
                                                           TestSceneSettings::PLAYER_CAPSULE_RADIUS_M,
                                                           TestSceneSettings::PLAYER_CAPSULE_HEIGHT_M);
    GetCollisionManager().AddCollider(playerCollider);
    mLocalPlayer = &localPlayer;

    CreateMaterialProbe(*this,
                        cubeMesh,
                        "Visual Test Matte",
                        {-3.2F, 1.1F, 2.2F},
                        {0.88F, 0.18F, 0.12F, 1.0F},
                        0.0F,
                        1.0F,
                        NO_EMISSION_LINEAR,
                        0.0F);
    CreateMaterialProbe(*this,
                        cubeMesh,
                        "Visual Test Specular",
                        {0.0F, 1.1F, 2.2F},
                        {0.88F, 0.88F, 0.82F, 1.0F},
                        0.0F,
                        0.04F,
                        NO_EMISSION_LINEAR,
                        0.0F);
    CreateMaterialProbe(*this,
                        cubeMesh,
                        "Visual Test Emissive",
                        {3.2F, 1.1F, 2.2F},
                        {0.08F, 0.12F, 0.18F, 1.0F},
                        0.0F,
                        0.35F,
                        {0.10F, 0.85F, 1.0F},
                        1.35F);
}

void TestScene::Update(float deltaTimeSec)
{
    if (mTestCube != nullptr)
    {
        mCubeRotationRad += deltaTimeSec * 0.8F;
        mTestCube->GetTransform().SetRotationRad({DirectX::XMConvertToRadians(24.0F), mCubeRotationRad, 0.0F});
    }

    Scene::Update(deltaTimeSec);
}

DirectX::XMFLOAT3 TestScene::GetCameraTargetPositionM() const noexcept
{
    if (mLocalPlayer == nullptr)
    {
        return TestSceneSettings::CAMERA_LOOK_AT_POSITION_M;
    }

    return VectorMath::Add(mLocalPlayer->GetTransform().GetPositionM(), TestSceneSettings::PLAYER_CAMERA_TARGET_OFFSET_M);
}

DirectX::XMFLOAT3 TestScene::GetLocalPlayerPositionM() const noexcept
{
    if (mLocalPlayer == nullptr)
    {
        return TestSceneSettings::PLAYER_START_POSITION_M;
    }

    return mLocalPlayer->GetTransform().GetPositionM();
}

void TestScene::UpdateNetworkPlayerPosition(int playerId, const DirectX::XMFLOAT3& positionM)
{
    // 서버에서 내 플레이어 위치를 보정해서 내려주는 경우를 대비한 처리입니다.
    if (playerId == NetworkSettings::LOCAL_PLAYER_ID && mLocalPlayer != nullptr)
    {
        mLocalPlayer->GetTransform().SetPositionM(positionM);
        return;
    }

    auto iter = mNetworkPlayers.find(playerId);
    if (iter == mNetworkPlayers.end() || iter->second == nullptr)
    {
        // 처음 수신한 플레이어 id라면 렌더링용 오브젝트를 만든 뒤 위치를 관리합니다.
        GameObject& networkPlayer = CreateNetworkPlayer(playerId, positionM);
        mNetworkPlayers[playerId] = &networkPlayer;
        return;
    }

    iter->second->GetTransform().SetPositionM(positionM);
}

GameObject& TestScene::CreateNetworkPlayer(int playerId, const DirectX::XMFLOAT3& positionM)
{
    GameObject& networkPlayer = CreateObject("Network Player " + std::to_string(playerId));
    networkPlayer.GetTransform().SetPositionM(positionM);
    networkPlayer.AddComponent<MeshComponent>(mPlayerMesh);
    auto& material = networkPlayer.AddComponent<MaterialComponent>(TestSceneSettings::NETWORK_PLAYER_BASE_COLOR_LINEAR);
    material.GetMaterial().SetSurface(0.0F, 0.48F);
    return networkPlayer;
}
} // namespace Kimgane::Engine
