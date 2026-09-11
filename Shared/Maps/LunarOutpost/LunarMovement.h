#pragma once
#include "LunarCollision.h"
#include "../../Physics/CharacterMovement.h"
#include <algorithm>
#include <cmath>

namespace Kimgane::Shared::LunarMap
{
// HeightAt(x,z) is supplied by the server's existing centered heightmap.
// Only nearby, reachable surfaces count: being below a bridge must not teleport a player onto it.
template<class HeightAt>
float GroundHeight(const Physics::Vec3& point, float footHeight,
                   const std::vector<NamedCollider>& colliders, HeightAt heightAt)
{
    constexpr float STEP_M = 0.30F;
    float ground = heightAt(point.x, point.z);
    for (const auto& collider : colliders)
    {
        float top = ground;
        bool inside = false;
        if (const auto* box = std::get_if<Physics::Box>(&collider.shape))
        {
            inside = std::abs(point.x - box->centerM.x) <= box->halfExtentsM.x &&
                     std::abs(point.z - box->centerM.z) <= box->halfExtentsM.z;
            top = box->centerM.y + box->halfExtentsM.y;
        }
        else if (const auto* ramp = std::get_if<Physics::Ramp>(&collider.shape))
        {
            const auto& c = ramp->centerM;
            const auto& s = ramp->sizeM;
            inside = std::abs(point.x - c.x) <= s.x * 0.5F && std::abs(point.z - c.z) <= s.z * 0.5F;
            float t = 0.0F;
            switch (ramp->direction)
            {
            case Physics::RampDirection::PositiveX: t = (point.x - c.x) / s.x + 0.5F; break;
            case Physics::RampDirection::NegativeX: t = 0.5F - (point.x - c.x) / s.x; break;
            case Physics::RampDirection::PositiveZ: t = (point.z - c.z) / s.z + 0.5F; break;
            case Physics::RampDirection::NegativeZ: t = 0.5F - (point.z - c.z) / s.z; break;
            }
            top = c.y - s.y * 0.5F + std::clamp(t, 0.0F, 1.0F) * s.y;
        }
        if (inside && top <= footHeight + STEP_M) ground = std::max(ground, top);
    }
    return ground;
}

template<class HeightAt>
float StepHorizontal(Physics::CharacterMovementState& state, const Physics::CharacterMovementInput& input,
                     Physics::ObjectId id, const Physics::CollisionWorld& world,
                     const std::vector<NamedCollider>& colliders, HeightAt heightAt,
                     float speedMps, float deltaTimeSec)
{
    constexpr float STEP_M = 0.30F;
    Physics::Vec3 next = state.positionM;
    if (input.moveUp || input.moveDown || input.moveLeft || input.moveRight)
    {
        next.x += std::sin(input.yawRad) * speedMps * deltaTimeSec;
        next.z += std::cos(input.yawRad) * speedMps * deltaTimeSec;
    }
    float ground = GroundHeight(next, state.positionM.y, colliders, heightAt);
    const bool reachable = ground <= state.positionM.y + STEP_M;
    if (!state.isJumping && reachable && state.positionM.y - ground <= STEP_M) next.y = ground;
    const Physics::CollisionBody candidate{id,
        Physics::MakeCapsuleFromFootPosition(next, Physics::Settings::PLAYER_CAPSULE_RADIUS_M,
                                             Physics::Settings::PLAYER_CAPSULE_HEIGHT_M),
        Physics::CollisionLayer::PLAYER, Physics::CollisionLayer::ALL, false};
    if (!reachable || world.HasBlockingContact(candidate, id))
    {
        next = state.positionM;
        ground = GroundHeight(next, state.positionM.y, colliders, heightAt);
    }
    state.positionM = next;
    if (!state.isJumping)
    {
        if (state.positionM.y - ground > STEP_M)
        {
            state.isJumping = true;
            state.velocityYMps = 0.0F;
        }
        else state.positionM.y = ground;
    }
    return ground;
}
} // namespace Kimgane::Shared::LunarMap
