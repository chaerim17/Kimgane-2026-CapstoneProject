#include "Pch.h"

#include "Scene.h"

#include "../Camera/CameraComponent.h"
#include "../Camera/CameraSettings.h"
#include "../Gameplay/NetworkSmoothingComponent.h"
#include "../Gameplay/PlayerControllerComponent.h"
#include "../../Engine/Network/NetworkManager.h"
#include "../Input/InputManager.h"
#include "../Physics/ColliderComponent.h"
#include "../Physics/TerrainColliderComponent.h"
#include "../Physics/RigidbodyComponent.h"
#include "../Rendering/LightComponent.h"
#include "../Rendering/MaterialComponent.h"
#include "../Rendering/MeshComponent.h"
#include "../Rendering/SceneRenderConstants.h"
#include "../Rendering/TextComponent.h"
#include "../Math/VectorMath.h"
#include "TestSceneSettings.h"
#include "../../Shared/Geometry/CollisionBoxLoader.h"
#include "../../Shared/Physics/CollisionResolver.h"

#include "../../Shared/Protocol.h"

#include <DirectXMath.h>
#include <iostream>

#include <utility>

namespace Kimgane::Engine
{
namespace Geometry = Kimgane::Shared::Geometry;

namespace
{
namespace SharedPhysics = Kimgane::Shared::Physics;
namespace SharedCollisionResolver = Kimgane::Shared::Physics::CollisionResolver;

const DirectX::XMFLOAT3 NO_EMISSION_LINEAR = {0.0F, 0.0F, 0.0F};
constexpr float UI_ORTHOGRAPHIC_HEIGHT_M = 9.0F;
constexpr float UI_CAMERA_DISTANCE_M = 10.0F;
constexpr float UI_NEAR_CLIP_M = 0.1F;
constexpr float UI_FAR_CLIP_M = 50.0F;
constexpr int LOCAL_PLAYER_COLLISION_SOLVER_ITERATIONS = 4;
constexpr float LOCAL_PLAYER_MIN_CORRECTION_SQ_M = 0.00000025F;
constexpr SharedCollisionResolver::PositionCorrectionSettings LOCAL_PLAYER_POSITION_CORRECTION_SETTINGS = {1.0F,
                                                                                                            0.001F};

float GetUiOrthographicWidthM(float cameraAspectRatio) noexcept
{
    return UI_ORTHOGRAPHIC_HEIGHT_M * std::max(cameraAspectRatio, 1.0F);
}

void ConfigureUiCamera(OrthographicCamera& camera, float cameraAspectRatio) noexcept
{
    camera.SetView({0.0F, 0.0F, -UI_CAMERA_DISTANCE_M}, {0.0F, 0.0F, 0.0F});
    camera.SetOrthographic(GetUiOrthographicWidthM(cameraAspectRatio),
                           UI_ORTHOGRAPHIC_HEIGHT_M,
                           UI_NEAR_CLIP_M,
                           UI_FAR_CLIP_M);
}

GameObject& CreateUiPanel(Scene& scene,
                          const std::shared_ptr<Mesh>& uiMesh,
                          std::string name,
                          const DirectX::XMFLOAT3& positionM,
                          const DirectX::XMFLOAT3& scale,
                          const DirectX::XMFLOAT4& colorLinear)
{
    GameObject& panel = scene.CreateObject(std::move(name));
    panel.GetTransform().SetPositionM(positionM);
    panel.GetTransform().SetScale(scale);
    panel.AddComponent<MeshComponent>(uiMesh);
    auto& material = panel.AddComponent<MaterialComponent>(colorLinear);
    material.GetMaterial().SetSurface(0.0F, 0.8F);
    material.GetMaterial().SetEmissionLinear({colorLinear.x, colorLinear.y, colorLinear.z}, 0.15F);
    return panel;
}

TextComponent& AddUiButtonLabel(GameObject& button, std::wstring text, float fontSizeDip)
{
    auto& label = button.AddComponent<TextComponent>(std::move(text), fontSizeDip);
    label.SetColorLinear({0.92F, 0.96F, 1.0F, 1.0F});
    label.SetInsetRatio(0.08F, 0.12F);
    label.SetAlignment(TextHorizontalAlignment::Center, TextVerticalAlignment::Center);
    return label;
}

std::size_t ToIndex(TitleMenuOption option) noexcept
{
    return static_cast<std::size_t>(option);
}

const wchar_t* GetTitleOptionLabelW(TitleMenuOption option) noexcept
{
    switch (option)
    {
    case TitleMenuOption::LocalGame:
        return L"로컬 테스트";
    case TitleMenuOption::OnlineGame:
        return L"서버 접속";
    case TitleMenuOption::Settings:
        return L"설정";
    default:
        return L"알 수 없음";
    }
}

const wchar_t* GetFpsToggleLabelW(bool enabled) noexcept
{
    return enabled ? L"FPS 표시 켜짐" : L"FPS 표시 꺼짐";
}

SharedPhysics::Vec3 ToSharedVec3(const DirectX::XMFLOAT3& value) noexcept
{
    return {value.x, value.y, value.z};
}

DirectX::XMFLOAT3 ToXMFloat3(const SharedPhysics::Vec3& value) noexcept
{
    return {value.x, value.y, value.z};
}

SharedPhysics::ContactInfo ToSharedContact(const ContactInfo& contact) noexcept
{
    SharedPhysics::ContactInfo sharedContact = {};
    sharedContact.normal = ToSharedVec3(contact.normal);
    sharedContact.surfaceNormal = ToSharedVec3(contact.surfaceNormal);
    sharedContact.penetrationM = contact.penetrationM;
    sharedContact.isTerrainContact = contact.isTerrainContact;
    sharedContact.isWalkable = contact.isWalkable;
    sharedContact.slopeAngleRad = contact.slopeAngleRad;
    return sharedContact;
}

bool IsPositionChanged(const SharedPhysics::Vec3& fromM, const SharedPhysics::Vec3& toM) noexcept
{
    return SharedPhysics::LengthSquared(SharedPhysics::Subtract(toM, fromM)) > LOCAL_PLAYER_MIN_CORRECTION_SQ_M;
}

bool RemoveVelocityIntoResolvedContact(SharedPhysics::Vec3& velocityMps,
                                       const SharedPhysics::ContactInfo& contact) noexcept
{
    if (!SharedCollisionResolver::ShouldBlockMovement(contact) &&
        !SharedCollisionResolver::IsWalkableGround(contact))
    {
        return false;
    }

    const SharedPhysics::Vec3 slidVelocityMps =
        SharedCollisionResolver::SlideMovement(velocityMps,
                                               contact,
                                               SharedCollisionResolver::ContactParticipant::ObjectB);
    if (!IsPositionChanged(velocityMps, slidVelocityMps))
    {
        return false;
    }

    velocityMps = slidVelocityMps;
    return true;
}

bool IsNpcObjectId(int objectId) noexcept
{
    return objectId >= MAX_PLAYERS && objectId < MAX_OBJECTS;
}

float DistanceSquaredM(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs) noexcept
{
    const DirectX::XMFLOAT3 deltaM = VectorMath::Subtract(lhs, rhs);
    return VectorMath::Dot(deltaM, deltaM);
}

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

void TitleScene::Build(std::shared_ptr<Mesh> uiMesh, const InputManager& inputManager, float cameraAspectRatio)
{
    Clear();
    mInputManager = &inputManager;
    mSelectedOption = TitleMenuOption::LocalGame;
    mPendingAction = TitleSceneAction::None;
    mOptionPanels.fill(nullptr);
    mOptionLabels.fill(nullptr);

    ConfigureUiCamera(mUiCamera, cameraAspectRatio);

    const float uiWidthM = GetUiOrthographicWidthM(cameraAspectRatio);
    CreateUiPanel(*this,
                  uiMesh,
                  "Title Background",
                  {0.0F, 0.0F, 1.0F},
                  {uiWidthM, UI_ORTHOGRAPHIC_HEIGHT_M, 0.1F},
                  {0.05F, 0.07F, 0.10F, 1.0F});
    CreateUiPanel(*this, uiMesh, "Title Header", {0.0F, 2.9F, 0.0F}, {6.8F, 1.2F, 0.1F}, {0.78F, 0.22F, 0.18F, 1.0F});

    GameObject& localGameOption =
        CreateUiPanel(*this, uiMesh, "Title Option Local", {0.0F, 1.1F, 0.0F}, {4.8F, 0.8F, 0.1F}, {0.12F, 0.18F, 0.25F, 1.0F});
    mOptionPanels[ToIndex(TitleMenuOption::LocalGame)] = &localGameOption;
    mOptionLabels[ToIndex(TitleMenuOption::LocalGame)] =
        &AddUiButtonLabel(localGameOption, GetTitleOptionLabelW(TitleMenuOption::LocalGame), 32.0F);

    GameObject& onlineGameOption =
        CreateUiPanel(*this, uiMesh, "Title Option Online", {0.0F, 0.0F, 0.0F}, {4.8F, 0.8F, 0.1F}, {0.12F, 0.18F, 0.25F, 1.0F});
    mOptionPanels[ToIndex(TitleMenuOption::OnlineGame)] = &onlineGameOption;
    mOptionLabels[ToIndex(TitleMenuOption::OnlineGame)] =
        &AddUiButtonLabel(onlineGameOption, GetTitleOptionLabelW(TitleMenuOption::OnlineGame), 32.0F);

    GameObject& settingsOption =
        CreateUiPanel(*this, uiMesh, "Title Option Settings", {0.0F, -1.1F, 0.0F}, {4.8F, 0.8F, 0.1F}, {0.12F, 0.18F, 0.25F, 1.0F});
    mOptionPanels[ToIndex(TitleMenuOption::Settings)] = &settingsOption;
    mOptionLabels[ToIndex(TitleMenuOption::Settings)] =
        &AddUiButtonLabel(settingsOption, GetTitleOptionLabelW(TitleMenuOption::Settings), 32.0F);

    RefreshVisualState();
}

void TitleScene::Update(float deltaTimeSec)
{
    (void)deltaTimeSec;

    if (mInputManager == nullptr)
    {
        return;
    }

    if (mInputManager->WasKeyPressed(InputKey::MenuUp))
    {
        MoveSelection(-1);
    }

    if (mInputManager->WasKeyPressed(InputKey::MenuDown))
    {
        MoveSelection(1);
    }

    if (mInputManager->WasKeyPressed(InputKey::Cancel))
    {
        mPendingAction = TitleSceneAction::OpenSettings;
    }

    if (mInputManager->WasKeyPressed(InputKey::Confirm))
    {
        switch (mSelectedOption)
        {
        case TitleMenuOption::LocalGame:
            mPendingAction = TitleSceneAction::StartLocalGame;
            break;
        case TitleMenuOption::OnlineGame:
            mPendingAction = TitleSceneAction::StartOnlineGame;
            break;
        case TitleMenuOption::Settings:
            mPendingAction = TitleSceneAction::OpenSettings;
            break;
        default:
            mPendingAction = TitleSceneAction::None;
            break;
        }
    }
}

const Camera* TitleScene::GetUiCamera() const noexcept
{
    return &mUiCamera;
}

TitleSceneAction TitleScene::ConsumePendingAction() noexcept
{
    const TitleSceneAction action = mPendingAction;
    mPendingAction = TitleSceneAction::None;
    return action;
}

TitleMenuOption TitleScene::GetSelectedOption() const noexcept
{
    return mSelectedOption;
}

const wchar_t* TitleScene::GetSelectedOptionLabelW() const noexcept
{
    return GetTitleOptionLabelW(mSelectedOption);
}

void TitleScene::MoveSelection(int direction) noexcept
{
    constexpr int OPTION_COUNT = static_cast<int>(TitleMenuOption::Count);
    int selectedIndex = static_cast<int>(mSelectedOption);
    selectedIndex = (selectedIndex + direction + OPTION_COUNT) % OPTION_COUNT;
    mSelectedOption = static_cast<TitleMenuOption>(selectedIndex);
    RefreshVisualState();
}

void TitleScene::RefreshVisualState() noexcept
{
    for (std::size_t index = 0; index < mOptionPanels.size(); ++index)
    {
        GameObject* panel = mOptionPanels[index];
        if (panel == nullptr)
        {
            continue;
        }

        auto* materialComponent = panel->GetComponent<MaterialComponent>();
        if (materialComponent == nullptr)
        {
            continue;
        }

        const bool selected = index == ToIndex(mSelectedOption);
        materialComponent->GetMaterial().SetBaseColorLinear(selected ? DirectX::XMFLOAT4{0.12F, 0.62F, 0.36F, 1.0F}
                                                                     : DirectX::XMFLOAT4{0.12F, 0.18F, 0.25F, 1.0F});

        TextComponent* label = mOptionLabels[index];
        if (label != nullptr)
        {
            label->SetColorLinear(selected ? DirectX::XMFLOAT4{1.0F, 1.0F, 1.0F, 1.0F}
                                           : DirectX::XMFLOAT4{0.72F, 0.78F, 0.86F, 1.0F});
        }
    }
}

const Camera* OverlayScene::GetUiCamera() const noexcept
{
    return &mUiCamera;
}

void OverlayScene::ConfigureOverlayCamera(float cameraAspectRatio) noexcept
{
    ConfigureUiCamera(mUiCamera, cameraAspectRatio);
}

void SettingsOverlayScene::Build(std::shared_ptr<Mesh> uiMesh,
                                 const InputManager& inputManager,
                                 bool fpsInWindowTitleEnabled,
                                 float cameraAspectRatio)
{
    Clear();
    mInputManager = &inputManager;
    mFpsInWindowTitleEnabled = fpsInWindowTitleEnabled;
    mCloseRequested = false;
    mFpsTogglePanel = nullptr;
    mFpsToggleLabel = nullptr;

    ConfigureOverlayCamera(cameraAspectRatio);

    const float uiWidthM = GetUiOrthographicWidthM(cameraAspectRatio);
    CreateUiPanel(*this,
                  uiMesh,
                  "Settings Dim Background",
                  {0.0F, 0.0F, 1.0F},
                  {uiWidthM, UI_ORTHOGRAPHIC_HEIGHT_M, 0.1F},
                  {0.0F, 0.0F, 0.0F, 0.45F});
    CreateUiPanel(*this, uiMesh, "Settings Panel", {0.0F, 0.0F, 0.0F}, {5.8F, 3.4F, 0.1F}, {0.10F, 0.14F, 0.20F, 0.86F});

    GameObject& fpsToggle =
        CreateUiPanel(*this, uiMesh, "Settings FPS Toggle", {0.0F, 0.55F, -0.1F}, {3.8F, 0.8F, 0.1F}, {0.12F, 0.62F, 0.36F, 0.95F});
    mFpsTogglePanel = &fpsToggle;
    mFpsToggleLabel = &AddUiButtonLabel(fpsToggle, GetFpsToggleLabelW(mFpsInWindowTitleEnabled), 28.0F);

    GameObject& closeHint =
        CreateUiPanel(*this, uiMesh, "Settings Close Hint", {0.0F, -1.05F, -0.1F}, {2.8F, 0.55F, 0.1F}, {0.78F, 0.22F, 0.18F, 0.95F});
    AddUiButtonLabel(closeHint, L"닫기", 24.0F);

    RefreshVisualState();
}

void SettingsOverlayScene::Update(float deltaTimeSec)
{
    (void)deltaTimeSec;

    if (mInputManager == nullptr)
    {
        return;
    }

    if (mInputManager->WasKeyPressed(InputKey::Confirm))
    {
        SetFpsInWindowTitleEnabled(!mFpsInWindowTitleEnabled);
    }

    if (mInputManager->WasKeyPressed(InputKey::Cancel))
    {
        mCloseRequested = true;
    }
}

void SettingsOverlayScene::SetFpsInWindowTitleEnabled(bool enabled) noexcept
{
    mFpsInWindowTitleEnabled = enabled;
    RefreshVisualState();
}

bool SettingsOverlayScene::IsFpsInWindowTitleEnabled() const noexcept
{
    return mFpsInWindowTitleEnabled;
}

bool SettingsOverlayScene::ConsumeCloseRequested() noexcept
{
    const bool closeRequested = mCloseRequested;
    mCloseRequested = false;
    return closeRequested;
}

void SettingsOverlayScene::RefreshVisualState() noexcept
{
    if (mFpsTogglePanel == nullptr)
    {
        return;
    }

    auto* materialComponent = mFpsTogglePanel->GetComponent<MaterialComponent>();
    if (materialComponent == nullptr)
    {
        return;
    }

    materialComponent->GetMaterial().SetBaseColorLinear(mFpsInWindowTitleEnabled ? DirectX::XMFLOAT4{0.12F, 0.62F, 0.36F, 0.95F}
                                                                                 : DirectX::XMFLOAT4{0.72F, 0.18F, 0.15F, 0.95F});

    if (mFpsToggleLabel != nullptr)
    {
        mFpsToggleLabel->SetText(GetFpsToggleLabelW(mFpsInWindowTitleEnabled));
        mFpsToggleLabel->SetColorLinear({1.0F, 1.0F, 1.0F, 1.0F});
    }
}

void GameScene::Build(std::shared_ptr<Mesh> cubeMesh,
                      std::shared_ptr<Mesh> playerModelMesh,        // 26.07.10 모델 메쉬 매개변수 추가
                      std::shared_ptr<Mesh> npcModelMesh,    // NPC 모델 메쉬 매개변수 추가
                      std::shared_ptr<Mesh> houseModelMesh,  // 집 모델 메쉬 매개변수 추가
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
    mHouseColliders.clear();
    mLocalPlayerCollisionTargets.clear();
    mIsLocalPlayerCollidingWithHouse = false; // 충돌처리 체크 초기화
    mPlayerMesh = playerModelMesh != nullptr ? std::move(playerModelMesh) : cubeMesh;       // 26.07.10 모델 메쉬가 없으면 큐브 메쉬를 사용
    mNpcMesh = npcModelMesh != nullptr ? std::move(npcModelMesh) : mPlayerMesh; // NPC 모델 메쉬가 없으면 플레이어 메쉬를 사용

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
    RegisterLocalPlayerCollisionTarget(terrainCollider);
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
    RegisterLocalPlayerCollisionTarget(boxCollider);

    mTestCube = &cube;

    GameObject& house = CreateObject("Test House");
    house.GetTransform().SetPositionM(TestSceneSettings::HOUSE_START_POSITION_M);
    house.AddComponent<MeshComponent>(houseModelMesh);
    auto& houseMaterial = house.AddComponent<MaterialComponent>(TestSceneSettings::HOUSE_MODEL_BASE_COLOR_LINEAR);
    houseMaterial.GetMaterial().SetSurface(0.0F, 0.85F);

    const std::vector<Geometry::NamedCollisionBox> houseCollisionBoxes =
        Geometry::CollisionBoxLoader::Load(TestSceneSettings::HOUSE_COLLISION_PATH);
    for (const Geometry::NamedCollisionBox& collisionBox : houseCollisionBoxes)
    {
        const DirectX::XMFLOAT3 centerM = ToXMFloat3(collisionBox.box.centerM);
        const DirectX::XMFLOAT3 sizeM = {collisionBox.box.halfExtentsM.x * 2.0F, collisionBox.box.halfExtentsM.y * 2.0F,
                                         collisionBox.box.halfExtentsM.z * 2.0F};
        auto& houseCollider = house.AddComponent<BoxColliderComponent>(centerM, sizeM);
        RegisterLocalPlayerCollisionTarget(houseCollider);
        mHouseColliders.push_back(&houseCollider); 
    }


    GameObject& localPlayer = CreateObject("Local Player");
    localPlayer.GetTransform().SetPositionM(TestSceneSettings::PLAYER_START_POSITION_M);
    localPlayer.AddComponent<MeshComponent>(mPlayerMesh);       // 26.07.10 모델 메쉬를 사용
    auto& playerMaterial = localPlayer.AddComponent<MaterialComponent>(     // 26.07.10 모델 메쉬를 사용하면 흰색, 아니면 녹색    
        mPlayerMesh == cubeMesh ? TestSceneSettings::PLAYER_BASE_COLOR_LINEAR
                                : TestSceneSettings::PLAYER_MODEL_BASE_COLOR_LINEAR);
    playerMaterial.GetMaterial().SetSurface(0.0F, 0.42F);
    auto& playerController = localPlayer.AddComponent<PlayerControllerComponent>(inputManager, networkManager);
    playerController.SetNetworkInputEnabled(UsesNetworkInput());
    auto& playerRigidbody = localPlayer.AddComponent<RigidbodyComponent>();
    playerRigidbody.SetUseGravity(true);
    playerRigidbody.SetDragPerSec(0.0F);
    playerRigidbody.SetGroundFrictionPerSec(0.0F);
    playerRigidbody.SetGrounded(true);
    auto& playerCollider =
        localPlayer.AddComponent<CapsuleColliderComponent>(DirectX::XMFLOAT3{0.0F, 0.0F, 0.0F},
                                                           TestSceneSettings::PLAYER_CAPSULE_RADIUS_M,
                                                           TestSceneSettings::PLAYER_CAPSULE_HEIGHT_M);
    GetCollisionManager().AddCollider(playerCollider);
    auto& cameraComponent = localPlayer.AddComponent<CameraComponent>(inputManager, TestSceneSettings::PLAYER_CAMERA_TARGET_OFFSET_M);
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

void GameScene::Update(float deltaTimeSec)
{
    if (mTestCube != nullptr)
    {
        mCubeRotationRad += deltaTimeSec * 0.8F;
        mTestCube->GetTransform().SetRotationRad({DirectX::XMConvertToRadians(24.0F), mCubeRotationRad, 0.0F});
    }

    Scene::Update(deltaTimeSec);
    ResolveLocalPlayerCollisions();

    const std::vector<ContactInfo> houseContacts = CheckLocalPlayerHouseCollision();
    const bool isCollidingNow = !houseContacts.empty();
    if (isCollidingNow && !mIsLocalPlayerCollidingWithHouse)
    {
        const DirectX::XMFLOAT3 playerPositionM = mLocalPlayer->GetTransform().GetPositionM();
        std::cout << "[Collision] TestHouse and Player, player location(" << playerPositionM.x << ", " << playerPositionM.y << ", "
                  << playerPositionM.z << "), " << houseContacts.size() << " box(es)\n";
    }
    mIsLocalPlayerCollidingWithHouse = isCollidingNow;
}

void GameScene::RegisterLocalPlayerCollisionTarget(ColliderComponent& collider)
{
    GetCollisionManager().AddCollider(collider);

    if (std::find(mLocalPlayerCollisionTargets.begin(), mLocalPlayerCollisionTargets.end(), &collider) ==
        mLocalPlayerCollisionTargets.end())
    {
        mLocalPlayerCollisionTargets.push_back(&collider);
    }
}

std::vector<ContactInfo> GameScene::QueryLocalPlayerContacts(CapsuleColliderComponent& playerCollider)
{
    std::vector<ContactInfo> contacts;
    playerCollider.Update(0.0F);

    for (ColliderComponent* targetCollider : mLocalPlayerCollisionTargets)
    {
        if (targetCollider == nullptr || targetCollider == &playerCollider || !targetCollider->GetOwner().IsActive())
        {
            continue;
        }

        targetCollider->Update(0.0F);
        ContactInfo contact = {};
        if (GetCollisionManager().CheckCollision(*targetCollider, playerCollider, contact))
        {
            contacts.push_back(contact);
        }
    }

    return contacts;
}

void GameScene::ResolveLocalPlayerCollisions()
{
    if (mLocalPlayer == nullptr)
    {
        return;
    }

    auto* playerCollider = mLocalPlayer->GetComponent<CapsuleColliderComponent>();
    if (playerCollider == nullptr)
    {
        return;
    }

    auto* playerRigidbody = mLocalPlayer->GetComponent<RigidbodyComponent>();
    SharedPhysics::Vec3 resolvedPositionM = ToSharedVec3(mLocalPlayer->GetTransform().GetPositionM());
    SharedPhysics::Vec3 resolvedVelocityMps =
        playerRigidbody != nullptr ? ToSharedVec3(playerRigidbody->GetVelocityMps()) : SharedPhysics::Vec3{};

    bool positionChanged = false;
    bool velocityChanged = false;
    bool groundedOnWalkableSurface = false;

    for (int iteration = 0; iteration < LOCAL_PLAYER_COLLISION_SOLVER_ITERATIONS; ++iteration)
    {
        mLocalPlayer->GetTransform().SetPositionM(ToXMFloat3(resolvedPositionM));
        const std::vector<ContactInfo> contacts = QueryLocalPlayerContacts(*playerCollider);
        if (contacts.empty())
        {
            break;
        }

        bool iterationChanged = false;
        for (const ContactInfo& contact : contacts)
        {
            const SharedPhysics::ContactInfo sharedContact = ToSharedContact(contact);
            const bool isWalkableGround = SharedCollisionResolver::IsWalkableGround(sharedContact);
            const bool blocksMovement = SharedCollisionResolver::ShouldBlockMovement(sharedContact);
            if (!isWalkableGround && !blocksMovement)
            {
                continue;
            }

            groundedOnWalkableSurface = groundedOnWalkableSurface || isWalkableGround;

            const SharedPhysics::Vec3 previousPositionM = resolvedPositionM;
            resolvedPositionM =
                SharedCollisionResolver::ResolvePosition(resolvedPositionM,
                                                         sharedContact,
                                                         SharedCollisionResolver::ContactParticipant::ObjectB,
                                                         LOCAL_PLAYER_POSITION_CORRECTION_SETTINGS);
            if (IsPositionChanged(previousPositionM, resolvedPositionM))
            {
                positionChanged = true;
                iterationChanged = true;
            }

            velocityChanged = RemoveVelocityIntoResolvedContact(resolvedVelocityMps, sharedContact) || velocityChanged;
        }

        if (!iterationChanged)
        {
            break;
        }
    }

    if (playerRigidbody != nullptr)
    {
        SharedPhysics::RigidbodyState state = playerRigidbody->GetSharedState();
        const bool groundedChanged = state.isGrounded != groundedOnWalkableSurface;
        if (positionChanged || velocityChanged || groundedChanged)
        {
            state.positionM = resolvedPositionM;
            state.velocityMps = resolvedVelocityMps;
            state.isGrounded = groundedOnWalkableSurface;
            playerRigidbody->SetSharedState(state);
            playerCollider->Update(0.0F);
        }

        return;
    }

    if (positionChanged || velocityChanged || groundedOnWalkableSurface)
    {
        mLocalPlayer->GetTransform().SetPositionM(ToXMFloat3(resolvedPositionM));
        playerCollider->Update(0.0F);
    }
}

void GameScene::RefreshGameplayCamera() noexcept
{
    if (mGameplayCamera != nullptr)
    {
        mGameplayCamera->Refresh();
    }
}

const Camera* GameScene::GetGameplayCamera() const noexcept
{
    if (mGameplayCamera == nullptr)
    {
        return nullptr;
    }

    return &mGameplayCamera->GetCamera();
}

DirectX::XMFLOAT3 GameScene::GetCameraTargetPositionM() const noexcept
{
    if (mLocalPlayer == nullptr)
    {
        return TestSceneSettings::CAMERA_LOOK_AT_POSITION_M;
    }

    return VectorMath::Add(mLocalPlayer->GetTransform().GetPositionM(), TestSceneSettings::PLAYER_CAMERA_TARGET_OFFSET_M);
}

DirectX::XMFLOAT3 GameScene::GetLocalPlayerPositionM() const noexcept
{
    if (mLocalPlayer == nullptr)
    {
        return TestSceneSettings::PLAYER_START_POSITION_M;
    }

    return mLocalPlayer->GetTransform().GetPositionM();
}

float GameScene::GetLocalPlayerYaw() const noexcept
{
    if (mLocalPlayer == nullptr)
    {
        return 0.0F;
    }

    return mLocalPlayer->GetTransform().GetRotationRad().y;
}
/// ----------------------------------------------------------------------------------
std::vector<ContactInfo> GameScene::CheckLocalPlayerHouseCollision() // 충돌처리 체크
{
    std::vector<ContactInfo> contacts;

    if (mLocalPlayer == nullptr) // 예외처리
    {
        return contacts;
    }

    auto* playerCollider = mLocalPlayer->GetComponent<CapsuleColliderComponent>();
    if (playerCollider == nullptr) // 예외처리
    {
        return contacts;
    }

    playerCollider->Update(0.0F);

    for (BoxColliderComponent* houseCollider : mHouseColliders) // TestHouse에 있는 박스 콜라이더들 꺼내서 충돌 체크
    {
        if (houseCollider == nullptr) // 예외처리
        {
            continue;
        }

        ContactInfo contact = {}; // 충돌 정보 담는 구조체
        if (GetCollisionManager().CheckCollision(*houseCollider, *playerCollider, contact))
        {
            contacts.push_back(contact);
        }
    }

    return contacts;
}
/// ----------------------------------------------------------------------------------
void GameScene::UpdateNetworkPlayerPosition(int playerId, const DirectX::XMFLOAT3& positionM, float yaw)
{
    const bool isNpc = IsNpcObjectId(playerId);

    if (mNetworkManager != nullptr && playerId == mNetworkManager->GetMyPlayerId())
    {
        if (mLocalPlayer == nullptr)
        {
            return;
        }
        CorrectLocalPlayerState(positionM, yaw);
        return;
    }

    auto iter = mNetworkPlayers.find(playerId);

    if (iter == mNetworkPlayers.end())
    {
        GameObject& networkPlayer = CreateNetworkPlayer(playerId, positionM);
        auto rotation = networkPlayer.GetTransform().GetRotationRad();
        rotation.y = yaw;
        networkPlayer.GetTransform().SetRotationRad(rotation);

        mNetworkPlayers[playerId] = &networkPlayer;
        return;
    }

    if (isNpc)
    {
        auto* smoothing = iter->second->GetComponent<NetworkSmoothingComponent>();
        if (smoothing != nullptr)
        {
            smoothing->SetTargetPositionM(positionM);
        }
        else
        {
            iter->second->GetTransform().SetPositionM(positionM);
        }
    }
    else
    {
        iter->second->GetTransform().SetPositionM(positionM);
    }

    auto rotation = iter->second->GetTransform().GetRotationRad();
    rotation.y = yaw;
    iter->second->GetTransform().SetRotationRad(rotation);
}

void GameScene::RemoveNetworkPlayer(int playerId)
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

GameObject& GameScene::CreateNetworkPlayer(int playerId, const DirectX::XMFLOAT3& positionM)
{
    const bool isNpc = IsNpcObjectId(playerId);

    GameObject& networkPlayer = CreateObject((isNpc ? "NPC " : "Network Player ") + std::to_string(playerId));
    networkPlayer.GetTransform().SetPositionM(positionM);
    networkPlayer.AddComponent<MeshComponent>(isNpc ? mNpcMesh : mPlayerMesh);
    auto& material = networkPlayer.AddComponent<MaterialComponent>(
        isNpc ? TestSceneSettings::NPC_MODEL_BASE_COLOR_LINEAR : TestSceneSettings::NETWORK_PLAYER_BASE_COLOR_LINEAR);
    material.GetMaterial().SetSurface(0.0F, 0.48F);

    if (isNpc)
    {
        auto& smoothing = networkPlayer.AddComponent<NetworkSmoothingComponent>();
        smoothing.SetMoveSpeedMps(TestSceneSettings::NPC_VISUAL_MOVE_SPEED_MPS);
        smoothing.SnapToPositionM(positionM);
    }

    return networkPlayer;
}

void GameScene::CorrectLocalPlayerState(const DirectX::XMFLOAT3& authoritativePositionM,
                                        float authoritativeYaw) noexcept
{
    if (mLocalPlayer == nullptr)
    {
        return;
    }

    const DirectX::XMFLOAT3 currentPositionM = mLocalPlayer->GetTransform().GetPositionM();
    const float errorSqM = DistanceSquaredM(currentPositionM, authoritativePositionM);

    if (errorSqM > 0.0F)
    {
        // 서버 요청: 본인 예측 위치가 서버 권위 위치와 다르면 서버 위치로 즉시 보정합니다.
        if (auto* playerRigidbody = mLocalPlayer->GetComponent<RigidbodyComponent>())
        {
            SharedPhysics::RigidbodyState state = playerRigidbody->GetSharedState();
            state.positionM = ToSharedVec3(authoritativePositionM);
            playerRigidbody->SetSharedState(state);
        }
        else
        {
            mLocalPlayer->GetTransform().SetPositionM(authoritativePositionM);
        }
    }

    auto rotationRad = mLocalPlayer->GetTransform().GetRotationRad();
    rotationRad.y = authoritativeYaw;
    mLocalPlayer->GetTransform().SetRotationRad(rotationRad);
}

bool LocalGameScene::UsesNetworkInput() const noexcept
{
    return false;
}

bool OnlineGameScene::UsesNetworkInput() const noexcept
{
    return true;
}
} // namespace Kimgane::Engine
