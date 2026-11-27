#include "Pch.h"

#include "PlayerInteraction.h"

#include "NetworkObjectSync.h"
#include "../Camera/Camera.h"
#include "../Core/SharedVec3Conversion.h"
#include "../Core/VectorMath.h"
#include "../Gameplay/LocalPlayer.h"
#include "../Input/InputManager.h"
#include "../Input/InputSettings.h"
#include "../Network/NetworkManager.h"
#include "../../Shared/Items/ItemBoxSettings.h"
#include "../../Shared/Physics/RaycastQueries.h"
#include "../../Shared/World/TestMapCollision.h"
#include "../../Shared/Protocol.h"

#include <cmath>
#include <iostream>
#include <iterator>

namespace Kimgane::Client
{
namespace
{
namespace SharedPhysics = Kimgane::Shared::Physics;
namespace MapSettings = Kimgane::Shared::World::TestMapSettings;
namespace SharedRaycast = Kimgane::Shared::Physics::RaycastQueries;
using Kimgane::Shared::Items::ItemId;

constexpr float SHOOT_MAX_RANGE_M = 100.0F;
using Kimgane::Shared::Items::ITEM_BOX_OPEN_DISTANCE_M;

const char* ToItemName(ItemId itemId) noexcept
{
    switch (itemId)
    {
    case ItemId::HpPotion:
        return "체력 포션";
    case ItemId::Chip:
        return "칩";
    case ItemId::Armor:
        return "방어구";
    default:
        return "없음";
    }
}

const char* ToOpenItemBoxStatusName(OpenItemBoxStatus status) noexcept
{
    switch (status)
    {
    case OpenItemBoxStatus::Success:
        return "성공";
    case OpenItemBoxStatus::NotFound:
        return "없는 상자";
    case OpenItemBoxStatus::NotItemBox:
        return "상자가 아님";
    case OpenItemBoxStatus::TooFar:
        return "거리가 멂";
    case OpenItemBoxStatus::RewardFailed:
        return "보상 지급 실패";
    default:
        return "알 수 없음";
    }
}
} // namespace

void PlayerInteraction::Reset(const InputManager& inputManager,
                              NetworkManager& networkManager,
                              const Kimgane::Shared::World::TestMapCollision& mapCollision,
                              const NetworkObjectSync& networkObjects) noexcept
{
    mInputManager = &inputManager;
    mNetworkManager = &networkManager;
    mMapCollision = &mapCollision;
    mNetworkObjects = &networkObjects;
    mSelectedQuickSlotIndex = PlayerHudSettings::NO_SELECTED_QUICK_SLOT;
    mIsPurificationRequested = false;
}

void PlayerInteraction::UpdateQuickSlotSelection() noexcept
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

void PlayerInteraction::HandleActions(const LocalPlayer* localPlayer, const Camera* camera)
{
    if (localPlayer != nullptr)
    {
        TryHandleShoot(*localPlayer, camera);
        HandleInteractInput(*localPlayer);
    }
    HandleOpenItemBoxResults();
}

int PlayerInteraction::GetSelectedQuickSlotIndex() const noexcept
{
    return mSelectedQuickSlotIndex;
}

bool PlayerInteraction::IsPurificationRequested() const noexcept
{
    return mIsPurificationRequested;
}

bool PlayerInteraction::IsInPurifierRange(const LocalPlayer* localPlayer) const noexcept
{
    if (localPlayer == nullptr)
    {
        return false;
    }

    // 기준점(월드) = 정화기 위치 + 발판 위 기준점(로컬). 반경·높이 오차는 더하지 않고 비교에만 씀.
    const SharedPhysics::Vec3 anchorM =
        SharedPhysics::Add(MapSettings::PURIFIER_POSITION_M, MapSettings::PURIFIER_INTERACT_FEET_CENTER_M);
    const DirectX::XMFLOAT3 feetM = localPlayer->GetTransform().GetPositionM();

    const float dx = feetM.x - anchorM.x;
    const float dz = feetM.z - anchorM.z;
    const float radiusM = MapSettings::PURIFIER_INTERACT_RADIUS_M;
    return dx * dx + dz * dz <= radiusM * radiusM &&
           std::abs(feetM.y - anchorM.y) <= MapSettings::PURIFIER_INTERACT_Y_TOLERANCE_M;
}

int PlayerInteraction::FindNearbyItemBoxId(const LocalPlayer* localPlayer) const
{
    if (localPlayer == nullptr || mNetworkObjects == nullptr)
    {
        return -1;
    }

    const DirectX::XMFLOAT3 playerPositionM = localPlayer->GetTransform().GetPositionM();
    float bestDistanceSq = ITEM_BOX_OPEN_DISTANCE_M * ITEM_BOX_OPEN_DISTANCE_M;
    int bestItemBoxId = -1;

    for (const auto& [itemBoxId, itemBox] : mNetworkObjects->GetItemBoxes())
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

void PlayerInteraction::TryHandleShoot(const LocalPlayer& localPlayer, const Camera* camera)
{
    if (mInputManager == nullptr || mMapCollision == nullptr || mNetworkObjects == nullptr)
    {
        return;
    }

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
    if (SharedRaycast::RaycastClosest(shootRayM, mMapCollision->GetWorld().GetBodies(),
                                      MapSettings::LOCAL_PLAYER_COLLIDER_ID, worldHit) &&
        worldHit.distanceM < bestDistanceM)
    {
        bestDistanceM = worldHit.distanceM;
        bestLabel = GetColliderLabel(worldHit.objectId);
    }

    // NPC는 충돌 월드에 없어서, 위치 + NPC capsule 크기로 Shared raycast를 즉석에서 돌림.
    for (const auto& [npcId, npc] : mNetworkObjects->GetNpcs())
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
    std::cout << "[Shoot] 명중: " << bestLabel;
    if (bestNpcId >= 0)
    {
        std::cout << "(id=" << bestNpcId << ")";
    }
    std::cout << " 위치(" << hitPointM.x << ", " << hitPointM.y << ", " << hitPointM.z << "), 거리="
              << bestDistanceM << "m\n";

    // 이동 충돌과 같은 Shared 플레이어 캡슐의 중심에서 맞은 지점으로 향하는 방향을 보냄.
    const SharedPhysics::Capsule playerCapsuleM = SharedPhysics::MakeCapsuleFromFootPosition(
        localPlayer.GetMovementState().positionM, SharedPhysics::Settings::PLAYER_CAPSULE_RADIUS_M,
        SharedPhysics::Settings::PLAYER_CAPSULE_HEIGHT_M);
    const DirectX::XMFLOAT3 shootDirectionM = VectorMath::NormalizeOrFallback(
        VectorMath::Subtract(hitPointM, ToXMFloat3(playerCapsuleM.centerM)), rayDirectionM);

    if (mNetworkManager != nullptr)
    {
        mNetworkManager->SendShoot(shootDirectionM); // 서버에 패킷 전송
    }
}

const char* PlayerInteraction::GetColliderLabel(SharedPhysics::ObjectId colliderId) const noexcept
{
    if (colliderId == MapSettings::TERRAIN_COLLIDER_ID)
    {
        return "지형";
    }
    if (mMapCollision != nullptr && mMapCollision->IsHouseCollider(colliderId))
    {
        return "집";
    }
    if (colliderId >= MapSettings::FIRST_PURIFIER_COLLIDER_ID)
    {
        return "정화기";
    }
    if (mNetworkObjects != nullptr && mNetworkObjects->GetItemBoxes().contains(colliderId))
    {
        return "아이템 상자";
    }
    return "고정 물체";
}

void PlayerInteraction::HandleInteractInput(const LocalPlayer& localPlayer)
{
    if (mInputManager == nullptr || mNetworkManager == nullptr)
    {
        return;
    }

    // 서버처럼 발판을 벗어나면 정화 요청 상태를 풀어 다시 "[F] 정화"를 띄웁니다.
    const bool isInPurifierRange = IsInPurifierRange(&localPlayer);
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
    case ItemId::PuriSyringe:
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
        if (!mIsPurificationRequested && localPlayer.GetMovementState().isGrounded)
        {
            mNetworkManager->SendStartPurification();
            mIsPurificationRequested = true;
        }
        return;
    }

    // 3순위: 열 수 있는 거리 안의 가장 가까운 상자
    const int bestItemBoxId = FindNearbyItemBoxId(&localPlayer);
    if (bestItemBoxId < 0) // 범위 안에 상자가 없음
    {
        return;
    }

    mNetworkManager->SendOpenItemBox(bestItemBoxId);
}

ItemId PlayerInteraction::GetSelectedUsableItemId() const
{
    if (mNetworkManager == nullptr || mSelectedQuickSlotIndex == PlayerHudSettings::NO_SELECTED_QUICK_SLOT)
    {
        return ItemId::None;
    }

    // 퀵슬롯은 획득 순서 그대로 1번 칸부터 채우므로(GameScene::BuildPlayerHudState), 선택 칸 번호 = 획득 순서 인덱스
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

void PlayerInteraction::HandleOpenItemBoxResults()
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
            std::cout << "[ItemBox] 열기 실패 id=" << result.objectId
                      << " 사유=" << ToOpenItemBoxStatusName(result.status) << '\n';
            continue;
        }

        std::cout << "[ItemBox] 열기 성공 id=" << result.objectId << " -> " << ToItemName(result.itemId) << " x"
                  << result.quantity << " (보유 " << result.totalQuantity << ")\n";
    }
}
} // namespace Kimgane::Client
