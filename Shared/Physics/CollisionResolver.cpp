#include "CollisionResolver.h"

#include "CollisionQueries.h"

#include <algorithm>

namespace Kimgane::Shared::Physics::CollisionResolver
{
namespace
{
[[nodiscard]] float Clamp01(float value) noexcept
{
    return std::max(0.0F, std::min(value, 1.0F));
}
} // namespace

Vec3 GetSeparationNormal(const ContactInfo& contact, ContactParticipant participant) noexcept
{
    const Vec3 normal = CollisionQueries::NormalizeOrUp(contact.normal);
    return participant == ContactParticipant::ObjectA ? Scale(normal, -1.0F) : normal;
}

float GetCorrectablePenetrationM(const ContactInfo& contact, PositionCorrectionSettings settings) noexcept
{
    const float ratio = Clamp01(settings.correctionRatio);
    const float allowedPenetrationM = std::max(0.0F, settings.allowedPenetrationM);
    return std::max(0.0F, contact.penetrationM - allowedPenetrationM) * ratio;
}

Vec3 ResolvePosition(const Vec3& positionM,
                     const ContactInfo& contact,
                     ContactParticipant participant,
                     PositionCorrectionSettings settings) noexcept
{
    const float correctionM = GetCorrectablePenetrationM(contact, settings);
    if (correctionM <= 0.0F)
    {
        return positionM;
    }

    return Add(positionM, Scale(GetSeparationNormal(contact, participant), correctionM));
}

Vec3 ResolvePositionFromContacts(const Vec3& positionM,
                                 const ContactInfo* contacts,
                                 std::size_t contactCount,
                                 ContactParticipant participant,
                                 PositionCorrectionSettings settings) noexcept
{
    if (contacts == nullptr || contactCount == 0U)
    {
        return positionM;
    }

    Vec3 resolvedPositionM = positionM;
    for (std::size_t index = 0; index < contactCount; ++index)
    {
        resolvedPositionM = ResolvePosition(resolvedPositionM, contacts[index], participant, settings);
    }

    return resolvedPositionM;
}

Vec3 SlideMovement(const Vec3& movementM, const ContactInfo& contact, ContactParticipant participant) noexcept
{
    const Vec3 separationNormal = GetSeparationNormal(contact, participant);
    const float separatingAmountM = Dot(movementM, separationNormal);
    if (separatingAmountM >= 0.0F)
    {
        return movementM;
    }

    return Subtract(movementM, Scale(separationNormal, separatingAmountM));
}

bool IsWalkableGround(const ContactInfo& contact) noexcept
{
    return contact.isGroundCandidate && contact.isWalkable && contact.surfaceNormal.y > 0.0F;
}

bool ShouldBlockMovement(const ContactInfo& contact) noexcept
{
    return !IsWalkableGround(contact);
}
} // namespace Kimgane::Shared::Physics::CollisionResolver
