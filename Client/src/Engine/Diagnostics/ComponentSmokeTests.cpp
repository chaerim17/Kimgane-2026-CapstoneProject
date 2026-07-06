#include "Pch.h"

#include "ComponentSmokeTests.h"

#include "../Camera/CameraSettings.h"
#include "../Camera/SpringArmCamera.h"
#include "../Core/GameObject.h"
#include "../Gameplay/PlayerControllerComponent.h"
#include "../Gameplay/PlayerControllerSettings.h"
#include "../Input/InputManager.h"
#include "../Physics/ColliderComponent.h"
#include "../Physics/CollisionManager.h"
#include "../Physics/RigidbodyComponent.h"
#include "../Physics/TerrainColliderComponent.h"
#include "../Rendering/LightComponent.h"
#include "../Rendering/MaterialComponent.h"
#include "../Rendering/MeshComponent.h"
#include "../Rendering/SceneRenderConstants.h"
#include "../Rendering/Shader.h"
#include "../Scene/Scene.h"
#include "../Terrain/TerrainHeightMap.h"

#include <cmath>
#include <string>

namespace Kimgane::Engine::Diagnostics
{
namespace
{
constexpr float EPSILON = 0.001F;
constexpr unsigned int EXPECTED_SCENE_CONSTANT_32BIT_COUNT = 28U;
constexpr unsigned int EXPECTED_OBJECT_CONSTANT_32BIT_COUNT = 28U;

class CountingComponent final : public Component
{
public:
    explicit CountingComponent(GameObject& owner) noexcept
        : Component(owner)
    {
    }

    void Update(float deltaTimeSec) override
    {
        ++mUpdateCount;
        mLastDeltaTimeSec = deltaTimeSec;
    }

    [[nodiscard]] int GetUpdateCount() const noexcept
    {
        return mUpdateCount;
    }

    [[nodiscard]] float GetLastDeltaTimeSec() const noexcept
    {
        return mLastDeltaTimeSec;
    }

private:
    int mUpdateCount = 0;
    float mLastDeltaTimeSec = 0.0F;
};

void Require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(std::string("Component smoke test failed: ") + message);
    }
}

void RequireNear(float actual, float expected, const char* message)
{
    Require(std::fabs(actual - expected) <= EPSILON, message);
}

void RequireVectorNear(const DirectX::XMFLOAT3& actual,
                       const DirectX::XMFLOAT3& expected,
                       const char* message)
{
    RequireNear(actual.x, expected.x, message);
    RequireNear(actual.y, expected.y, message);
    RequireNear(actual.z, expected.z, message);
}

void RunCoreSmokeTests()
{
    GameObject object("Core Smoke");
    object.GetTransform().SetPositionM({1.0F, 2.0F, 3.0F});
    object.GetTransform().TranslateM({2.0F, -1.0F, 0.5F});
    RequireVectorNear(object.GetTransform().GetPositionM(), {3.0F, 1.0F, 3.5F}, "Transform translation");

    const DirectX::XMFLOAT4X4 world = object.GetTransform().GetWorldMatrix4x4();
    RequireNear(world._41, 3.0F, "Transform world matrix x translation");
    RequireNear(world._42, 1.0F, "Transform world matrix y translation");
    RequireNear(world._43, 3.5F, "Transform world matrix z translation");

    auto& countingComponent = object.AddComponent<CountingComponent>();
    object.Update(0.25F);
    Require(countingComponent.GetUpdateCount() == 1, "GameObject updates active components");
    RequireNear(countingComponent.GetLastDeltaTimeSec(), 0.25F, "Component receives delta time");

    object.SetActive(false);
    object.Update(1.0F);
    Require(countingComponent.GetUpdateCount() == 1, "Inactive GameObject skips component update");
    Require(object.RemoveComponents<CountingComponent>(), "GameObject removes typed components");
    Require(object.GetComponent<CountingComponent>() == nullptr, "Removed component cannot be fetched");
}

void RunCameraSmokeTests()
{
    SpringArmCamera camera;
    camera.SetLens(CameraSettings::DEFAULT_FOV_Y_RAD,
                   16.0F / 9.0F,
                   CameraSettings::DEFAULT_NEAR_CLIP_M,
                   CameraSettings::DEFAULT_FAR_CLIP_M);
    camera.Update(1.0F / 60.0F);
    camera.UpdateEye({0.0F, 0.0F, 0.0F});

    Require(camera.GetEyeM().y > 0.0F, "SpringArmCamera places eye above target");
    Require(camera.GetTargetArmLengthM() >= CameraSettings::SPRING_ARM_MIN_LENGTH_M,
            "SpringArmCamera keeps arm length in valid range");

    const DirectX::XMFLOAT4X4 viewProjection = camera.GetViewProjectionMatrix4x4();
    Require(std::fabs(viewProjection._11) > EPSILON, "Camera builds view projection matrix");
}

void RunMaterialAndLightSmokeTests()
{
    auto material = Material::CreateSolidColor({0.25F, 0.5F, 0.75F, 1.0F});
    RequireNear(material->GetBaseColorLinear().x, 0.25F, "Material stores base color");
    material->SetSurface(-1.0F, 2.0F);
    RequireNear(material->GetMetallic(), 0.0F, "Material clamps metallic minimum");
    RequireNear(material->GetRoughness(), 1.0F, "Material clamps roughness maximum");
    material->SetSurface(0.5F, 0.0F);
    RequireNear(material->GetRoughness(), 0.02F, "Material clamps roughness minimum");
    material->SetEmissionLinear({-1.0F, 0.25F, 2.0F}, -3.0F);
    RequireNear(material->GetEmissionLinear().x, 0.0F, "Material clamps emission red");
    RequireNear(material->GetEmissionLinear().y, 0.25F, "Material stores emission green");
    RequireNear(material->GetEmissionLinear().z, 2.0F, "Material stores emission blue");
    RequireNear(material->GetEmissionLinear().w, 0.0F, "Material clamps emission intensity");

    GameObject materialObject("MaterialComponent Smoke");
    auto& materialComponent = materialObject.AddComponent<MaterialComponent>(std::shared_ptr<Material>{});
    RequireNear(materialComponent.GetBaseColor().x, 1.0F, "MaterialComponent creates fallback material");

    GameObject lightObject("DirectionalLightComponent Smoke");
    auto& lightComponent = lightObject.AddComponent<DirectionalLightComponent>();
    lightComponent.SetDirection({0.0F, -2.0F, 0.0F});
    lightComponent.SetColorLinear({-1.0F, 0.5F, 2.0F});
    lightComponent.SetIntensity(-1.0F);
    lightComponent.SetAmbientStrength(2.0F);
    RequireVectorNear(lightComponent.GetLight().GetDirection(), {0.0F, -1.0F, 0.0F}, "Light normalizes direction");
    RequireNear(lightComponent.GetLight().GetColorLinear().x, 0.0F, "Light clamps color minimum");
    RequireNear(lightComponent.GetLight().GetColorLinear().y, 0.5F, "Light stores color channel");
    RequireNear(lightComponent.GetLight().GetIntensity(), 0.0F, "Light clamps intensity");
    RequireNear(lightComponent.GetLight().GetAmbientStrength(), 1.0F, "Light clamps ambient strength");

    Scene scene;
    GameObject& sceneLight = scene.CreateObject("Scene Light Smoke");
    auto& sceneLightComponent = sceneLight.AddComponent<DirectionalLightComponent>();
    sceneLightComponent.SetIntensity(0.42F);
    sceneLightComponent.SetAmbientStrength(0.24F);
    RequireNear(scene.GetDirectionalLight().GetIntensity(), 0.42F, "Scene reads active light component");
    sceneLight.SetActive(false);
    RequireNear(scene.GetDirectionalLight().GetIntensity(), 1.0F, "Scene falls back when light component is inactive");
}

void RunMeshSmokeTests(ID3D12Device& device)
{
    std::shared_ptr<Mesh> cubeMesh = Mesh::CreateCube(device, 2.0F);
    Require(cubeMesh != nullptr, "Mesh creates cube");
    Require(cubeMesh->HasIndices(), "Cube mesh uses indices");
    Require(cubeMesh->GetLocalTriangles().size() == 12U, "Cube mesh stores local triangles");
    RequireNear(cubeMesh->GetLocalAabb().Extents.x, 1.0F, "Cube mesh local AABB x extent");
    RequireNear(cubeMesh->GetLocalAabb().Extents.y, 1.0F, "Cube mesh local AABB y extent");
    RequireNear(cubeMesh->GetLocalAabb().Extents.z, 1.0F, "Cube mesh local AABB z extent");

    GameObject meshObject("MeshComponent Smoke");
    auto& meshComponent = meshObject.AddComponent<MeshComponent>(cubeMesh);
    Require(meshComponent.GetMesh() == cubeMesh, "MeshComponent stores mesh reference");
    meshComponent.SetMesh(nullptr);
    Require(meshComponent.GetMesh() == nullptr, "MeshComponent accepts empty mesh for disabled rendering");
}

void RunPhysicsSmokeTests()
{
    GameObject bodyObject("Rigidbody Smoke");
    auto& rigidbody = bodyObject.AddComponent<RigidbodyComponent>();
    rigidbody.SetUseGravity(false);
    rigidbody.SetDragPerSec(0.0F);
    rigidbody.SetMassKg(2.0F);
    rigidbody.AddForce({2.0F, 0.0F, 0.0F}, ForceMode::Force);
    rigidbody.Update(1.0F);
    RequireNear(rigidbody.GetVelocityMps().x, 1.0F, "Rigidbody integrates force");
    RequireNear(bodyObject.GetTransform().GetPositionM().x, 1.0F, "Rigidbody translates owner");
    rigidbody.AddForce({2.0F, 0.0F, 0.0F}, ForceMode::Impulse);
    RequireNear(rigidbody.GetVelocityMps().x, 2.0F, "Rigidbody applies impulse by mass");
    rigidbody.AddForce({0.0F, 1.0F, 0.0F}, ForceMode::VelocityChange);
    RequireNear(rigidbody.GetVelocityMps().y, 1.0F, "Rigidbody applies velocity change");

    GameObject boxObject("BoxCollider Smoke");
    boxObject.GetTransform().SetPositionM({2.0F, 0.0F, 0.0F});
    auto& boxCollider = boxObject.AddComponent<BoxColliderComponent>(DirectX::XMFLOAT3{0.0F, 0.0F, 0.0F},
                                                                     DirectX::XMFLOAT3{2.0F, 2.0F, 2.0F});
    Require(boxCollider.GetType() == ColliderType::Box, "BoxCollider reports collider type");
    RequireNear(boxCollider.GetWorldAabb().Center.x, 2.0F, "BoxCollider follows owner transform");
    float hitDistanceM = 0.0F;
    Require(boxCollider.Raycast({2.0F, 0.0F, -5.0F}, {0.0F, 0.0F, 1.0F}, hitDistanceM),
            "BoxCollider raycast hits");
    RequireNear(hitDistanceM, 4.0F, "BoxCollider raycast distance");

    GameObject otherBoxObject("BoxCollider Overlap Smoke");
    otherBoxObject.GetTransform().SetPositionM({3.0F, 0.0F, 0.0F});
    auto& otherBoxCollider = otherBoxObject.AddComponent<BoxColliderComponent>(DirectX::XMFLOAT3{0.0F, 0.0F, 0.0F},
                                                                               DirectX::XMFLOAT3{2.0F, 2.0F, 2.0F});
    CollisionManager collisionManager;
    ContactInfo contact = {};
    Require(collisionManager.CheckCollision(boxCollider, otherBoxCollider, contact), "CollisionManager detects box overlap");
    Require(contact.penetrationM > 0.0F, "CollisionManager reports penetration");
    collisionManager.AddCollider(boxCollider);
    collisionManager.AddCollider(otherBoxCollider);
    ColliderComponent* hitCollider = nullptr;
    Require(collisionManager.Raycast({2.0F, 0.0F, -5.0F},
                                     {0.0F, 0.0F, 1.0F},
                                     hitDistanceM,
                                     hitCollider),
            "CollisionManager raycast hits registered collider");
    Require(hitCollider == &boxCollider, "CollisionManager returns nearest collider");

    std::shared_ptr<TerrainHeightMap> heightMap = TerrainHeightMap::CreateFlat(3U, 3U, 1.0F, 0.0F);
    Require(heightMap->ContainsSamplePositionM(1.0F, 1.0F), "TerrainHeightMap contains sample position");
    RequireNear(heightMap->SampleHeightM(0.5F, 0.5F), 0.0F, "TerrainHeightMap samples flat height");
    RequireVectorNear(heightMap->SampleNormal(0.5F, 0.5F), {0.0F, 1.0F, 0.0F}, "TerrainHeightMap samples up normal");

    GameObject terrainObject("TerrainCollider Smoke");
    auto& terrainCollider = terrainObject.AddComponent<TerrainColliderComponent>(heightMap);
    float terrainHeightM = 0.0F;
    DirectX::XMFLOAT3 terrainNormal = {};
    Require(terrainCollider.GetHeightAtWorld({0.0F, 5.0F, 0.0F}, terrainHeightM, terrainNormal),
            "TerrainCollider samples world height");
    RequireNear(terrainHeightM, 0.0F, "TerrainCollider reports flat world height");
    RequireVectorNear(terrainNormal, {0.0F, 1.0F, 0.0F}, "TerrainCollider reports flat world normal");
    Require(terrainCollider.Raycast({0.0F, 5.0F, 0.0F}, {0.0F, -1.0F, 0.0F}, hitDistanceM),
            "TerrainCollider raycast hits flat terrain");
    RequireNear(hitDistanceM, 4.75F, "TerrainCollider raycast midpoint distance");

    GameObject terrainBoxObject("Terrain Box Contact Smoke");
    terrainBoxObject.GetTransform().SetPositionM({0.0F, 0.4F, 0.0F});
    auto& terrainBoxCollider =
        terrainBoxObject.AddComponent<BoxColliderComponent>(DirectX::XMFLOAT3{0.0F, 0.0F, 0.0F},
                                                            DirectX::XMFLOAT3{1.0F, 1.0F, 1.0F});
    Require(collisionManager.CheckCollision(terrainCollider, terrainBoxCollider, contact),
            "CollisionManager detects terrain-box contact");
    Require(contact.isTerrainContact, "Terrain-box contact is flagged");

    GameObject capsuleObject("CapsuleCollider Smoke");
    capsuleObject.GetTransform().SetPositionM({0.0F, 1.0F, 0.0F});
    auto& capsuleCollider =
        capsuleObject.AddComponent<CapsuleColliderComponent>(DirectX::XMFLOAT3{0.0F, 0.0F, 0.0F}, 0.5F, 2.0F);
    Require(capsuleCollider.GetType() == ColliderType::Capsule, "CapsuleCollider reports collider type");
    RequireNear(capsuleCollider.GetWorldAabb().Center.y, 1.0F, "CapsuleCollider follows owner transform");
    Require(capsuleCollider.Raycast({0.0F, 1.0F, -5.0F}, {0.0F, 0.0F, 1.0F}, hitDistanceM),
            "CapsuleCollider raycast hits");
    RequireNear(hitDistanceM, 4.5F, "CapsuleCollider raycast distance");

    GameObject otherCapsuleObject("CapsuleCollider Overlap Smoke");
    otherCapsuleObject.GetTransform().SetPositionM({0.8F, 1.0F, 0.0F});
    auto& otherCapsuleCollider =
        otherCapsuleObject.AddComponent<CapsuleColliderComponent>(DirectX::XMFLOAT3{0.0F, 0.0F, 0.0F}, 0.5F, 2.0F);
    Require(collisionManager.CheckCollision(capsuleCollider, otherCapsuleCollider, contact),
            "CollisionManager detects capsule overlap");
    Require(contact.penetrationM > 0.0F, "CollisionManager reports capsule penetration");

    GameObject terrainCapsuleObject("Terrain Capsule Contact Smoke");
    terrainCapsuleObject.GetTransform().SetPositionM({0.0F, 0.4F, 0.0F});
    auto& terrainCapsuleCollider =
        terrainCapsuleObject.AddComponent<CapsuleColliderComponent>(DirectX::XMFLOAT3{0.0F, 0.0F, 0.0F}, 0.25F, 1.0F);
    Require(collisionManager.CheckCollision(terrainCollider, terrainCapsuleCollider, contact),
            "CollisionManager detects terrain-capsule contact");
    Require(contact.isTerrainContact, "Terrain-capsule contact is flagged");
}

void RunInputAndGameplaySmokeTests()
{
    InputManager inputManager;
    inputManager.BeginFrame();
    inputManager.SetKeyDown(InputKey::MoveForward, true);
    inputManager.SetKeyDown(InputKey::MoveRight, true);

    const InputState inputState = inputManager.GetState();
    RequireNear(inputState.mMoveAxis.x, 0.707106F, "InputManager normalizes diagonal x");
    RequireNear(inputState.mMoveAxis.y, 0.707106F, "InputManager normalizes diagonal y");

    GameObject playerObject("PlayerController Smoke");
    auto& playerController = playerObject.AddComponent<PlayerControllerComponent>(inputManager);
    playerController.SetJumpEnabled(true);
    auto& rigidbody = playerObject.AddComponent<RigidbodyComponent>();
    rigidbody.SetUseGravity(false);
    rigidbody.SetDragPerSec(0.0F);
    rigidbody.SetGroundFrictionPerSec(0.0F);
    playerObject.Update(1.0F);

    const float expectedDiagonalVelocityMps = PlayerControllerSettings::DEFAULT_MOVE_SPEED_MPS * 0.707106F;
    RequireNear(rigidbody.GetVelocityMps().x, expectedDiagonalVelocityMps, "PlayerController writes x velocity");
    RequireNear(rigidbody.GetVelocityMps().z, expectedDiagonalVelocityMps, "PlayerController writes z velocity");
    RequireNear(playerObject.GetTransform().GetPositionM().x,
                expectedDiagonalVelocityMps,
                "PlayerController moves owner through Rigidbody x");
    RequireNear(playerObject.GetTransform().GetPositionM().z,
                expectedDiagonalVelocityMps,
                "PlayerController moves owner through Rigidbody z");

    inputManager.BeginFrame();
    inputManager.SetKeyDown(InputKey::MoveForward, false);
    inputManager.SetKeyDown(InputKey::MoveRight, false);
    inputManager.SetKeyDown(InputKey::Jump, true);
    rigidbody.SetGrounded(true);
    playerController.Update(0.0F);
    RequireNear(rigidbody.GetVelocityMps().y,
                PlayerControllerSettings::DEFAULT_JUMP_VELOCITY_MPS,
                "PlayerController applies jump velocity");
    Require(!rigidbody.IsGrounded(), "PlayerController clears grounded after jump");
}

void RunShaderSmokeTests()
{
    Require(RenderRootParameter::SCENE_CONSTANTS_32BIT_COUNT == EXPECTED_SCENE_CONSTANT_32BIT_COUNT,
            "Scene root constant count matches HLSL cbuffer");
    Require(RenderRootParameter::OBJECT_CONSTANTS_32BIT_COUNT == EXPECTED_OBJECT_CONSTANT_32BIT_COUNT,
            "Object root constant count matches HLSL cbuffer");

    const auto vertexShader = ShaderCompiler::CompileFromFile(ShaderLibrary::GetLitColorShaderPath(),
                                                              "VSMain",
                                                              "vs_5_0");
    const auto pixelShader = ShaderCompiler::CompileFromFile(ShaderLibrary::GetLitColorShaderPath(),
                                                             "PSMain",
                                                             "ps_5_0");
    Require(vertexShader != nullptr && vertexShader->GetBufferSize() > 0U, "HLSL vertex shader compiles");
    Require(pixelShader != nullptr && pixelShader->GetBufferSize() > 0U, "HLSL pixel shader compiles");
}
} // namespace

void RunClientComponentSmokeTests(ID3D12Device& device)
{
#if defined(_DEBUG)
    RunCoreSmokeTests();
    RunCameraSmokeTests();
    RunMaterialAndLightSmokeTests();
    RunMeshSmokeTests(device);
    RunPhysicsSmokeTests();
    RunInputAndGameplaySmokeTests();
    RunShaderSmokeTests();
#else
    (void)device;
#endif
}
} // namespace Kimgane::Engine::Diagnostics
