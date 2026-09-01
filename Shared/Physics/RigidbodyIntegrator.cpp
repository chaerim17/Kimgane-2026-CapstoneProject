#include "RigidbodyIntegrator.h"

#include <algorithm>

namespace Kimgane::Shared::Physics::RigidbodyIntegrator
{
namespace
{
[[nodiscard]] float ClampMassKg(float massKg) noexcept
{
    return std::max(massKg, Settings::MIN_MASS_KG);
}

void ApplyDamping(RigidbodyState& state, float deltaTimeSec) noexcept
{
    const float dragScale = std::clamp(1.0F - state.dragPerSec * deltaTimeSec, 0.0F, 1.0F);
    state.velocityMps = Scale(state.velocityMps, dragScale);

    if (state.isGrounded)
    {
        const float frictionScale = std::clamp(1.0F - state.groundFrictionPerSec * deltaTimeSec, 0.0F, 1.0F);
        state.velocityMps.x *= frictionScale;
        state.velocityMps.z *= frictionScale;

        const float horizontalSpeedSq =
            state.velocityMps.x * state.velocityMps.x + state.velocityMps.z * state.velocityMps.z;
        if (horizontalSpeedSq <= Settings::RESTING_HORIZONTAL_SPEED_SQ)
        {
            state.velocityMps.x = 0.0F;
            state.velocityMps.z = 0.0F;
        }
    }
}
} // namespace

void AddForce(RigidbodyState& state, const Vec3& force, ForceMode mode) noexcept
{
    const float massKg = ClampMassKg(state.massKg);

    switch (mode)
    {
    case ForceMode::Force:
        state.accumulatedForceN = Add(state.accumulatedForceN, force);
        break;
    case ForceMode::Impulse:
        state.velocityMps = Add(state.velocityMps, Scale(force, 1.0F / massKg));
        break;
    case ForceMode::VelocityChange:
        state.velocityMps = Add(state.velocityMps, force);
        break;
    }
}

void ClearAccumulatedForce(RigidbodyState& state) noexcept
{
    state.accumulatedForceN = {};
}

Vec3 BuildAccelerationMps2(const RigidbodyState& state, const Vec3& globalGravityMps2) noexcept
{
    Vec3 accelerationMps2 = Scale(state.accumulatedForceN, 1.0F / ClampMassKg(state.massKg));
    if (state.useGravity && !(state.isGrounded && state.velocityMps.y <= 0.0F))
    {
        accelerationMps2 = Add(accelerationMps2, Scale(globalGravityMps2, state.gravityScale));
    }

    return accelerationMps2;
}

void Integrate(RigidbodyState& state,
               float deltaTimeSec,
               const Vec3& globalGravityMps2,
               bool clearGroundedAfterStep) noexcept
{
    if (state.isKinematic)
    {
        state.velocityMps = {};
        state.lastDisplacementM = {};
        state.accelerationMps2 = {};
        ClearAccumulatedForce(state);
        return;
    }

    if (deltaTimeSec <= 0.0F)
    {
        state.lastDisplacementM = {};
        state.accelerationMps2 = {};
        ClearAccumulatedForce(state);
        return;
    }

    if (state.isGrounded && state.velocityMps.y < 0.0F)
    {
        state.velocityMps.y = 0.0F;
    }

    state.accelerationMps2 = BuildAccelerationMps2(state, globalGravityMps2);
    state.velocityMps = Add(state.velocityMps, Scale(state.accelerationMps2, deltaTimeSec));
    ApplyDamping(state, deltaTimeSec);

    state.previousPositionM = state.positionM;
    state.positionM = Add(state.positionM, Scale(state.velocityMps, deltaTimeSec));
    state.lastDisplacementM = Subtract(state.positionM, state.previousPositionM);
    state.hasPreviousPosition = true;

    ClearAccumulatedForce(state);
    if (clearGroundedAfterStep)
    {
        state.isGrounded = false;
    }
}

void Integrate(RigidbodyState& state, float deltaTimeSec) noexcept
{
    Integrate(state, deltaTimeSec, BuildDefaultGravityMps2(), false);
}
} // namespace Kimgane::Shared::Physics::RigidbodyIntegrator
