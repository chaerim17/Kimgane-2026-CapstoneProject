#pragma once

#include "NetworkObjectSync.h"
#include "PlayerInteraction.h"
#include "Scene.h"
#include "../Camera/Camera.h"
#include "../Camera/SpringArmCamera.h"
#include "../Debug/ColliderDebugDrawSystem.h"
#include "../Gameplay/LocalPlayer.h"
#include "../Rendering/LunarEnvironment.h"
#include "../Terrain/TerrainHeightMap.h"
#include "../UI/PlayerHud.h"
#include "../../Shared/Physics/FixedStepClock.h"
#include "../../Shared/World/TestMapCollision.h"

#include <memory>
#include <span>
#include <vector>

namespace Kimgane::Client
{
class InputManager;
class LunarTerrainRenderer;
class NetworkManager;

// 실제 플레이 씬. 맵·내 캐릭터·카메라를 만들고 매 프레임 순서를 조율함.
// - 내 캐릭터(LocalPlayer): 고정 스텝 물리, 서버 보정, 표시 위치 보간
// - 서버 오브젝트(원격 플레이어·NPC·상자): NetworkObjectSync에 맡김
// - 행동 입력(퀵슬롯·사격·F 상호작용): PlayerInteraction에 맡김
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
    // 일반 갱신용 제한 delta와 고정 물리용 실제 경과 시간을 분리합니다.
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
    // 서버 위치 수신. 내 캐릭터면 예측 위치를 보정하고, 아니면 NetworkObjectSync에 넘김.
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
    // 정화기 모델 배치. 충돌은 mMapCollision.Load()가 Shared 값으로 등록함.
    void CreatePurifier(std::shared_ptr<Mesh> mesh, D3D12_GPU_DESCRIPTOR_HANDLE textureGpuHandle);
    // 서버가 보낸 내 변이 수치로 이동속도 배율과 외형을 맞춤.
    void SyncLocalPlayerMoveSpeed();
    void RefreshLocalPlayerMutationAppearance();
    void LogLocalPlayerHouseContact();
    [[nodiscard]] DirectX::XMFLOAT3 GetLocalPlayerRenderPositionM() const noexcept;

    NetworkManager* mNetworkManager = nullptr;
    const InputManager* mInputManager = nullptr;

    LunarEnvironment mEnvironment;
    Kimgane::Shared::World::TestMapCollision mMapCollision;
    NetworkObjectSync mNetworkObjects;
    PlayerInteraction mInteraction;
    bool mIsLocalPlayerCollidingWithHouse = false; // 집 접촉 로그를 처음 닿을 때만 찍기 위한 상태
    LocalPlayer* mLocalPlayer = nullptr;
    Kimgane::Shared::Physics::FixedStepClock mPhysicsClock;
    std::unique_ptr<SpringArmCamera> mGameplayCamera;
    ColliderDebugDrawSystem mColliderDebugDraw;
    // 데미지 숫자 풀. 최대 RenderSettings::MAX_DAMAGE_NUMBER_COUNT개까지 늘어나고 끝난 항목을 재사용함.
    std::vector<DamageNumber> mDamageNumbers;
    unsigned int mNextDamageNumberLane = 0;
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
