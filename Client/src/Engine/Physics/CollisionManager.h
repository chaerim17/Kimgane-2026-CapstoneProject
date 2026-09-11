#pragma once

#include "ColliderComponent.h"

#include <DirectXMath.h>

#include <vector>

namespace Kimgane::Engine
{
struct ContactInfo
{
    DirectX::XMFLOAT3 normal = {0.0F, 1.0F, 0.0F};
    DirectX::XMFLOAT3 surfaceNormal = {0.0F, 1.0F, 0.0F};
    float penetrationM = 0.0F;
    bool isTerrainContact = false;
    bool isGroundCandidate = false;
    bool isWalkable = true;
    float slopeAngleRad = 0.0F;
};

// 클라이언트 콜라이더의 등록 목록을 관리하고, 요청받은 충돌/레이캐스트를 판정합니다.
// 매 프레임 전체 충돌 검사나 이벤트 전달은 수행하지 않습니다.
// 로컬 플레이어의 위치/속도 보정은 호출부와 CollisionResolver가 처리하며,
// HP 감소와 오브젝트 삭제 같은 게임 규칙도 이 클래스의 책임에 포함되지 않습니다.
class CollisionManager final
{
public:
    // 콜라이더를 소유하지 않습니다. 파괴 전 등록 해제 또는 목록 초기화가 필요합니다.
    void AddCollider(ColliderComponent& collider);
    void RemoveCollider(const ColliderComponent& collider);
    void ClearColliders() noexcept;

    // 등록된 콜라이더 중 ignoreCollider를 제외하고 가장 가까운 레이 충돌을 찾습니다.
    [[nodiscard]] bool Raycast(const DirectX::XMFLOAT3& originM,
                               const DirectX::XMFLOAT3& direction,
                               float& outHitDistanceM,
                               const ColliderComponent* ignoreCollider = nullptr) const noexcept;
    [[nodiscard]] bool Raycast(const DirectX::XMFLOAT3& originM,
                               const DirectX::XMFLOAT3& direction,
                               float& outHitDistanceM,
                               ColliderComponent*& outHitCollider,
                               const ColliderComponent* ignoreCollider = nullptr) const noexcept;
    // 등록 여부와 관계없이 전달받은 두 콜라이더를 형상별로 판정하고 접촉 정보를 반환합니다.
    // 판정 성공 시 outContact에 법선, 침투 깊이, 지면/경사 정보를 기록합니다.
    [[nodiscard]] bool CheckCollision(ColliderComponent& a, ColliderComponent& b, ContactInfo& outContact) const noexcept;

private:
    [[nodiscard]] static bool CheckBoxBox(ColliderComponent& a,
                                          ColliderComponent& b,
                                          ContactInfo& outContact) noexcept;
    [[nodiscard]] static bool CheckBoxCapsule(ColliderComponent& box,
                                              ColliderComponent& capsule,
                                              ContactInfo& outContact) noexcept;
    [[nodiscard]] static bool CheckCapsuleCapsule(ColliderComponent& a,
                                                  ColliderComponent& b,
                                                  ContactInfo& outContact) noexcept;
    [[nodiscard]] static bool CheckRampCapsule(ColliderComponent& ramp,
                                               ColliderComponent& capsule,
                                               ContactInfo& outContact) noexcept;
    [[nodiscard]] static bool CheckCapsuleRamp(ColliderComponent& capsule,
                                               ColliderComponent& ramp,
                                               ContactInfo& outContact) noexcept;
    [[nodiscard]] static bool CheckTerrainBox(ColliderComponent& terrain,
                                              ColliderComponent& box,
                                              ContactInfo& outContact) noexcept;
    [[nodiscard]] static bool CheckTerrainCapsule(ColliderComponent& terrain,
                                                  ColliderComponent& capsule,
                                                  ContactInfo& outContact) noexcept;

    std::vector<ColliderComponent*> mColliders;
};
} // namespace Kimgane::Engine
