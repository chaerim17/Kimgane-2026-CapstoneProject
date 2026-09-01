#pragma once

#include "CollisionTypes.h"

namespace Kimgane::Shared::Physics
{
enum class ForceMode
{
    Force,
    Impulse,
    VelocityChange
};

// Client/Server가 같이 쓰는 순수 강체 상태입니다.
// 위치/속도/힘은 SI 단위 기준으로 저장하고, 렌더링/세션 객체 의존성은 두지 않습니다.
struct RigidbodyState
{
    Vec3 positionM = {};
    Vec3 velocityMps = {};
    Vec3 accelerationMps2 = {};
    Vec3 accumulatedForceN = {};
    Vec3 previousPositionM = {};
    Vec3 lastDisplacementM = {};

    float massKg = Settings::DEFAULT_MASS_KG;
    float dragPerSec = Settings::DEFAULT_DRAG_PER_SEC;
    float groundFrictionPerSec = Settings::DEFAULT_GROUND_FRICTION_PER_SEC;
    float restitution = Settings::DEFAULT_RESTITUTION;
    float gravityScale = Settings::DEFAULT_GRAVITY_SCALE;

    bool isKinematic = false;
    bool useGravity = true;
    bool isGrounded = false;
    bool hasPreviousPosition = false;
};

[[nodiscard]] constexpr bool HasSweptMotion(const RigidbodyState& state) noexcept
{
    return state.hasPreviousPosition && LengthSquared(state.lastDisplacementM) > 0.0F;
}
} // namespace Kimgane::Shared::Physics
