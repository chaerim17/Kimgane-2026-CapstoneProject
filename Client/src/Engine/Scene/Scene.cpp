#include "Pch.h"

#include "Scene.h"

#include "../Camera/CameraComponent.h"
#include "../Camera/CameraSettings.h"
#include "../Gameplay/PlayerControllerComponent.h"
#include "../../Engine/Network/NetworkManager.h"
#include "../Physics/ColliderComponent.h"
#include "../Physics/TerrainColliderComponent.h"
#include "../Physics/RigidbodyComponent.h"
#include "../Rendering/LightComponent.h"
#include "../Rendering/MaterialComponent.h"
#include "../Rendering/MeshComponent.h"
#include "../Rendering/SceneRenderConstants.h"
#include "../Math/VectorMath.h"
#include "../../Shared/protocol.h"
#include "TestSceneSettings.h"

#include <DirectXMath.h>
#include <iostream>

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
                      std::shared_ptr<Mesh> playerModelMesh,        // 26.07.10 모델 메쉬 매개변수 추가
                      std::shared_ptr<Mesh> terrainMesh,
                      std::shared_ptr<const TerrainHeightMap> terrainHeightMap,
                      const InputManager& inputManager,
                      NetworkManager& networkManager,
                      float cameraAspectRatio)
{
    Clear();
    mNetworkManager = &networkManager;
    mGameplayCamera = nullptr;
    mNetworkPlayers.clear();
    mPlayerMesh = playerModelMesh != nullptr ? std::move(playerModelMesh) : cubeMesh;       // 26.07.10 모델 메쉬가 없으면 큐브 메쉬를 사용

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
    localPlayer.AddComponent<MeshComponent>(mPlayerMesh);       // 26.07.10 모델 메쉬를 사용
    auto& playerMaterial = localPlayer.AddComponent<MaterialComponent>(     // 26.07.10 모델 메쉬를 사용하면 흰색, 아니면 녹색    
        mPlayerMesh == cubeMesh ? TestSceneSettings::PLAYER_BASE_COLOR_LINEAR
                                : TestSceneSettings::PLAYER_MODEL_BASE_COLOR_LINEAR);
    playerMaterial.GetMaterial().SetSurface(0.0F, 0.42F);
    auto& playerController = localPlayer.AddComponent<PlayerControllerComponent>(inputManager, networkManager);
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
    auto& cameraComponent = localPlayer.AddComponent<CameraComponent>(TestSceneSettings::PLAYER_CAMERA_TARGET_OFFSET_M);
    cameraComponent.SetLens(CameraSettings::DEFAULT_FOV_Y_RAD,
                            cameraAspectRatio,
                            CameraSettings::DEFAULT_NEAR_CLIP_M,
                            CameraSettings::DEFAULT_FAR_CLIP_M);
    cameraComponent.Refresh();
    playerController.SetCamera(&cameraComponent.GetCamera());
    mGameplayCamera = &cameraComponent;
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

void TestScene::RefreshGameplayCamera() noexcept
{
    if (mGameplayCamera != nullptr)
    {
        mGameplayCamera->Refresh();
    }
}

const Camera* TestScene::GetGameplayCamera() const noexcept
{
    if (mGameplayCamera == nullptr)
    {
        return nullptr;
    }

    return &mGameplayCamera->GetCamera();
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

void TestScene::UpdateNetworkPlayerPosition(int playerId, const DirectX::XMFLOAT3& positionM, float yaw)
{
    if (mNetworkManager != nullptr && playerId == mNetworkManager->GetMyPlayerId())
    {
        if (mLocalPlayer == nullptr)
        {
            return;
        }
        mLocalPlayer->GetTransform().SetPositionM(positionM);
        return;
    }

    auto iter = mNetworkPlayers.find(playerId);

    if (iter == mNetworkPlayers.end())
    {
        GameObject& networkPlayer = CreateNetworkPlayer(playerId, positionM);

        mNetworkPlayers[playerId] = &networkPlayer;
        return;
    }

    iter->second->GetTransform().SetPositionM(positionM);
    auto rotation = iter->second->GetTransform().GetRotationRad();
    rotation.y = yaw;
    iter->second->GetTransform().SetRotationRad(rotation);
}

void TestScene::RemoveNetworkPlayer(int playerId)
{
    std::cout << "[SCENE REMOVE] " << playerId << '\n';
    auto iter = mNetworkPlayers.find(playerId);

    if (iter == mNetworkPlayers.end())
    {
        return;
    }

    iter->second->SetActive(false);

    mNetworkPlayers.erase(iter);

    std::cout << "[Scene] Network Player " << playerId << " Removed\n";
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
