#include "CharacterMovement.h"
#include "RigidbodyIntegrator.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

namespace Kimgane::Shared::Physics
{
void ResolveCharacterContacts(RigidbodyState& state,
                              CharacterContactQuery& contactQuery,
                              const CharacterContactSettings& settings)
{
    const Vec3 initialPositionM = state.positionM;
    const bool initiallyGrounded = state.isGrounded;
    bool anyPositionChanged = false;
    bool velocityChanged = false;
    bool groundedOnWalkableSurface = false;
    std::vector<ContactInfo> contacts;
    for (int iteration = 0; iteration < settings.maxIterations; ++iteration)
    {
        contacts.clear();
        contactQuery.QueryContacts(state.positionM, contacts);
        bool positionChanged = false;
        for (const ContactInfo& contact : contacts)
        {
            const bool isWalkableGround = CollisionResolver::IsWalkableGround(contact);
            if (!isWalkableGround && !CollisionResolver::ShouldBlockMovement(contact))
            {
                continue;
            }

            // 기존 로컬 정책: 어느 반복에서든 보행 가능한 지면을 만나면 접지로 기록합니다.
            groundedOnWalkableSurface = groundedOnWalkableSurface || isWalkableGround;
            const Vec3 previousPositionM = state.positionM;
            state.positionM = CollisionResolver::ResolvePosition(state.positionM, contact,
                CollisionResolver::ContactParticipant::ObjectB, settings.positionCorrection);
            positionChanged = LengthSquared(Subtract(state.positionM, previousPositionM)) >
                                  settings.minPositionCorrectionSqM || positionChanged;

            const Vec3 slidVelocityMps = CollisionResolver::SlideMovement(state.velocityMps, contact,
                CollisionResolver::ContactParticipant::ObjectB);
            if (LengthSquared(Subtract(slidVelocityMps, state.velocityMps)) > settings.minVelocityChangeSqMps)
            {
                state.velocityMps = slidVelocityMps;
                velocityChanged = true;
            }
        }
        anyPositionChanged = anyPositionChanged || positionChanged;
        if (!positionChanged)
        {
            break;
        }
    }
    // 기존 컴포넌트 반영 기준을 유지해, 상태 변화가 없는 미세 위치 보정은 적용하지 않습니다.
    if (!anyPositionChanged && !velocityChanged && initiallyGrounded == groundedOnWalkableSurface)
    {
        state.positionM = initialPositionM;
    }
    state.isGrounded = groundedOnWalkableSurface;
}

void StepCharacterMovement(RigidbodyState& state,
                           const CharacterMotionInput& input,
                           float deltaTimeSec,
                           CharacterContactQuery& contactQuery,
                           const CharacterContactSettings& settings)
{
    const float moveSpeedMps = std::max(0.0F, input.moveSpeedMps);
    state.velocityMps.x = input.direction.x * moveSpeedMps;
    state.velocityMps.z = input.direction.z * moveSpeedMps;
    if (input.jumpRequested && state.isGrounded)
    {
        RigidbodyIntegrator::AddForce(state, {0.0F, std::max(0.0F, input.jumpVelocityMps), 0.0F},
                                      ForceMode::VelocityChange);
        state.isGrounded = false;
    }
    RigidbodyIntegrator::Integrate(state, deltaTimeSec);
    ResolveCharacterContacts(state, contactQuery, settings);
}

float StepCharacterHorizontalMovement(CharacterMovementState& state,
                                      const CharacterMovementInput& input,
                                      ObjectId objectId,
                                      const CollisionWorld& collisionWorld,
                                      float terrainHeightM,
                                      std::span<const Box> groundBoxes,
                                      float moveSpeedMps,
                                      float deltaTimeSec)
{
    //-----------------------------------------------------------------------------------------------------
    const bool moved = input.moveUp || input.moveDown || input.moveRight || input.moveLeft;

    float moveX = 0.0F;
    float moveZ = 0.0F;

    if (moved)
    {
        moveX = std::sin(input.yawRad);
        moveZ = std::cos(input.yawRad);
    }
    //-----------------------------------------------------------------------------------------------------
    
    // 대각선 이동 보정
    const float length = std::sqrt(moveX * moveX + moveZ * moveZ);
    if (length > 0.0F)
    {
        moveX /= length;
        moveZ /= length;
    }

    Vec3 nextPositionM = state.positionM;
    nextPositionM.x += moveX * moveSpeedMps * deltaTimeSec;
    nextPositionM.z += moveZ * moveSpeedMps * deltaTimeSec;

    // 충돌 체크를 위해 현재 위치에서 이동 후 위치로 캡슐 생성
    const CollisionBody playerBody{
        objectId,
        MakeCapsuleFromFootPosition(nextPositionM, Settings::PLAYER_CAPSULE_RADIUS_M,
                                   Settings::PLAYER_CAPSULE_HEIGHT_M),
        CollisionLayer::PLAYER, CollisionLayer::ALL, false};
    const bool blocked = collisionWorld.HasBlockingContact(playerBody, objectId);

    float groundHeightM = terrainHeightM;
    float bestDistanceM = FLT_MAX;
    constexpr float MAX_GROUND_BOX_DISTANCE_M = 1.0F;
    for (const Box& box : groundBoxes)
    {
        const bool insideXZ = nextPositionM.x >= box.centerM.x - box.halfExtentsM.x &&
                              nextPositionM.x <= box.centerM.x + box.halfExtentsM.x &&
                              nextPositionM.z >= box.centerM.z - box.halfExtentsM.z &&
                              nextPositionM.z <= box.centerM.z + box.halfExtentsM.z;
        const float topYM = box.centerM.y + box.halfExtentsM.y;
        const float distanceM = state.positionM.y - topYM;
        if (insideXZ && state.positionM.y >= topYM && distanceM <= MAX_GROUND_BOX_DISTANCE_M &&
            distanceM < bestDistanceM)
        {
            bestDistanceM = distanceM;
            groundHeightM = topYM;
        }
    }

    if (!blocked)
    {
        state.positionM.x = nextPositionM.x;
        state.positionM.z = nextPositionM.z;
    }
    if (!state.isJumping)
    {
        state.positionM.y = groundHeightM;
    }
    return groundHeightM;
}

//
void StepCharacterVerticalMovement(CharacterMovementState& state,
                                   float groundHeightM,
                                   float gravityMps2,
                                   float deltaTimeSec) noexcept
{
    if (state.isJumping)
    {
        state.positionM.y += state.velocityYMps * deltaTimeSec;
        state.velocityYMps -= gravityMps2 * deltaTimeSec;
        if (state.velocityYMps < 0.0F && state.positionM.y <= groundHeightM)
        {
            state.positionM.y = groundHeightM;
            state.velocityYMps = 0.0F;
            state.isJumping = false;
        }
    }
    if (!state.isJumping)
    {
        state.positionM.y = groundHeightM;
    }
}
} // namespace Kimgane::Shared::Physics
