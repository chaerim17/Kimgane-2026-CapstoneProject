#pragma once

#include "../UI/PlayerHudSettings.h"
#include "../../Shared/Items/ItemTypes.h"
#include "../../Shared/Physics/CollisionTypes.h"

namespace Kimgane::Shared::World
{
class TestMapCollision;
}

namespace Kimgane::Client
{
class Camera;
class InputManager;
class LocalPlayer;
class NetworkManager;
class NetworkObjectSync;

// 내 캐릭터의 행동 입력. 결과 판정은 서버가 하고, 클라는 요청 패킷과 화면 안내에 필요한 상태만 가짐.
// - 숫자 1~5: 퀵슬롯 선택(같은 칸을 다시 누르면 해제)
// - 조준 중 좌클릭: 충돌 월드·NPC 레이캐스트로 목표점을 찾아 사격 방향을 보냄
// - F: 선택한 소모 아이템 사용 → 정화기 발판 정화 → 가까운 상자 열기 순서로 하나만 실행
class PlayerInteraction final
{
public:
    // 씬을 새로 만들 때 호출. 선택·정화 요청 상태를 초기화함.
    void Reset(const InputManager& inputManager,
               NetworkManager& networkManager,
               const Kimgane::Shared::World::TestMapCollision& mapCollision,
               const NetworkObjectSync& networkObjects) noexcept;

    // 이동 입력보다 먼저 호출: 숫자키로 퀵슬롯 칸을 고름.
    void UpdateQuickSlotSelection() noexcept;
    // 이동·물리 계산 뒤 호출: 사격 → F 상호작용 → 상자 열기 결과 로그.
    void HandleActions(const LocalPlayer* localPlayer, const Camera* camera);

    [[nodiscard]] int GetSelectedQuickSlotIndex() const noexcept;
    // 정화 요청을 보낸 뒤 발판 위에 있는 동안 true. 서버는 정화 상태를 보내지 않아서 클라가 같은 규칙으로 따라감.
    [[nodiscard]] bool IsPurificationRequested() const noexcept;
    // 발이 정화기 발판 위 상호작용 범위(Shared TestMapSettings) 안에 있는지.
    [[nodiscard]] bool IsInPurifierRange(const LocalPlayer* localPlayer) const noexcept;
    // 열 수 있는 거리 안에서 가장 가까운 상자 id. 없으면 -1.
    [[nodiscard]] int FindNearbyItemBoxId(const LocalPlayer* localPlayer) const;

private:
    void TryHandleShoot(const LocalPlayer& localPlayer, const Camera* camera);
    // 충돌 월드 body id를 로그용 이름(Terrain/TestHouse/Purifier/ItemBox)으로 바꿈.
    [[nodiscard]] const char* GetColliderLabel(Kimgane::Shared::Physics::ObjectId colliderId) const noexcept;
    void HandleInteractInput(const LocalPlayer& localPlayer);
    // 선택한 퀵슬롯 칸에 지금 F로 쓸 수 있는 소모 아이템이 있으면 그 id, 아니면 ItemId::None.
    [[nodiscard]] Kimgane::Shared::Items::ItemId GetSelectedUsableItemId() const;
    // NetworkManager에 쌓인 상자 열기 결과를 꺼내서 로그로 출력함.
    void HandleOpenItemBoxResults();

    const InputManager* mInputManager = nullptr;
    NetworkManager* mNetworkManager = nullptr;
    const Kimgane::Shared::World::TestMapCollision* mMapCollision = nullptr;
    const NetworkObjectSync* mNetworkObjects = nullptr;
    int mSelectedQuickSlotIndex = PlayerHudSettings::NO_SELECTED_QUICK_SLOT; // 처음에는 선택 없음
    bool mIsPurificationRequested = false;
};
} // namespace Kimgane::Client
