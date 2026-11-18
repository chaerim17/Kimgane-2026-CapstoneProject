#include "Pch.h"

#include "Scene.h"
#include "../Terrain/LunarTerrainRenderer.h"

#include "../Camera/CameraSettings.h"
#include "../Gameplay/PlayerControllerSettings.h"
#include "../Network/NetworkManager.h"
#include "../Input/InputManager.h"
#include "../Input/InputSettings.h"
#include "../../Shared/Physics/CharacterMovementWorld.h"
#include "../Rendering/DamageNumberComponent.h"
#include "../Rendering/HealthBarComponent.h"
#include "../Rendering/SceneRenderConstants.h"
#include "../Rendering/TextComponent.h"
#include "../Core/VectorMath.h"
#include "TestSceneSettings.h"
#include "../../Shared/Geometry/CollisionBoxLoader.h"
#include "../../Shared/Physics/RaycastQueries.h"

#include "../../Shared/Protocol.h"

#include <DirectXMath.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>

#include <utility>

namespace Kimgane::Client
{
namespace Geometry = Kimgane::Shared::Geometry;

namespace
{
namespace SharedPhysics = Kimgane::Shared::Physics;
namespace MapSettings = Kimgane::Shared::World::TestMapSettings;
namespace SharedRaycast = Kimgane::Shared::Physics::RaycastQueries;

constexpr float UI_ORTHOGRAPHIC_HEIGHT_M = 9.0F;
constexpr float UI_CAMERA_DISTANCE_M = 10.0F;
constexpr float UI_NEAR_CLIP_M = 0.1F;
constexpr float UI_FAR_CLIP_M = 50.0F;
constexpr float SHOOT_MAX_RANGE_M = 100.0F;
// ServerConfig.h의 ITEM_BOX_OPEN_DISTANCE_M과 동일하게 값을 유지.
constexpr float ITEM_BOX_OPEN_DISTANCE_M = 3.0F;

const char* ToItemName(Kimgane::Shared::Items::ItemId itemId) noexcept
{
    using ItemId = Kimgane::Shared::Items::ItemId;
    switch (itemId)
    {
    case ItemId::HpPotion:
        return "HpPotion";
    case ItemId::Chip:
        return "Chip";
    case ItemId::Armor:
        return "Armor";
    default:
        return "None";
    }
}

const char* ToOpenItemBoxStatusName(OpenItemBoxStatus status) noexcept
{
    switch (status)
    {
    case OpenItemBoxStatus::Success:
        return "Success";
    case OpenItemBoxStatus::NotFound:
        return "NotFound";
    case OpenItemBoxStatus::NotItemBox:
        return "NotItemBox";
    case OpenItemBoxStatus::TooFar:
        return "TooFar";
    case OpenItemBoxStatus::RewardFailed:
        return "RewardFailed";
    default:
        return "Unknown";
    }
}

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
    panel.SetMesh(uiMesh);
    Material& material = panel.GetMaterial();
    material.SetBaseColorLinear(colorLinear);
    material.SetSurface(0.0F, 0.8F);
    material.SetEmissionLinear({colorLinear.x, colorLinear.y, colorLinear.z}, 0.15F);
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

} // namespace

void Scene::Clear() noexcept
{
    mTerrainRenderer.reset();
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
}

void Scene::Render(ID3D12GraphicsCommandList& commandList,
                   D3D12_GPU_DESCRIPTOR_HANDLE defaultTextureGpuHandle,
                   MeshPrimitiveTopology primitiveTopology) const
{
    for (const auto& object : mObjects)
    {
        if (!object || !object->IsActive())
        {
            continue;
        }

        const std::shared_ptr<Mesh>& mesh = object->GetMesh();
        if (!mesh || mesh->GetPrimitiveTopology() != primitiveTopology)
        {
            continue;
        }

        const Material& material = object->GetMaterial();
        ObjectShaderConstants objectConstants = {};
        objectConstants.world = GetRenderWorldMatrix(*object);
        objectConstants.baseColor = material.GetBaseColorLinear();
        objectConstants.surface = {material.GetMetallic(), material.GetRoughness(), 0.0F, 0.0F};
        objectConstants.emission = material.GetEmissionLinear();
        commandList.SetGraphicsRoot32BitConstants(RenderRootParameter::OBJECT,
                                                  RenderRootParameter::OBJECT_CONSTANTS_32BIT_COUNT,
                                                  &objectConstants,
                                                  0);

        // 텍스처가 없는 머티리얼은 기본 흰색 텍스처를 대신 바인딩 (셰이더가 항상 t0을 읽을 수 있어야 함)
        D3D12_GPU_DESCRIPTOR_HANDLE textureHandle = material.GetTextureGpuHandle();
        if (textureHandle.ptr == 0)
        {
            textureHandle = defaultTextureGpuHandle;
        }
        commandList.SetGraphicsRootDescriptorTable(RenderRootParameter::TEXTURE, textureHandle);

        mesh->Render(commandList);
    }
}

DirectX::XMFLOAT4X4 Scene::GetRenderWorldMatrix(const GameObject& object) const noexcept
{
    return object.GetTransform().GetWorldMatrix4x4();
}

const std::vector<std::unique_ptr<GameObject>>& Scene::GetObjects() const noexcept
{
    return mObjects;
}

const DirectionalLight& Scene::GetDirectionalLight() const noexcept
{
    return mDirectionalLight;
}

DirectionalLight& Scene::GetDirectionalLight() noexcept
{
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

        const bool selected = index == ToIndex(mSelectedOption);
        panel->GetMaterial().SetBaseColorLinear(selected ? DirectX::XMFLOAT4{0.12F, 0.62F, 0.36F, 1.0F}
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

    mFpsTogglePanel->GetMaterial().SetBaseColorLinear(mFpsInWindowTitleEnabled ? DirectX::XMFLOAT4{0.12F, 0.62F, 0.36F, 0.95F}
                                                                                 : DirectX::XMFLOAT4{0.72F, 0.18F, 0.15F, 0.95F});

    if (mFpsToggleLabel != nullptr)
    {
        mFpsToggleLabel->SetText(GetFpsToggleLabelW(mFpsInWindowTitleEnabled));
        mFpsToggleLabel->SetColorLinear({1.0F, 1.0F, 1.0F, 1.0F});
    }
}

void GameScene::Build(std::shared_ptr<Mesh> cubeMesh,
                      ID3D12Device& device,
                      std::shared_ptr<Mesh> playerModelMesh,        // 모델 메쉬 매개변수 추가
                      D3D12_GPU_DESCRIPTOR_HANDLE playerTextureGpuHandle, // 플레이어 텍스처 SRV 핸들
                      std::shared_ptr<Mesh> npcModelMesh,    // NPC 모델 메쉬 매개변수 추가
                      D3D12_GPU_DESCRIPTOR_HANDLE npcTextureGpuHandle, // NPC 텍스처 SRV 핸들
                      const std::shared_ptr<LunarEnvironmentAssets>& environmentAssets,
                      D3D12_GPU_DESCRIPTOR_HANDLE houseTextureGpuHandle, // 집 텍스처 SRV 핸들
                      std::shared_ptr<Mesh> itemBoxBodyMesh,       // 아이템 박스 본체 메쉬
                      std::shared_ptr<Mesh> itemBoxLidMesh,        // 아이템 박스 뚜껑 메쉬
                      D3D12_GPU_DESCRIPTOR_HANDLE itemBoxTextureGpuHandle, // 아이템 박스 텍스처 SRV 핸들
                      std::shared_ptr<Mesh> mutationPlayerMesh,    // 중간 변이 플레이어 메쉬
                      D3D12_GPU_DESCRIPTOR_HANDLE mutationPlayerTextureGpuHandle, // 중간 변이 플레이어 텍스처 SRV 핸들
                      std::shared_ptr<Mesh> purifierMesh,          // 정화기 메쉬
                      D3D12_GPU_DESCRIPTOR_HANDLE purifierTextureGpuHandle, // 정화기 텍스처 SRV 핸들
                      std::shared_ptr<LunarTerrainRenderer> terrainRenderer,
                      std::shared_ptr<const TerrainHeightMap> terrainHeightMap,
                      const InputManager& inputManager,
                      NetworkManager& networkManager,
                      float cameraAspectRatio)
{
    if (!terrainHeightMap)
    {
        throw std::invalid_argument("GameScene requires shared terrain data");
    }
    Clear();
    mColliderDebugDraw.Initialize(device, terrainHeightMap, ToXMFloat3(MapSettings::TERRAIN_POSITION_M));
    mNetworkManager = &networkManager;
    mInputManager = &inputManager;
    mRemotePlayers.clear();
    mNpcs.clear();
    mDamageNumberObjects.clear();
    mNextDamageNumberLane = 0;
    mItemBoxes.clear();
    mMapCollision.Load(terrainHeightMap->GetSharedData());
    mIsLocalPlayerCollidingWithHouse = false; // 충돌처리 체크 초기화
    const bool usesPlayerModel = playerModelMesh != nullptr;
    mPlayerAppearance = {};
    mPlayerAppearance.normalMesh = usesPlayerModel ? std::move(playerModelMesh) : cubeMesh; // 모델이 없으면 큐브로 대체
    mPlayerAppearance.normalTextureGpuHandle = playerTextureGpuHandle;
    mPlayerAppearance.mutationMesh = std::move(mutationPlayerMesh);
    mPlayerAppearance.mutationTextureGpuHandle = mutationPlayerTextureGpuHandle;
    mNpcMesh = npcModelMesh != nullptr ? std::move(npcModelMesh) : mPlayerAppearance.normalMesh; // NPC 모델이 없으면 플레이어 메쉬
    mItemBoxBodyMesh = std::move(itemBoxBodyMesh);
    mItemBoxLidMesh = std::move(itemBoxLidMesh);
    mItemBoxTextureGpuHandle = itemBoxTextureGpuHandle;
    mNpcTextureGpuHandle = npcTextureGpuHandle;
    mItemBoxCollisionBoxes = Geometry::CollisionBoxLoader::Load(TestSceneSettings::ITEM_BOX_COLLISION_PATH);

    DirectionalLight& light = GetDirectionalLight();
    light.SetDirection({0.65F, -0.8F, 0.45F});
    light.SetColorLinear({0.91F, 0.95F, 1.0F});
    light.SetIntensity(3.05F);
    light.SetAmbientStrength(0.16F);

    // 지형은 LunarTerrainRenderer가 따로 그림. 씬 오브젝트로는 만들지 않음.
    SetTerrain(std::move(terrainRenderer));

    // 지형·집·정화기 충돌은 위의 mMapCollision.Load()가 Shared 월드에 등록함(서버와 같은 함수).
    mEnvironment.Build(*this, environmentAssets, houseTextureGpuHandle);

    CreatePurifier(std::move(purifierMesh), purifierTextureGpuHandle);

    LocalPlayer& localPlayer = CreateObject<LocalPlayer>("Local Player", mPlayerAppearance, inputManager, networkManager);
    Material& playerMaterial = localPlayer.GetMaterial();
    // 모델 메쉬를 쓰면 흰색(텍스처 색 그대로), 큐브로 대체됐으면 녹색
    playerMaterial.SetBaseColorLinear(usesPlayerModel ? TestSceneSettings::PLAYER_MODEL_BASE_COLOR_LINEAR
                                                      : TestSceneSettings::PLAYER_BASE_COLOR_LINEAR);
    playerMaterial.SetSurface(0.0F, 0.42F);
    localPlayer.SetNetworkInputEnabled(UsesNetworkInput());
    mLocalPlayer = &localPlayer;
    // 캐릭터 입력/중력/적분/충돌은 서버와 같은 Shared Step에서 처리함.
    auto spawnState = SharedPhysics::MakePlayerMovementState(MapSettings::PLAYER_SPAWN_POSITION_M);
    SharedPhysics::ResolveCharacterContactsInWorld(spawnState, MapSettings::LOCAL_PLAYER_COLLIDER_ID,
                                                   mMapCollision.GetWorld());
    localPlayer.ResetMovementState(spawnState);
    mGameplayCamera = std::make_unique<SpringArmCamera>();
    mGameplayCamera->SetLens(CameraSettings::DEFAULT_FOV_Y_RAD,
                            cameraAspectRatio,
                            CameraSettings::DEFAULT_NEAR_CLIP_M,
                            LunarRenderSettings::FAR_CLIP_M);
    mGameplayCamera->UpdateEye(
        VectorMath::Add(localPlayer.GetTransform().GetPositionM(), TestSceneSettings::PLAYER_CAMERA_TARGET_OFFSET_M));
    localPlayer.SetCamera(mGameplayCamera.get());
    mPhysicsClock.Reset();
}

void GameScene::Update(float deltaTimeSec)
{
    Update(deltaTimeSec, static_cast<double>(deltaTimeSec));
}

void GameScene::Update(float deltaTimeSec, double physicsElapsedTimeSec)
{
    if (mLocalPlayer != nullptr)
    {
        mLocalPlayer->DecayRenderCorrection(physicsElapsedTimeSec);
    }
    if (mInputManager != nullptr && mInputManager->WasKeyPressed(InputKey::ToggleColliderDebug))
    {
        mColliderDebugDraw.ToggleVisible();
    }
    UpdateQuickSlotSelection();

    // 컨트롤러가 이번 프레임 이동 입력을 만들기 전에 서버 변이 속도 배율을 반영함.
    SyncLocalPlayerMoveSpeed();
    // 입력/카메라는 렌더 프레임마다 갱신하고 캐릭터 물리는 누적 시간에 따라 60Hz로 처리.
    Scene::Update(deltaTimeSec);
    UpdateGameplayCamera(deltaTimeSec);
    const int physicsSteps = mPhysicsClock.Advance(physicsElapsedTimeSec);
    if (mLocalPlayer != nullptr && mLocalPlayer->IsActive())
    {
        for (int step = 0; step < physicsSteps; ++step)
        {
            mLocalPlayer->StepMovement(SharedPhysics::FIXED_STEP_DELTA_SEC, MapSettings::LOCAL_PLAYER_COLLIDER_ID,
                                       mMapCollision.GetWorld());
        }
    }
    // 디버그 표시와 집 접촉 로그는 이동 계산 이후의 상태를 기준으로 함.
    mColliderDebugDraw.Sync(*this, mMapCollision.GetWorld(), GetLocalPlayerPositionM());

    const auto houseContacts = CheckLocalPlayerHouseCollision();
    const bool isCollidingNow = !houseContacts.empty();
    if (isCollidingNow && !mIsLocalPlayerCollidingWithHouse)
    {
        const DirectX::XMFLOAT3 playerPositionM = mLocalPlayer->GetTransform().GetPositionM();
        std::cout << "[Collision] TestHouse and Player, player location(" << playerPositionM.x << ", " << playerPositionM.y << ", "
                  << playerPositionM.z << "), " << houseContacts.size() << " box(es)\n";
    }
    mIsLocalPlayerCollidingWithHouse = isCollidingNow;

    TryHandleShoot();
    HandleInteractInput();
    HandleOpenItemBoxResults();
    RefreshHealthBars();
    RefreshMutationAppearances();
    SyncItemBoxes();
    if (const auto* camera = GetGameplayCamera()) mEnvironment.UpdateLod(camera->GetEyeM());
}

const SharedPhysics::CollisionWorld* GameScene::GetCollisionWorld() const noexcept
{
    return &mMapCollision.GetWorld();
}

void GameScene::UpdateGameplayCamera(float deltaTimeSec) noexcept
{
    if (mInputManager == nullptr || mGameplayCamera == nullptr || mLocalPlayer == nullptr || !mLocalPlayer->IsActive())
    {
        return;
    }

    // 마우스로 궤도 회전, 우클릭으로 조준 전환 후 플레이어 머리 위 기준점을 따라감.
    const InputState inputState = mInputManager->GetState();
    mGameplayCamera->RotateYawRad(-inputState.mMouseDeltaPx.x * CameraSettings::MOUSE_ORBIT_SENSITIVITY_RAD_PER_PX);
    mGameplayCamera->RotatePitchRad(-inputState.mMouseDeltaPx.y * CameraSettings::MOUSE_ORBIT_SENSITIVITY_RAD_PER_PX);
    mGameplayCamera->SetAiming(inputState.mAimDown);
    mGameplayCamera->Update(deltaTimeSec);
    mGameplayCamera->UpdateEye(
        VectorMath::Add(mLocalPlayer->GetTransform().GetPositionM(), TestSceneSettings::PLAYER_CAMERA_TARGET_OFFSET_M));
}

void GameScene::RefreshGameplayCamera() noexcept
{
    if (mGameplayCamera != nullptr && mLocalPlayer != nullptr)
    {
        // 렌더 직전에 보간된 표시 위치 기준으로 카메라를 다시 맞춤.
        mGameplayCamera->UpdateEye(VectorMath::Add(GetLocalPlayerRenderPositionM(),
                                                  TestSceneSettings::PLAYER_CAMERA_TARGET_OFFSET_M));
    }
}

DirectX::XMFLOAT3 GameScene::GetLocalPlayerRenderPositionM() const noexcept
{
    if (mLocalPlayer == nullptr)
    {
        return TestSceneSettings::PLAYER_START_POSITION_M;
    }
    return mLocalPlayer->GetRenderPositionM(mPhysicsClock.GetInterpolationAlpha());
}

DirectX::XMFLOAT4X4 GameScene::GetRenderWorldMatrix(const GameObject& object) const noexcept
{
    if (&object != mLocalPlayer)
    {
        return Scene::GetRenderWorldMatrix(object);
    }
    Transform renderTransform = object.GetTransform();
    renderTransform.SetPositionM(GetLocalPlayerRenderPositionM());
    return renderTransform.GetWorldMatrix4x4();
}

const Camera* GameScene::GetGameplayCamera() const noexcept
{
    return mLocalPlayer != nullptr ? mGameplayCamera.get() : nullptr;
}

DirectX::XMFLOAT3 GameScene::GetCameraTargetPositionM() const noexcept
{
    if (mLocalPlayer == nullptr)
    {
        return TestSceneSettings::CAMERA_LOOK_AT_POSITION_M;
    }

    return VectorMath::Add(GetLocalPlayerRenderPositionM(), TestSceneSettings::PLAYER_CAMERA_TARGET_OFFSET_M);
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

    return mLocalPlayer->GetYawRad();
}

PlayerHudState GameScene::BuildPlayerHudState() const
{
    PlayerHudState state;

    // TODO: 서버가 플레이어 HP를 보내주면 그 값으로 교체 (현재 패킷에 없음)
    state.maxHp = PlayerHudSettings::PLACEHOLDER_MAX_HP;
    state.currentHp = PlayerHudSettings::PLACEHOLDER_CURRENT_HP;
    if (mNetworkManager != nullptr)
    {
        if (const auto* myState = mNetworkManager->FindObject(mNetworkManager->GetMyPlayerId()))
        {
            state.mutationRatio = myState->mMutation / TestSceneSettings::MUTATION_MAX_VALUE;
        }
    }
    state.selectedQuickSlotIndex = mSelectedQuickSlotIndex;

    // F키 안내: 정화기 발판 위가 상자보다 우선 (HandleInteractInput과 같은 우선순위)
    // 정화를 요청한 뒤에는 발판을 벗어날 때까지 안내를 숨김.
    // 퀵슬롯 아이템 사용(F 1순위)은 선택 테두리로 충분해서 따로 안내하지 않음.
    if (IsLocalPlayerInPurifierRange())
    {
        if (!mIsPurificationRequested)
        {
            state.interactionPrompt = PlayerHudSettings::INTERACTION_PROMPT_PURIFY_TEXT;
        }
    }
    else if (FindNearbyItemBoxId() >= 0)
    {
        state.interactionPrompt = PlayerHudSettings::INTERACTION_PROMPT_OPEN_ITEM_BOX_TEXT;
    }

    if (mLocalPlayer != nullptr)
    {
        state.moveSpeedMps = mLocalPlayer->GetMoveSpeedMps();
    }

    // 퀵슬롯: 인벤토리에 처음 들어온 순서대로 1번 칸부터 채움. 남는 칸은 빈 칸.
    // 같은 아이템을 또 얻으면 칸은 그대로이고 수량만 늘어남 (매 프레임 다시 채우므로 실시간 반영).
    if (mNetworkManager != nullptr)
    {
        size_t slotIndex = 0;
        for (const Kimgane::Shared::Items::ItemId itemId : mNetworkManager->GetItemAcquireOrder())
        {
            if (slotIndex >= state.quickSlots.size())
            {
                break; // 6번째 이후 아이템은 퀵슬롯에 안 보임 (인벤토리 창이 생기면 거기서 표시)
            }

            // 다 쓴 아이템은 NetworkManager가 획득 순서에서 이미 빼므로 뒤 아이템이 자동으로 앞당겨짐
            QuickSlotState& slot = state.quickSlots[slotIndex++];
            slot.itemId = itemId;
            slot.iconPath = PlayerHudSettings::GetItemIconPath(itemId);
            slot.quantity = mNetworkManager->GetItemQuantity(itemId);
        }
    }

    return state;
}

void GameScene::UpdateQuickSlotSelection() noexcept
{
    // 퀵슬롯 개수는 키 배열·InputKey·HUD 설정 세 곳에 있음. 하나만 바꾸면 컴파일 단계에서 막음.
    static_assert(std::size(InputSettings::QUICK_SLOT_VIRTUAL_KEYS) ==
                      static_cast<std::size_t>(PlayerHudSettings::QUICK_SLOT_COUNT),
                  "QUICK_SLOT_VIRTUAL_KEYS 개수와 QUICK_SLOT_COUNT가 다름");
    static_assert(static_cast<std::size_t>(InputKey::QuickSlot5) - static_cast<std::size_t>(InputKey::QuickSlot1) + 1 ==
                      static_cast<std::size_t>(PlayerHudSettings::QUICK_SLOT_COUNT),
                  "InputKey::QuickSlot1~N 개수와 QUICK_SLOT_COUNT가 다름");

    if (mInputManager == nullptr)
    {
        return;
    }

    for (int slotIndex = 0; slotIndex < PlayerHudSettings::QUICK_SLOT_COUNT; ++slotIndex)
    {
        const auto key = static_cast<InputKey>(static_cast<std::size_t>(InputKey::QuickSlot1) + slotIndex);
        if (mInputManager->WasKeyPressed(key))
        {
            // 같은 칸을 한 번 더 누르면 선택 해제, 다른 칸이면 그 칸으로 선택 이동
            mSelectedQuickSlotIndex =
                mSelectedQuickSlotIndex == slotIndex ? PlayerHudSettings::NO_SELECTED_QUICK_SLOT : slotIndex;
            return;
        }
    }
}
/// ----------------------------------------------------------------------------------
std::vector<SharedPhysics::ContactInfo> GameScene::CheckLocalPlayerHouseCollision()
{
    if (mLocalPlayer == nullptr)
    {
        return {};
    }
    auto contacts = mMapCollision.GetWorld().QueryCharacterContacts(
        SharedPhysics::MakePlayerCollisionBody(MapSettings::LOCAL_PLAYER_COLLIDER_ID,
                                               mLocalPlayer->GetMovementState().positionM));
    std::erase_if(contacts, [this](const SharedPhysics::ContactInfo& contact) {
        return !mMapCollision.IsHouseCollider(contact.objectA);
    });
    return contacts;
}
/// ----------------------------------------------------------------------------------
void GameScene::TryHandleShoot()
{
    if (mInputManager == nullptr || mLocalPlayer == nullptr) // 예외처리
    {
        return;
    }

    const Camera* camera = GetGameplayCamera();
    if (camera == nullptr || !camera->IsAiming()) // 조준 중이 아니면 발사 입력을 무시
    {
        return;
    }

    if (!mInputManager->GetState().mShootPressed) // 좌클릭이 눌린 순간이 아니면 무시
    {
        return;
    }

    const DirectX::XMFLOAT3 rayOriginM = camera->GetEyeM();
    const DirectX::XMFLOAT3 rayDirectionM = camera->GetForward();
    const SharedRaycast::Ray shootRayM{ToSharedVec3(rayOriginM), ToSharedVec3(rayDirectionM), SHOOT_MAX_RANGE_M};

    float bestDistanceM = SHOOT_MAX_RANGE_M;
    const char* bestLabel = nullptr;
    int bestNpcId = -1;

    // 지형·집·정화기·상자: 서버와 같은 Shared 충돌 월드에서 가장 가까운 대상을 찾음. 자기 자신은 제외.
    SharedRaycast::RaycastHit worldHit{};
    if (SharedRaycast::RaycastClosest(shootRayM, mMapCollision.GetWorld().GetBodies(),
                                      MapSettings::LOCAL_PLAYER_COLLIDER_ID, worldHit) &&
        worldHit.distanceM < bestDistanceM)
    {
        bestDistanceM = worldHit.distanceM;
        bestLabel = GetColliderLabel(worldHit.objectId);
    }

    // NPC는 콜라이더가 없어서, 위치 + NPC capsule 크기로 Shared raycast를 즉석에서 돌린다.
    for (const auto& [npcId, npc] : mNpcs)
    {
        if (npc == nullptr)
        {
            continue;
        }

        const SharedPhysics::Capsule npcCapsuleM =
            SharedPhysics::MakeCapsuleFromFootPosition(ToSharedVec3(npc->GetTransform().GetPositionM()),
                                                       SharedPhysics::Settings::NPC_CAPSULE_RADIUS_M,
                                                       SharedPhysics::Settings::NPC_CAPSULE_HEIGHT_M);

        SharedRaycast::RaycastHit npcHit{};
        if (SharedRaycast::RaycastCapsule(shootRayM, npcCapsuleM, npcHit) && npcHit.distanceM < bestDistanceM)
        {
            bestDistanceM = npcHit.distanceM;
            bestLabel = "NPC";
            bestNpcId = npcId;
        }
    }

    if (bestLabel == nullptr) // 아무것도 안 맞았으면 무시
    {
        return;
    }

    const DirectX::XMFLOAT3 hitPointM = VectorMath::Add(rayOriginM, VectorMath::Scale(rayDirectionM, bestDistanceM));

    // 디버그용 로그 출력
    std::cout << "[Shoot] hit " << bestLabel;
    if (bestNpcId >= 0)
    {
        std::cout << "(id=" << bestNpcId << ")";
    }
    std::cout << " at (" << hitPointM.x << ", " << hitPointM.y << ", " << hitPointM.z << "), distance="
              << bestDistanceM << "m\n";

    // 이동 충돌과 같은 Shared 플레이어 캡슐의 중심에서 맞은 지점으로 향하는 방향을 보냄.
    const SharedPhysics::Capsule playerCapsuleM = SharedPhysics::MakeCapsuleFromFootPosition(
        mLocalPlayer->GetMovementState().positionM, SharedPhysics::Settings::PLAYER_CAPSULE_RADIUS_M,
        SharedPhysics::Settings::PLAYER_CAPSULE_HEIGHT_M);
    const DirectX::XMFLOAT3 shootDirectionM = VectorMath::NormalizeOrFallback(
        VectorMath::Subtract(hitPointM, ToXMFloat3(playerCapsuleM.centerM)), rayDirectionM);

    if (mNetworkManager != nullptr)
    {
        mNetworkManager->SendShoot(shootDirectionM);    // 서버에 패킷 전송
    }
}

const char* GameScene::GetColliderLabel(SharedPhysics::ObjectId colliderId) const noexcept
{
    if (colliderId == MapSettings::TERRAIN_COLLIDER_ID)
    {
        return "Terrain";
    }
    if (mMapCollision.IsHouseCollider(colliderId))
    {
        return "TestHouse";
    }
    if (colliderId >= MapSettings::FIRST_PURIFIER_COLLIDER_ID)
    {
        return "Purifier";
    }
    if (mItemBoxes.find(colliderId) != mItemBoxes.end())
    {
        return "ItemBox";
    }
    return "Static";
}

void GameScene::HandleInteractInput()
{
    if (mInputManager == nullptr || mNetworkManager == nullptr || mLocalPlayer == nullptr)
    {
        return;
    }

    // 서버처럼 발판을 벗어나면 정화 요청 상태를 풀어 다시 "[F] 정화"를 띄웁니다.
    const bool isInPurifierRange = IsLocalPlayerInPurifierRange();
    if (!isInPurifierRange)
    {
        mIsPurificationRequested = false;
    }

    if (!mInputManager->WasKeyPressed(InputKey::Interact)) // F가 눌린 순간이 아니면 무시
    {
        return;
    }

    // 1순위: 퀵슬롯에서 소모 아이템을 골라 둔 상태면 발판·상자보다 아이템 사용이 먼저
    // 수량 차감·효과 적용은 서버가 하고, 결과 패킷의 수량으로 퀵슬롯이 갱신됩니다.
    // 서버가 사용을 거절해도(변이 0 등) 요청을 보낸 순간 선택을 풀어서, 연타로 다음 칸 아이템이 쓰이지 않게 합니다.
    switch (GetSelectedUsableItemId())
    {
    case Kimgane::Shared::Items::ItemId::PuriSyringe:
        mNetworkManager->SendUsePuriSyringe();
        mSelectedQuickSlotIndex = PlayerHudSettings::NO_SELECTED_QUICK_SLOT;
        return;
    default:
        break; // 쓸 수 있는 소모 아이템이 선택되지 않았으면 아래 상호작용으로 넘어감
    }

    // 2순위: 정화기 발판 위에서는 상자보다 정화가 우선 (HUD도 "[F] 정화"를 띄움)
    // 발판 판정·정화 진행은 서버가 다시 확인하고, 발판을 벗어나면 서버가 정화를 멈춥니다.
    if (isInPurifierRange)
    {
        // 서버는 공중에서 보낸 요청을 거절하므로, 땅에 서 있을 때만 보내고 요청 상태로 바꿉니다.
        if (!mIsPurificationRequested && mLocalPlayer->GetMovementState().isGrounded)
        {
            mNetworkManager->SendStartPurification();
            mIsPurificationRequested = true;
        }
        return;
    }

    const int bestItemBoxId = FindNearbyItemBoxId();
    if (bestItemBoxId < 0) // 범위 안에 상자가 없음
    {
        return;
    }

    mNetworkManager->SendOpenItemBox(bestItemBoxId);
}

Kimgane::Shared::Items::ItemId GameScene::GetSelectedUsableItemId() const
{
    using Kimgane::Shared::Items::ItemId;

    if (mNetworkManager == nullptr || mSelectedQuickSlotIndex == PlayerHudSettings::NO_SELECTED_QUICK_SLOT)
    {
        return ItemId::None;
    }

    // 퀵슬롯은 획득 순서 그대로 1번 칸부터 채우므로(BuildPlayerHudState), 선택 칸 번호 = 획득 순서 인덱스
    const auto& acquireOrder = mNetworkManager->GetItemAcquireOrder();
    const auto slotIndex = static_cast<std::size_t>(mSelectedQuickSlotIndex);
    if (slotIndex >= acquireOrder.size()) // 빈 칸을 선택함
    {
        return ItemId::None;
    }

    const ItemId itemId = acquireOrder[slotIndex];
    if (mNetworkManager->GetItemQuantity(itemId) == 0)
    {
        return ItemId::None;
    }

    // 서버에 사용 패킷이 있는 소모 아이템만 F로 사용. 나머지(HP 포션 등)는 패킷이 생기면 case 추가.
    switch (itemId)
    {
    case ItemId::PuriSyringe:
        return itemId;
    default:
        return ItemId::None;
    }
}

int GameScene::FindNearbyItemBoxId() const
{
    if (mLocalPlayer == nullptr)
    {
        return -1;
    }

    const DirectX::XMFLOAT3 playerPositionM = mLocalPlayer->GetTransform().GetPositionM();
    float bestDistanceSq = ITEM_BOX_OPEN_DISTANCE_M * ITEM_BOX_OPEN_DISTANCE_M;
    int bestItemBoxId = -1;

    for (const auto& [itemBoxId, itemBox] : mItemBoxes)
    {
        if (itemBox == nullptr || !itemBox->IsActive())
        {
            continue;
        }

        const DirectX::XMFLOAT3 toBoxM = VectorMath::Subtract(itemBox->GetTransform().GetPositionM(), playerPositionM);
        const float distanceSq = VectorMath::Dot(toBoxM, toBoxM);
        if (distanceSq <= bestDistanceSq)
        {
            bestDistanceSq = distanceSq;
            bestItemBoxId = itemBoxId;
        }
    }

    return bestItemBoxId;
}

bool GameScene::IsLocalPlayerInPurifierRange() const noexcept
{
    if (mLocalPlayer == nullptr)
    {
        return false;
    }

    // 기준점(월드) = 정화기 위치 + 발판 위 기준점(로컬). 반경·높이 오차는 더하지 않고 비교에만 씀.
    const SharedPhysics::Vec3 anchorM =
        SharedPhysics::Add(MapSettings::PURIFIER_POSITION_M, MapSettings::PURIFIER_INTERACT_FEET_CENTER_M);
    const DirectX::XMFLOAT3 feetM = mLocalPlayer->GetTransform().GetPositionM();

    const float dx = feetM.x - anchorM.x;
    const float dz = feetM.z - anchorM.z;
    const float radiusM = MapSettings::PURIFIER_INTERACT_RADIUS_M;
    return dx * dx + dz * dz <= radiusM * radiusM &&
           std::abs(feetM.y - anchorM.y) <= MapSettings::PURIFIER_INTERACT_Y_TOLERANCE_M;
}

void GameScene::HandleOpenItemBoxResults()
{
    if (mNetworkManager == nullptr)
    {
        return;
    }

    S2C_OpenItemBoxResult result{};
    while (mNetworkManager->GetOpenItemBoxResult(result))
    {
        if (result.status != OpenItemBoxStatus::Success)
        {
            std::cout << "[ItemBox] open failed id=" << result.objectId
                      << " status=" << ToOpenItemBoxStatusName(result.status) << '\n';
            continue;
        }

        std::cout << "[ItemBox] opened id=" << result.objectId << " -> " << ToItemName(result.itemId) << " x"
                  << result.quantity << " (total " << result.totalQuantity << ")\n";
    }
}

void GameScene::RefreshHealthBars()
{
    if (mNetworkManager == nullptr)
    {
        return;
    }

    for (auto& [npcId, npc] : mNpcs)
    {
        if (npc == nullptr)
        {
            continue;
        }

        if (auto* healthBar = npc->GetComponent<HealthBarComponent>())
        {
            healthBar->SetHp(mNetworkManager->GetCurrentHp(npcId), mNetworkManager->GetMaxHp(npcId));
        }
    }
}
void GameScene::SyncLocalPlayerMoveSpeed()
{
    if (mNetworkManager == nullptr || mLocalPlayer == nullptr)
    {
        return;
    }

    const auto* myState = mNetworkManager->FindObject(mNetworkManager->GetMyPlayerId());
    if (myState == nullptr)
    {
        return;
    }

    // 서버와 동일하게 기본 속도에 변이 배율(1.0~3.0)을 곱해 이동.
    mLocalPlayer->SetMoveSpeedMps(PlayerControllerSettings::DEFAULT_MOVE_SPEED_MPS * myState->mMoveSpeed);
}

void GameScene::RefreshMutationAppearances()
{
    if (mNetworkManager == nullptr)
    {
        return;
    }

    if (mLocalPlayer != nullptr)
    {
        if (const auto* myState = mNetworkManager->FindObject(mNetworkManager->GetMyPlayerId()))
        {
            mLocalPlayer->ApplyMutationAppearance(myState->mMutation);
        }
    }

    for (auto& [playerId, remotePlayer] : mRemotePlayers)
    {
        if (remotePlayer == nullptr)
        {
            continue;
        }

        if (const auto* objectState = mNetworkManager->FindObject(playerId))
        {
            remotePlayer->ApplyMutationAppearance(objectState->mMutation);
        }
    }
}
/// ----------------------------------------------------------------------------------
void GameScene::UpdateNetworkPlayerPosition(int playerId, const DirectX::XMFLOAT3& positionM, float yaw)
{
    if (mNetworkManager != nullptr && playerId == mNetworkManager->GetMyPlayerId())
    {
        if (mLocalPlayer != nullptr)
        {
            mLocalPlayer->CorrectToServer(positionM, yaw, mPhysicsClock.GetInterpolationAlpha());
        }
        return;
    }

    if (const auto npc = mNpcs.find(playerId); npc != mNpcs.end())
    {
        npc->second->SetTargetPositionM(positionM);
        npc->second->SetYawRad(yaw);
        return;
    }

    if (const auto remotePlayer = mRemotePlayers.find(playerId); remotePlayer != mRemotePlayers.end())
    {
        remotePlayer->second->GetTransform().SetPositionM(positionM);
        remotePlayer->second->SetYawRad(yaw);
        return;
    }

    CreateNetworkObject(playerId, positionM, yaw);
}

void GameScene::ShowDamageNumber(int targetId, int damage, const DirectX::XMFLOAT3& targetPositionM)
{
    if (damage <= 0 || !IsNpcObject(targetId))
    {
        return;
    }

    // 서버 위치보다 화면에 보이는 보간 위치를 우선. 제거 전에 호출하므로 처치 피격도 동일.
    DirectX::XMFLOAT3 displayPositionM = targetPositionM;
    const auto target = mNpcs.find(targetId);
    if (target != mNpcs.end() && target->second != nullptr && target->second->IsActive())
    {
        displayPositionM = target->second->GetTransform().GetPositionM();
    }

    GameObject* numberObject = nullptr;
    for (GameObject* candidate : mDamageNumberObjects)
    {
        if (!candidate->IsActive())
        {
            numberObject = candidate;
            break;
        }
    }
    if (numberObject == nullptr && mDamageNumberObjects.size() < RenderSettings::MAX_DAMAGE_NUMBER_COUNT)
    {
        numberObject = &CreateObject("Damage Number");
        numberObject->AddComponent<DamageNumberComponent>(
            TestSceneSettings::NPC_HEALTH_BAR_HEIGHT_OFFSET_M + RenderSettings::DAMAGE_NUMBER_HEIGHT_GAP_M,
            TestSceneSettings::NPC_HEALTH_BAR_VISIBILITY_HEIGHT_OFFSET_M);
        mDamageNumberObjects.push_back(numberObject);
    }
    if (numberObject == nullptr)
    {
        // 동시 표시 한도에 도달하면 가장 오래된 숫자부터 재사용.
        numberObject = *std::max_element(
            mDamageNumberObjects.begin(), mDamageNumberObjects.end(),
            [](const GameObject* lhs, const GameObject* rhs)
            {
                return lhs->GetComponent<DamageNumberComponent>()->GetElapsedTimeSec() <
                       rhs->GetComponent<DamageNumberComponent>()->GetElapsedTimeSec();
            });
    }

    const float laneCenter = static_cast<float>(RenderSettings::DAMAGE_NUMBER_LANE_COUNT - 1) * 0.5F;
    const float horizontalOffsetPx = (static_cast<float>(mNextDamageNumberLane) - laneCenter) *
                                     RenderSettings::DAMAGE_NUMBER_HORIZONTAL_SPACING_PX;
    mNextDamageNumberLane = (mNextDamageNumberLane + 1) % RenderSettings::DAMAGE_NUMBER_LANE_COUNT;
    numberObject->GetTransform().SetPositionM(displayPositionM);
    numberObject->GetComponent<DamageNumberComponent>()->Reset(damage, horizontalOffsetPx);
}

void GameScene::RemoveNetworkPlayer(int playerId)
{
    std::cout << "[SCENE REMOVE] " << playerId << '\n';
    if (const auto npc = mNpcs.find(playerId); npc != mNpcs.end())
    {
        npc->second->SetActive(false);
        mNpcs.erase(npc);
    }
    else if (const auto remotePlayer = mRemotePlayers.find(playerId); remotePlayer != mRemotePlayers.end())
    {
        remotePlayer->second->SetActive(false);
        mRemotePlayers.erase(remotePlayer);
    }
    else
    {
        return;
    }

    std::cout << "[Scene] Network Player " << playerId << " Removed\n";
}

bool GameScene::IsNpcObject(int objectId) const noexcept
{
    // 서버가 보낸 ObjectType으로 NPC 여부를 판단.
    const ObjectState* state = mNetworkManager != nullptr ? mNetworkManager->FindObject(objectId) : nullptr;
    return state != nullptr && state->mType == Kimgane::Shared::World::ObjectType::Npc;
}

void GameScene::CreateNetworkObject(int objectId, const DirectX::XMFLOAT3& positionM, float yawRad)
{
    if (IsNpcObject(objectId))
    {
        Npc& npc = CreateObject<Npc>("NPC " + std::to_string(objectId), positionM,
                                     TestSceneSettings::NPC_VISUAL_MOVE_SPEED_MPS);
        npc.SetYawRad(yawRad);
        npc.SetMesh(mNpcMesh);
        Material& material = npc.GetMaterial();
        material.SetBaseColorLinear(TestSceneSettings::NPC_MODEL_BASE_COLOR_LINEAR);
        material.SetSurface(0.0F, 0.48F);
        material.SetTextureGpuHandle(mNpcTextureGpuHandle);
        // TODO: NPC 머리/몸통 히트박스(Shared Npc::HUMANOID_HIT_BOXES) 정렬 확인이 필요하면
        // ColliderDebugDrawSystem에 NPC 히트박스 표시 기능을 추가. (예전 콜라이더 컴포넌트 방식은 제거됨)
        npc.AddComponent<HealthBarComponent>(TestSceneSettings::NPC_HEALTH_BAR_HEIGHT_OFFSET_M,
                                             TestSceneSettings::NPC_HEALTH_BAR_VISIBILITY_HEIGHT_OFFSET_M);
        mNpcs[objectId] = &npc;
        return;
    }

    RemotePlayer& remotePlayer =
        CreateObject<RemotePlayer>("Network Player " + std::to_string(objectId), mPlayerAppearance);
    remotePlayer.GetTransform().SetPositionM(positionM);
    remotePlayer.SetYawRad(yawRad);
    Material& material = remotePlayer.GetMaterial();
    material.SetBaseColorLinear(TestSceneSettings::NETWORK_PLAYER_BASE_COLOR_LINEAR);
    material.SetSurface(0.0F, 0.48F);
    mRemotePlayers[objectId] = &remotePlayer;
}

void GameScene::CreatePurifier(std::shared_ptr<Mesh> mesh, D3D12_GPU_DESCRIPTOR_HANDLE textureGpuHandle)
{
    const DirectX::XMFLOAT3& positionM = TestSceneSettings::PURIFIER_POSITION_M;

    GameObject& purifier = CreateObject("Purifier NR01");
    purifier.GetTransform().SetPositionM(positionM);
    purifier.SetMesh(std::move(mesh));
    Material& material = purifier.GetMaterial();
    material.SetBaseColorLinear(TestSceneSettings::PURIFIER_BASE_COLOR_LINEAR);
    material.SetSurface(0.2F, 0.5F);
    material.SetTextureGpuHandle(textureGpuHandle);

    // 이동 충돌·레이캐스트·디버그 표시(발판·설비 박스·경사로)는 mMapCollision.Load()가 Shared 값으로 등록함.
    // 서버도 같은 Load()를 쓰므로 로컬/온라인 모두 같은 충돌이 됨.
}

void GameScene::CreateItemBox(int itemBoxId, const DirectX::XMFLOAT3& positionM)
{
    ItemBox& itemBox = CreateObject<ItemBox>("Item Box " + std::to_string(itemBoxId) + " Body");
    itemBox.GetTransform().SetPositionM(positionM);
    itemBox.SetMesh(mItemBoxBodyMesh);
    itemBox.GetMaterial().SetBaseColorLinear(TestSceneSettings::ITEM_BOX_BASE_COLOR_LINEAR);
    itemBox.GetMaterial().SetSurface(0.1F, 0.6F);
    itemBox.GetMaterial().SetTextureGpuHandle(mItemBoxTextureGpuHandle);

    // House/Terrain과 같은 공유 물리 월드에 등록. 이동 충돌·레이캐스트·디버그 표시가 모두 이 body를 씀.
    // 서버 GameWorld::CreateItemBox도 같은 id/박스로 이 월드에 등록하므로 그대로 맞춰줌.
    // S07_Supply_Crate_collision.txt 박스는 본체 기준 로컬 좌표이고, 본체+닫힌 뚜껑을 감싸는 박스 하나뿐임.
    if (!mItemBoxCollisionBoxes.empty())
    {
        const SharedPhysics::Vec3 centerWorldM =
            SharedPhysics::Add(mItemBoxCollisionBoxes.front().box.centerM, ToSharedVec3(positionM));
        mMapCollision.GetWorld().AddOrUpdateBody({itemBoxId,
            SharedPhysics::Box{centerWorldM, mItemBoxCollisionBoxes.front().box.halfExtentsM},
            SharedPhysics::CollisionLayer::STATIC_WORLD, SharedPhysics::CollisionLayer::ALL, false});
    }

    GameObject& lid = CreateObject("Item Box " + std::to_string(itemBoxId) + " Lid");
    lid.SetMesh(mItemBoxLidMesh);
    lid.GetMaterial().SetBaseColorLinear(TestSceneSettings::ITEM_BOX_BASE_COLOR_LINEAR);
    lid.GetMaterial().SetSurface(0.1F, 0.6F);
    lid.GetMaterial().SetTextureGpuHandle(mItemBoxTextureGpuHandle);
    itemBox.AttachLid(lid, TestSceneSettings::ITEM_BOX_LID_LOCAL_OFFSET_M);

    mItemBoxes[itemBoxId] = &itemBox;
}

void GameScene::SyncItemBoxes()
{
    if (mNetworkManager == nullptr)
    {
        return;
    }

    for (const auto& [objectId, state] : mNetworkManager->GetObjects())
    {
        if (state.mType != Kimgane::Shared::World::ObjectType::ItemBox || !state.mIsActive)
        {
            continue;
        }

        if (mItemBoxes.find(objectId) == mItemBoxes.end())
        {
            CreateItemBox(objectId, {state.mX, state.mY, state.mZ});
        }
    }

    for (auto iter = mItemBoxes.begin(); iter != mItemBoxes.end();)
    {
        const auto* state = mNetworkManager->FindObject(iter->first);
        const bool stillPresent =
            state != nullptr && state->mIsActive && state->mType == Kimgane::Shared::World::ObjectType::ItemBox;
        if (stillPresent)
        {
            ++iter;
            continue;
        }

        if (iter->second != nullptr)
        {
            iter->second->Despawn();
        }
        mMapCollision.GetWorld().RemoveBody(iter->first);
        iter = mItemBoxes.erase(iter);
    }
}

bool LocalGameScene::UsesNetworkInput() const noexcept
{
    return false;
}

bool OnlineGameScene::UsesNetworkInput() const noexcept
{
    return true;
}
} // namespace Kimgane::Client
