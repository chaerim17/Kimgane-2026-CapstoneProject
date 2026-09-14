#include "CharacterMovement.h"
#include "RigidbodyIntegrator.h"

#include <algorithm>

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

} // namespace Kimgane::Shared::Physics
