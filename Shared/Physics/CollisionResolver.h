#pragma once

#include "CollisionTypes.h"

#include <cstddef>

// ContactInfo를 실제 위치/이동량 보정에 쓰기 위한 순수 계산 함수입니다.
// GameObject나 서버 세션을 모르도록 값만 반환하고, 적용은 호출자가 담당합니다.
namespace Kimgane::Shared::Physics::CollisionResolver
{
enum class ContactParticipant
{
    /// ContactInfo의 objectA 쪽 위치/이동량을 보정할 때 사용합니다.
    ObjectA,

    /// ContactInfo의 objectB 쪽 위치/이동량을 보정할 때 사용합니다.
    ObjectB
};

struct PositionCorrectionSettings
{
    // 1.0이면 겹친 만큼 한 번에 밀고, 낮추면 여러 프레임에 나눠 보정할 수 있습니다.
    float correctionRatio = 1.0F;

    // 아주 작은 겹침은 흔들림을 만들 수 있어서 의도적으로 남겨둘 수 있습니다.
    float allowedPenetrationM = 0.001F;
};

/// 선택한 object가 충돌에서 빠져나가야 하는 방향을 반환합니다.
[[nodiscard]] Vec3 GetSeparationNormal(const ContactInfo& contact,
                                       ContactParticipant participant = ContactParticipant::ObjectA) noexcept;

/// 허용 침투와 보정 비율을 적용해 이번 프레임에 밀어낼 거리를 계산합니다.
[[nodiscard]] float GetCorrectablePenetrationM(const ContactInfo& contact,
                                               PositionCorrectionSettings settings = {}) noexcept;

/// positionM을 직접 바꾸지 않고, contact에서 빠져나간 보정 위치를 반환합니다.
[[nodiscard]] Vec3 ResolvePosition(const Vec3& positionM,
                                   const ContactInfo& contact,
                                   ContactParticipant participant = ContactParticipant::ObjectA,
                                   PositionCorrectionSettings settings = {}) noexcept;

/// 여러 contact를 순서대로 적용한 보정 위치를 반환합니다.
[[nodiscard]] Vec3 ResolvePositionFromContacts(const Vec3& positionM,
                                               const ContactInfo* contacts,
                                               std::size_t contactCount,
                                               ContactParticipant participant = ContactParticipant::ObjectA,
                                               PositionCorrectionSettings settings = {}) noexcept;

/// 벽이나 급경사 쪽으로 파고드는 이동 성분만 제거해 sliding 이동량을 반환합니다.
[[nodiscard]] Vec3 SlideMovement(const Vec3& movementM,
                                 const ContactInfo& contact,
                                 ContactParticipant participant = ContactParticipant::ObjectA) noexcept;

/// walkable 표면 접촉인지 판단합니다. 서버/클라의 접지 기준을 맞추기 위한 helper입니다.
[[nodiscard]] bool IsWalkableGround(const ContactInfo& contact) noexcept;

/// 이동을 차단해야 하는 contact인지 판단합니다. 완만한 바닥 접촉은 차단으로 보지 않습니다.
[[nodiscard]] bool ShouldBlockMovement(const ContactInfo& contact) noexcept;
} // namespace Kimgane::Shared::Physics::CollisionResolver
