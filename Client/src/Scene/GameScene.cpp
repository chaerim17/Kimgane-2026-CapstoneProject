#include "Pch.h"

#include "GameScene.h"

#include "TestSceneSettings.h"
#include "../Camera/CameraSettings.h"
#include "../Core/SharedVec3Conversion.h"
#include "../Core/VectorMath.h"
#include "../Gameplay/PlayerControllerSettings.h"
#include "../Input/InputManager.h"
#include "../Network/NetworkManager.h"
#include "../Rendering/RenderSettings.h"
#include "../Terrain/LunarTerrainRenderer.h"
#include "../../Shared/Geometry/CollisionBoxLoader.h"
#include "../../Shared/Items/ItemBoxSettings.h"
#include "../../Shared/Physics/CharacterMovementWorld.h"
#include "../../Shared/Physics/RaycastQueries.h"

#include <DirectXMath.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace Kimgane::Client
{
namespace
{
namespace SharedPhysics = Kimgane::Shared::Physics;
namespace MapSettings = Kimgane::Shared::World::TestMapSettings;
namespace Pulse
{
constexpr float MAX_RANGE_M = 12.0F;
constexpr float TARGET_HEIGHT_M = 1.15F;
constexpr float OCCLUSION_MARGIN_M = 0.05F;
constexpr std::size_t MAX_TARGETS = 24;
constexpr auto VISIBLE_DURATION = std::chrono::milliseconds(1500);
constexpr auto COOLDOWN = std::chrono::seconds(15);
} // namespace Pulse
namespace Raycast = SharedPhysics::RaycastQueries;
using Kimgane::Shared::Items::ItemId;
using PulseClock = std::chrono::steady_clock;
constexpr float PULSE_FADE_SECONDS = .35F;
constexpr int LOCAL_PULSE_TARGET_ID = -2;
constexpr SharedPhysics::ObjectId LOCAL_PULSE_WALL_ID = -4;

bool IsInsideScreen(const DirectX::XMMATRIX& viewProjection, const DirectX::XMFLOAT3& positionM) noexcept
{
    DirectX::XMFLOAT4 clip;
    DirectX::XMStoreFloat4(&clip, DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&positionM), viewProjection));
    return clip.w > 0.0F && std::abs(clip.x) <= clip.w && std::abs(clip.y) <= clip.w &&
           clip.z >= 0.0F && clip.z <= clip.w;
}

bool IsBlockedForPulse(const SharedPhysics::CollisionWorld& world, const SharedPhysics::Vec3& eye,
                       const SharedPhysics::Vec3& target)
{
    const auto delta = SharedPhysics::Subtract(target, eye);
    const float distanceM = std::sqrt(SharedPhysics::Dot(delta, delta));
    if (!(distanceM > Pulse::OCCLUSION_MARGIN_M))
        return false;

    const Raycast::Ray ray{eye, SharedPhysics::Scale(delta, 1.0F / distanceM),
                          distanceM - Pulse::OCCLUSION_MARGIN_M};
    constexpr auto OCCLUDER_LAYERS = SharedPhysics::CollisionLayer::STATIC_WORLD |
                                     SharedPhysics::CollisionLayer::TERRAIN;
    for (const auto& body : world.GetBodies())
    {
        Raycast::RaycastHit hit{};
        if (!body.isTrigger && (body.layerMask & OCCLUDER_LAYERS) != 0 && Raycast::RaycastCollisionBody(ray, body, hit))
            return true;
    }
    return false;
}
} // namespace

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
        throw std::invalid_argument("GameScene에 지형 높이 데이터가 없습니다.");
    }
    mDetectionTargets.clear();
    mDetectionTestTarget = nullptr;
    mDetectionVisibleUntil = {};
    mDetectionNextUse = {};
    Clear();
    mColliderDebugDraw.Initialize(device, terrainHeightMap, ToXMFloat3(MapSettings::TERRAIN_POSITION_M));
    mNetworkManager = &networkManager;
    mInputManager = &inputManager;
    mDamageNumbers.clear();
    mNextDamageNumberLane = 0;
    mMapCollision.Load(terrainHeightMap->GetSharedData());
    mIsLocalPlayerCollidingWithHouse = false;

    const bool usesPlayerModel = playerModelMesh != nullptr;
    PlayerAppearance playerAppearance;
    playerAppearance.normalMesh = usesPlayerModel ? std::move(playerModelMesh) : cubeMesh; // 모델이 없으면 큐브로 대체
    playerAppearance.normalTextureGpuHandle = playerTextureGpuHandle;
    playerAppearance.mutationMesh = std::move(mutationPlayerMesh);
    playerAppearance.mutationTextureGpuHandle = mutationPlayerTextureGpuHandle;

    NetworkObjectAssets networkObjectAssets;
    networkObjectAssets.playerAppearance = playerAppearance;
    // NPC 모델이 없으면 플레이어 메쉬로 대체
    networkObjectAssets.npcMesh = npcModelMesh != nullptr ? std::move(npcModelMesh) : playerAppearance.normalMesh;
    networkObjectAssets.npcTextureGpuHandle = npcTextureGpuHandle;
    networkObjectAssets.itemBoxBodyMesh = std::move(itemBoxBodyMesh);
    networkObjectAssets.itemBoxLidMesh = std::move(itemBoxLidMesh);
    networkObjectAssets.itemBoxTextureGpuHandle = itemBoxTextureGpuHandle;
    networkObjectAssets.itemBoxCollisionBoxes =
        Kimgane::Shared::Geometry::CollisionBoxLoader::Load(Kimgane::Shared::Items::ItemBoxAssetSettings::COLLISION_PATH);
    if (!UsesNetworkInput())
        BuildDetectionPulseTest(cubeMesh, networkObjectAssets);
    mNetworkObjects.Reset(*this, networkManager, mMapCollision.GetWorld(), std::move(networkObjectAssets));
    mInteraction.Reset(inputManager, networkManager, mMapCollision, mNetworkObjects, !UsesNetworkInput());

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

    LocalPlayer& localPlayer = CreateObject<LocalPlayer>("Local Player", playerAppearance, inputManager, networkManager);
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

void GameScene::BuildDetectionPulseTest(const std::shared_ptr<Mesh>& cubeMesh, const NetworkObjectAssets& assets)
{
    // 로컬 테스트 시작 지점에서 정면(+Z)에 벽과 NPC가 각각 3m, 6m 앞에 놓인다.
    constexpr SharedPhysics::Vec3 WALL_SIZE_M{3.0F, 2.6F, .35F};
    constexpr float WALL_OFFSET_M = 3.0F;
    constexpr float NPC_OFFSET_M = 6.0F;
    const auto floorAt = [this](float offsetM)
    {
        auto point = MapSettings::PLAYER_SPAWN_POSITION_M;
        point.z += offsetM;
        SharedPhysics::TerrainSample sample{};
        if (mMapCollision.GetTerrainSampler().SampleHeightAtWorld(point, sample))
            point.y = sample.heightM;
        return point;
    };

    auto wallCenter = floorAt(WALL_OFFSET_M);
    wallCenter.y += WALL_SIZE_M.y * .5F - .1F;
    auto& wall = CreateObject("Detection Pulse Test Wall");
    wall.SetMesh(cubeMesh);
    wall.GetTransform().SetPositionM(ToXMFloat3(wallCenter));
    wall.GetTransform().SetScale(ToXMFloat3(SharedPhysics::Scale(WALL_SIZE_M, 1.0F / TestSceneSettings::CUBE_SIZE_M)));
    wall.GetMaterial().SetBaseColorLinear({.32F, .37F, .43F, 1.0F});
    const SharedPhysics::Box collisionBox{wallCenter, SharedPhysics::Scale(WALL_SIZE_M, .5F)};
    mMapCollision.GetWorld().AddOrUpdateBody({LOCAL_PULSE_WALL_ID, collisionBox,
        SharedPhysics::CollisionLayer::STATIC_WORLD, SharedPhysics::CollisionLayer::ALL, false});

    auto& npc = CreateObject<Npc>("Detection Pulse Test NPC", ToXMFloat3(floorAt(NPC_OFFSET_M)), 0.0F);
    npc.SetMesh(assets.npcMesh);
    npc.GetMaterial().SetTextureGpuHandle(assets.npcTextureGpuHandle);
    npc.GetMaterial().SetBaseColorLinear(TestSceneSettings::NPC_MODEL_BASE_COLOR_LINEAR);
    mDetectionTestTarget = &npc;
}

const GameObject* GameScene::FindDetectionTarget(int objectId) const noexcept
{
    return objectId == LOCAL_PULSE_TARGET_ID ? mDetectionTestTarget : mNetworkObjects.FindDetectionTarget(objectId);
}

void GameScene::ScanDetectionPulse()
{
    mDetectionTargets.clear();
    const auto playerPositionM = ToSharedVec3(GetLocalPlayerPositionM());
    const auto eyeM = ToSharedVec3(mGameplayCamera->GetEyeM());
    const auto viewProjection = mGameplayCamera->GetViewProjectionMatrix();
    const auto consider = [&](int id)
    {
        const GameObject* object = FindDetectionTarget(id);
        if (object == nullptr || !object->IsActive())
            return;
        auto targetM = ToSharedVec3(object->GetTransform().GetPositionM());
        const auto delta = SharedPhysics::Subtract(targetM, playerPositionM);
        const float distanceSquared = SharedPhysics::Dot(delta, delta);
        if (!(distanceSquared <= Pulse::MAX_RANGE_M * Pulse::MAX_RANGE_M))
            return;
        targetM.y += Pulse::TARGET_HEIGHT_M;
        if (IsInsideScreen(viewProjection, ToXMFloat3(targetM)) && IsBlockedForPulse(mMapCollision.GetWorld(), eyeM, targetM))
            mDetectionTargets.push_back({id, object, distanceSquared});
    };

    if (UsesNetworkInput())
    {
        for (const auto& entry : mNetworkManager->GetObjects())
            consider(entry.first);
    }
    else
    {
        consider(LOCAL_PULSE_TARGET_ID);
    }

    std::sort(mDetectionTargets.begin(), mDetectionTargets.end(), [](const auto& a, const auto& b) {
        if (a.distanceSquared == b.distanceSquared)
            return a.objectId < b.objectId;
        return a.distanceSquared < b.distanceSquared;
    });
    if (mDetectionTargets.size() > Pulse::MAX_TARGETS)
        mDetectionTargets.resize(Pulse::MAX_TARGETS);
}

void GameScene::UpdateDetectionPulse(bool canActivate)
{
    const bool requested = mInteraction.ConsumeDetectionPulseRequest();
    const auto now = PulseClock::now();
    const bool disconnected = UsesNetworkInput() && !mNetworkManager->IsConnected();
    if (disconnected)
        mDetectionVisibleUntil = {};
    if (now >= mDetectionVisibleUntil)
        mDetectionTargets.clear();

    if (disconnected || !canActivate || !requested || now < mDetectionNextUse ||
        mGameplayCamera == nullptr || mLocalPlayer == nullptr || !mLocalPlayer->IsActive())
        return;

    // 기존에 동기화된 오브젝트만 대상으로 하는 클라이언트 발동 테스트.
    ScanDetectionPulse();
    mDetectionNextUse = now + Pulse::COOLDOWN;
    mDetectionVisibleUntil = now + Pulse::VISIBLE_DURATION;
}

float GameScene::GetDetectionPulseOpacity() const noexcept
{
    const float remaining = std::chrono::duration<float>(mDetectionVisibleUntil - PulseClock::now()).count();
    return std::clamp(remaining / PULSE_FADE_SECONDS, 0.0F, 1.0F);
}

void GameScene::CollectDetectionPulseTargets(std::vector<const GameObject*>& targets) const
{
    for (const auto& target : mDetectionTargets)
    {
        if (FindDetectionTarget(target.objectId) == target.object && target.object->IsActive())
            targets.push_back(target.object);
    }
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
    mInteraction.UpdateQuickSlotSelection();

    // 컨트롤러가 이번 프레임 이동 입력을 만들기 전에 서버 변이 속도 배율을 반영함.
    SyncLocalPlayerMoveSpeed();
    // 입력/카메라는 렌더 프레임마다 갱신하고 캐릭터 물리는 누적 시간에 따라 60Hz로 처리.
    Scene::Update(deltaTimeSec);
    UpdateGameplayCamera(deltaTimeSec);
    for (DamageNumber& damageNumber : mDamageNumbers)
    {
        damageNumber.Update(deltaTimeSec);
    }
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

    LogLocalPlayerHouseContact();

    mInteraction.HandleActions(mLocalPlayer, GetGameplayCamera());
    RefreshLocalPlayerMutationAppearance();
    mNetworkObjects.Update();
    if (const auto* camera = GetGameplayCamera()) mEnvironment.UpdateLod(camera->GetEyeM());
}

const SharedPhysics::CollisionWorld* GameScene::GetCollisionWorld() const noexcept
{
    return &mMapCollision.GetWorld();
}

void GameScene::CollectWorldHealthBars(std::vector<WorldHealthBarView>& outHealthBars) const
{
    for (const auto& [npcId, npc] : mNetworkObjects.GetNpcs())
    {
        if (npc != nullptr && npc->IsActive())
        {
            outHealthBars.push_back({npc->GetTransform().GetPositionM(), &npc->GetHealthBar()});
        }
    }
}

std::span<const DamageNumber> GameScene::GetDamageNumbers() const noexcept
{
    return mDamageNumbers;
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
            state.mutationRatio = myState->mMutation / PlayerHudSettings::MUTATION_MAX_VALUE;
        }
    }
    state.selectedQuickSlotIndex = mInteraction.GetSelectedQuickSlotIndex();

    // 선택한 탐지 칩, 정화기, 상자 순으로 F키 안내를 표시한다.
    if (mInteraction.IsDetectionPulseSelected())
    {
        state.interactionPrompt = PulseClock::now() < mDetectionNextUse
            ? L"탐지 펄스 재충전 중" : L"[F] 탐지 펄스";
    }
    else if (mInteraction.IsInPurifierRange(mLocalPlayer))
    {
        if (!mInteraction.IsPurificationRequested())
        {
            state.interactionPrompt = PlayerHudSettings::INTERACTION_PROMPT_PURIFY_TEXT;
        }
    }
    else if (mInteraction.FindNearbyItemBoxId(mLocalPlayer) >= 0)
    {
        state.interactionPrompt = PlayerHudSettings::INTERACTION_PROMPT_OPEN_ITEM_BOX_TEXT;
    }

    if (mLocalPlayer != nullptr)
    {
        state.moveSpeedMps = mLocalPlayer->GetMoveSpeedMps();
    }

    // 퀵슬롯: 인벤토리에 처음 들어온 순서대로 1번 칸부터 채움. 남는 칸은 빈 칸.
    // 같은 아이템을 또 얻으면 칸은 그대로이고 수량만 늘어남 (매 프레임 다시 채우므로 실시간 반영).
    if (!UsesNetworkInput())
    {
        state.quickSlots[0] = {ItemId::Chip, PlayerHudSettings::GetItemIconPath(ItemId::Chip), 1};
    }
    else if (mNetworkManager != nullptr)
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

void GameScene::RefreshLocalPlayerMutationAppearance()
{
    if (mNetworkManager == nullptr || mLocalPlayer == nullptr)
    {
        return;
    }

    if (const auto* myState = mNetworkManager->FindObject(mNetworkManager->GetMyPlayerId()))
    {
        mLocalPlayer->ApplyMutationAppearance(myState->mMutation);
    }
}

void GameScene::LogLocalPlayerHouseContact()
{
    const auto houseContacts = CheckLocalPlayerHouseCollision();
    const bool isCollidingNow = !houseContacts.empty();
    if (isCollidingNow && !mIsLocalPlayerCollidingWithHouse)
    {
        const DirectX::XMFLOAT3 playerPositionM = mLocalPlayer->GetTransform().GetPositionM();
        std::cout << "[Collision] 집과 플레이어 접촉, 플레이어 위치(" << playerPositionM.x << ", " << playerPositionM.y << ", "
                  << playerPositionM.z << "), 박스 " << houseContacts.size() << "개\n";
    }
    mIsLocalPlayerCollidingWithHouse = isCollidingNow;
}

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

    mNetworkObjects.ApplyServerTransform(playerId, positionM, yaw);
}

void GameScene::RemoveNetworkPlayer(int playerId)
{
    mNetworkObjects.Remove(playerId);
}

void GameScene::ShowDamageNumber(int targetId, int damage, const DirectX::XMFLOAT3& targetPositionM)
{
    if (damage <= 0 || !mNetworkObjects.IsNpc(targetId))
    {
        return;
    }

    // 서버 위치보다 화면에 보이는 보간 위치를 우선. 제거 전에 호출하므로 처치 피격도 동일.
    DirectX::XMFLOAT3 displayPositionM = targetPositionM;
    if (const Npc* target = mNetworkObjects.FindNpc(targetId); target != nullptr && target->IsActive())
    {
        displayPositionM = target->GetTransform().GetPositionM();
    }

    // 끝난 숫자를 재사용하고, 없으면 한도까지 새로 만들고, 한도에 닿았으면 가장 오래된 숫자를 재사용함.
    DamageNumber* damageNumber = nullptr;
    for (DamageNumber& candidate : mDamageNumbers)
    {
        if (!candidate.IsActive())
        {
            damageNumber = &candidate;
            break;
        }
    }
    if (damageNumber == nullptr && mDamageNumbers.size() < RenderSettings::MAX_DAMAGE_NUMBER_COUNT)
    {
        damageNumber = &mDamageNumbers.emplace_back(
            TestSceneSettings::NPC_HEALTH_BAR_HEIGHT_OFFSET_M + RenderSettings::DAMAGE_NUMBER_HEIGHT_GAP_M,
            TestSceneSettings::NPC_HEALTH_BAR_VISIBILITY_HEIGHT_OFFSET_M);
    }
    if (damageNumber == nullptr)
    {
        damageNumber = &*std::max_element(mDamageNumbers.begin(), mDamageNumbers.end(),
                                          [](const DamageNumber& lhs, const DamageNumber& rhs)
                                          {
                                              return lhs.GetElapsedTimeSec() < rhs.GetElapsedTimeSec();
                                          });
    }

    const float laneCenter = static_cast<float>(RenderSettings::DAMAGE_NUMBER_LANE_COUNT - 1) * 0.5F;
    const float horizontalOffsetPx = (static_cast<float>(mNextDamageNumberLane) - laneCenter) *
                                     RenderSettings::DAMAGE_NUMBER_HORIZONTAL_SPACING_PX;
    mNextDamageNumberLane = (mNextDamageNumberLane + 1) % RenderSettings::DAMAGE_NUMBER_LANE_COUNT;
    damageNumber->Show(damage, displayPositionM, horizontalOffsetPx);
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

bool LocalGameScene::UsesNetworkInput() const noexcept
{
    return false;
}

bool OnlineGameScene::UsesNetworkInput() const noexcept
{
    return true;
}
} // namespace Kimgane::Client
