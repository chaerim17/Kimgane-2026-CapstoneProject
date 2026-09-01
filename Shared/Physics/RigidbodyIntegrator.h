#pragma once

#include "RigidbodyTypes.h"

namespace Kimgane::Shared::Physics::RigidbodyIntegrator
{
[[nodiscard]] constexpr Vec3 BuildDefaultGravityMps2() noexcept
{
    return {0.0F, Settings::GRAVITY_Y_MPS2, 0.0F};
}

void AddForce(RigidbodyState& state, const Vec3& force, ForceMode mode) noexcept;
void ClearAccumulatedForce(RigidbodyState& state) noexcept;

[[nodiscard]] Vec3 BuildAccelerationMps2(const RigidbodyState& state,
                                         const Vec3& globalGravityMps2) noexcept;

void Integrate(RigidbodyState& state,
               float deltaTimeSec,
               const Vec3& globalGravityMps2,
               bool clearGroundedAfterStep) noexcept;

void Integrate(RigidbodyState& state, float deltaTimeSec) noexcept;
} // namespace Kimgane::Shared::Physics::RigidbodyIntegrator
