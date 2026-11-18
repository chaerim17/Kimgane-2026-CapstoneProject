#pragma once

#include "../Camera/Camera.h"
#include "../Camera/SpringArmCamera.h"
#include "../Core/GameObject.h"
#include "../Debug/ColliderDebugDrawSystem.h"
#include "../Gameplay/ItemBox.h"
#include "../Gameplay/LocalPlayer.h"
#include "../Gameplay/Npc.h"
#include "../Rendering/Light.h"
#include "../Rendering/Mesh.h"
#include "../Rendering/LunarEnvironment.h"
#include "../UI/DamageNumber.h"
#include "../UI/PlayerHud.h"
#include "../UI/TextLabel.h"
#include "../UI/WorldHealthBar.h"
#include "../../Shared/Geometry/CollisionBoxLoader.h"
#include "../Terrain/TerrainHeightMap.h"
#include "../../Shared/Physics/FixedStepClock.h"
#include "../../Shared/Physics/RigidbodyTypes.h"
#include "../../Shared/World/TestMapCollision.h"

#include <DirectXMath.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <d3d12.h>

#include <array>
#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace Kimgane::Client
{
class LunarTerrainRenderer;
// GameObject를 소유하고 생성/정리, 컴포넌트 갱신, 렌더링을 관리하는 기본 씬.
// 충돌 판정은 Shared CollisionWorld가 담당하며, 충돌 월드가 있는 씬(GameScene)만 GetCollisionWorld()를 재정의함.
class Scene
{
public:
    Scene() = default;
    virtual ~Scene() = default;

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) = delete;
    Scene& operator=(Scene&&) = delete;

    // T(기본 GameObject)를 만들어 씬이 소유함. 반환 참조는 Clear() 전까지 유효.
    template <typename T = GameObject, typename... Args>
    T& CreateObject(Args&&... args)
    {
        static_assert(std::is_base_of_v<GameObject, T>, "T는 GameObject를 상속해야 함");
        auto object = std::make_unique<T>(std::forward<Args>(args)...);
        T& reference = *object;
        mObjects.push_back(std::move(object));
        return reference;
    }
    void Clear() noexcept;
    void SetTerrain(std::shared_ptr<LunarTerrainRenderer> terrain) noexcept { mTerrainRenderer = std::move(terrain); }
    [[nodiscard]] const std::shared_ptr<LunarTerrainRenderer>& GetTerrain() const noexcept { return mTerrainRenderer; }

    virtual void Update(float deltaTimeSec);
    virtual void Render(ID3D12GraphicsCommandList& commandList,
                        D3D12_GPU_DESCRIPTOR_HANDLE defaultTextureGpuHandle,
                        MeshPrimitiveTopology primitiveTopology = MeshPrimitiveTopology::TriangleList) const;

    [[nodiscard]] const std::vector<std::unique_ptr<GameObject>>& GetObjects() const noexcept;

    // 패널 위에 그릴 글자를 만들어 씬이 소유함.
    TextLabel& CreateTextLabel(const GameObject& panel, std::wstring text, float fontSizeDip);
    [[nodiscard]] const std::vector<std::unique_ptr<TextLabel>>& GetTextLabels() const noexcept;
    // 렌더러가 그릴 머리 위 체력바 목록. 체력바가 있는 씬(GameScene)만 채움.
    virtual void CollectWorldHealthBars(std::vector<WorldHealthBarView>& outHealthBars) const;
    // 렌더러가 그릴 데미지 숫자 풀. 비활성 항목은 렌더러가 건너뜀.
    [[nodiscard]] virtual std::span<const DamageNumber> GetDamageNumbers() const noexcept;
    // 레이캐스트(체력바 가림 판정 등)에 쓰는 Shared 충돌 월드. 충돌이 없는 UI 씬은 nullptr.
    [[nodiscard]] virtual const Kimgane::Shared::Physics::CollisionWorld* GetCollisionWorld() const noexcept
    {
        return nullptr;
    }
    // 씬에 하나뿐인 태양광. UI 씬은 기본값을 쓰고, GameScene은 Build에서 설정함.
    [[nodiscard]] const DirectionalLight& GetDirectionalLight() const noexcept;
    [[nodiscard]] DirectionalLight& GetDirectionalLight() noexcept;
    // 렌더 전용 행렬입니다. 실제 Transform/충돌 위치를 변경하지 않음.
    [[nodiscard]] virtual DirectX::XMFLOAT4X4 GetRenderWorldMatrix(const GameObject& object) const noexcept;

private:
    std::shared_ptr<LunarTerrainRenderer> mTerrainRenderer;
    DirectionalLight mDirectionalLight;
    std::vector<std::unique_ptr<GameObject>> mObjects;
    std::vector<std::unique_ptr<TextLabel>> mTextLabels;
};

class Camera;
class NetworkManager;
class InputManager;

enum class TitleSceneAction
{
    None,
    StartLocalGame,
    StartOnlineGame,
    OpenSettings
};

enum class TitleMenuOption : std::size_t
{
    LocalGame,
    OnlineGame,
    Settings,
    Count
};

class TitleScene final : public Scene
{
public:
    void Build(std::shared_ptr<Mesh> uiMesh, const InputManager& inputManager, float cameraAspectRatio);
    void Update(float deltaTimeSec) override;

    [[nodiscard]] const Camera* GetUiCamera() const noexcept;
    [[nodiscard]] TitleSceneAction ConsumePendingAction() noexcept;
    [[nodiscard]] TitleMenuOption GetSelectedOption() const noexcept;
    [[nodiscard]] const wchar_t* GetSelectedOptionLabelW() const noexcept;

private:
    void MoveSelection(int direction) noexcept;
    void RefreshVisualState() noexcept;

    const InputManager* mInputManager = nullptr;
    OrthographicCamera mUiCamera;
    std::array<GameObject*, static_cast<std::size_t>(TitleMenuOption::Count)> mOptionPanels = {};
    std::array<TextLabel*, static_cast<std::size_t>(TitleMenuOption::Count)> mOptionLabels = {};
    TitleMenuOption mSelectedOption = TitleMenuOption::LocalGame;
    TitleSceneAction mPendingAction = TitleSceneAction::None;
};

class OverlayScene : public Scene
{
public:
    [[nodiscard]] const Camera* GetUiCamera() const noexcept;

protected:
    void ConfigureOverlayCamera(float cameraAspectRatio) noexcept;

private:
    OrthographicCamera mUiCamera;
};

class SettingsOverlayScene final : public OverlayScene
{
public:
    void Build(std::shared_ptr<Mesh> uiMesh,
               const InputManager& inputManager,
               bool fpsInWindowTitleEnabled,
               float cameraAspectRatio);
    void Update(float deltaTimeSec) override;

    void SetFpsInWindowTitleEnabled(bool enabled) noexcept;
    [[nodiscard]] bool IsFpsInWindowTitleEnabled() const noexcept;
    [[nodiscard]] bool ConsumeCloseRequested() noexcept;

private:
    void RefreshVisualState() noexcept;

    const InputManager* mInputManager = nullptr;
    GameObject* mFpsTogglePanel = nullptr;
    TextLabel* mFpsToggleLabel = nullptr;
    bool mFpsInWindowTitleEnabled = true;
    bool mCloseRequested = false;
};

// 플레이 환경과 캐릭터를 구성하고 입력 모드, 카메라, 네트워크 객체 갱신을 연결합니다.
// Shared 맵과 캐릭터 이동을 호출하고, 계산 결과를 클라이언트 컴포넌트에 표시합니다.
class GameScene : public Scene
{
public:
    void Build(std::shared_ptr<Mesh> cubeMesh,
               ID3D12Device& device,
               std::shared_ptr<Mesh> playerModelMesh,       // 모델 메쉬 매개변수 추가
               D3D12_GPU_DESCRIPTOR_HANDLE playerTextureGpuHandle, // 플레이어 텍스처 SRV 핸들
               std::shared_ptr<Mesh> npcModelMesh,          // NPC 모델 메쉬 매개변수 추가
               D3D12_GPU_DESCRIPTOR_HANDLE npcTextureGpuHandle,    // NPC 텍스처 SRV 핸들
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
               float cameraAspectRatio);
    void Update(float deltaTimeSec) override;
    void RefreshGameplayCamera() noexcept;
    // 일반 컴포넌트용 제한 delta와 고정 물리용 실제 경과 시간을 분리합니다.
    void Update(float deltaTimeSec, double physicsElapsedTimeSec);
    [[nodiscard]] DirectX::XMFLOAT4X4 GetRenderWorldMatrix(const GameObject& object) const noexcept override;
    [[nodiscard]] const Kimgane::Shared::Physics::CollisionWorld* GetCollisionWorld() const noexcept override;
    void CollectWorldHealthBars(std::vector<WorldHealthBarView>& outHealthBars) const override;
    [[nodiscard]] std::span<const DamageNumber> GetDamageNumbers() const noexcept override;
    [[nodiscard]] const Camera* GetGameplayCamera() const noexcept;
    [[nodiscard]] DirectX::XMFLOAT3 GetCameraTargetPositionM() const noexcept;
    [[nodiscard]] DirectX::XMFLOAT3 GetLocalPlayerPositionM() const noexcept;
    [[nodiscard]] float GetLocalPlayerYaw() const noexcept;
    // 하단 HUD에 표시할 체력·이동속도·퀵슬롯 수량을 모음
    [[nodiscard]] PlayerHudState BuildPlayerHudState() const;
    void UpdateNetworkPlayerPosition(int playerId, const DirectX::XMFLOAT3& positionM, float yaw);
    void RemoveNetworkPlayer(int playerId);
    void ShowDamageNumber(int targetId, int damage, const DirectX::XMFLOAT3& targetPositionM);
    // 집과의 접촉을 조회해 로그 판정에 사용합니다. 이동 보정이나 충돌 이벤트 전달은 하지 않습니다.
    [[nodiscard]] std::vector<Kimgane::Shared::Physics::ContactInfo> CheckLocalPlayerHouseCollision();

protected:
    [[nodiscard]] virtual bool UsesNetworkInput() const noexcept = 0;

private:
    // 마우스 입력으로 3인칭 카메라를 회전시키고 로컬 플레이어를 따라가게 함. 오브젝트 갱신 직후 호출.
    void UpdateGameplayCamera(float deltaTimeSec) noexcept;
    [[nodiscard]] bool IsNpcObject(int objectId) const noexcept;
    // 서버 오브젝트 종류에 따라 NPC 또는 원격 플레이어를 만듦.
    void CreateNetworkObject(int objectId, const DirectX::XMFLOAT3& positionM, float yawRad);
    void CreateItemBox(int itemBoxId, const DirectX::XMFLOAT3& positionM);
    // 정화기 모델 배치. 충돌은 mMapCollision.Load()가 Shared 값으로 등록함.
    void CreatePurifier(std::shared_ptr<Mesh> mesh, D3D12_GPU_DESCRIPTOR_HANDLE textureGpuHandle);
    // 서버가 들고 있는 오브젝트 목록과 mItemBoxes를 비교해서 새로 생긴 상자는 만들고, 사라진 상자는 지움.
    void SyncItemBoxes();

    // 조준 중 좌클릭하면 Shared 충돌 월드와 NPC를 대상으로 raycast해서 맞은 대상을 로그로 출력함.
    // 로컬 테스트용
    void TryHandleShoot();
    // 충돌 월드 body id를 로그용 이름(Terrain/TestHouse/Purifier/ItemBox)으로 바꿈.
    [[nodiscard]] const char* GetColliderLabel(Kimgane::Shared::Physics::ObjectId colliderId) const noexcept;

    // F키 상호작용. 우선순위: 선택한 퀵슬롯의 소모 아이템 사용 → 정화기 발판 정화 → 가까운 상자 열기
    void HandleInteractInput();
    // 선택한 퀵슬롯 칸에 지금 F로 쓸 수 있는 소모 아이템이 있으면 그 ID, 아니면 ItemId::None.
    [[nodiscard]] Kimgane::Shared::Items::ItemId GetSelectedUsableItemId() const;
    // 열 수 있는 거리(ITEM_BOX_OPEN_DISTANCE_M) 안에서 가장 가까운 상자 ID. 없으면 -1.
    [[nodiscard]] int FindNearbyItemBoxId() const;
    // 로컬 플레이어 발이 정화기 발판 위 상호작용 범위(Shared TestMapSettings) 안에 있는지.
    [[nodiscard]] bool IsLocalPlayerInPurifierRange() const noexcept;
    // NetworkManager에 쌓인 상자 열기 결과를 매 프레임 꺼내서 로그로 출력함.
    void HandleOpenItemBoxResults();

    // NPC 체력바에 NetworkManager가 받아둔 최신 currentHp/maxHp를 매 프레임 반영함.
    void RefreshHealthBars();
    void SyncLocalPlayerMoveSpeed();
    void RefreshMutationAppearances();
    // 숫자 1~5로 퀵슬롯 칸을 선택하고, 이미 선택된 칸 번호를 다시 누르면 선택 해제. 아이템 사용은 아직 없음.
    void UpdateQuickSlotSelection() noexcept;
    [[nodiscard]] DirectX::XMFLOAT3 GetLocalPlayerRenderPositionM() const noexcept;

    NetworkManager* mNetworkManager = nullptr;
    const InputManager* mInputManager = nullptr;

    PlayerAppearance mPlayerAppearance;
    std::shared_ptr<Mesh> mNpcMesh;
    D3D12_GPU_DESCRIPTOR_HANDLE mNpcTextureGpuHandle = {};
    std::shared_ptr<Mesh> mItemBoxBodyMesh;
    std::shared_ptr<Mesh> mItemBoxLidMesh;
    D3D12_GPU_DESCRIPTOR_HANDLE mItemBoxTextureGpuHandle = {};
    std::vector<Kimgane::Shared::Geometry::NamedCollisionBox> mItemBoxCollisionBoxes;
    LunarEnvironment mEnvironment;
    Kimgane::Shared::World::TestMapCollision mMapCollision;
    bool mIsLocalPlayerCollidingWithHouse = false;      // 충돌처리 체크
    // 정화 요청을 보낸 뒤 발판 위에 있는 동안 true. 서버는 정화 상태를 보내지 않아서 클라가 같은 규칙으로 따라감.
    bool mIsPurificationRequested = false;
    LocalPlayer* mLocalPlayer = nullptr;
    Kimgane::Shared::Physics::FixedStepClock mPhysicsClock;
    std::unique_ptr<SpringArmCamera> mGameplayCamera;
    ColliderDebugDrawSystem mColliderDebugDraw;
    // 서버 오브젝트 id → 클라 객체. 서버 GameWorld처럼 플레이어와 NPC를 나눠 둠.
    std::unordered_map<int, RemotePlayer*> mRemotePlayers;
    std::unordered_map<int, Npc*> mNpcs;
    // 데미지 숫자 풀. 최대 RenderSettings::MAX_DAMAGE_NUMBER_COUNT개까지 늘어나고 끝난 항목을 재사용함.
    std::vector<DamageNumber> mDamageNumbers;
    unsigned int mNextDamageNumberLane = 0;
    std::unordered_map<int, ItemBox*> mItemBoxes;
    int mSelectedQuickSlotIndex = PlayerHudSettings::NO_SELECTED_QUICK_SLOT; // 처음에는 선택 없음
};

class LocalGameScene final : public GameScene
{
private:
    [[nodiscard]] bool UsesNetworkInput() const noexcept override;
};

class OnlineGameScene final : public GameScene
{
private:
    [[nodiscard]] bool UsesNetworkInput() const noexcept override;
};
} // namespace Kimgane::Client
