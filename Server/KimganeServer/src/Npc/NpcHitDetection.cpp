#include "NpcHitDetection.h"
#include <cmath>

bool NpcHitDetection::RaycastHitBoxes(
    const Kimgane::Shared::Physics::RaycastQueries::Ray& worldRay,
    const Kimgane::Shared::Physics::Vec3& positionM, float yawRad,
    std::span<const Kimgane::Shared::Npc::HitBox> hitBoxes, NpcShotHit& hit)
{
    namespace Physics = Kimgane::Shared::Physics;
    namespace Raycast = Physics::RaycastQueries;
    const float c = std::cos(yawRad);
    const float s = std::sin(yawRad);
    const auto toLocal = [c, s](const Physics::Vec3& v) -> Physics::Vec3
    {
        return {c * v.x - s * v.z, v.y, s * v.x + c * v.z};
    };
    const Raycast::Ray ray{toLocal(Physics::Subtract(worldRay.originM, positionM)),
        toLocal(worldRay.directionM), worldRay.maxDistanceM};
    bool found = false;
    for (const auto& box : hitBoxes)
    {
        Raycast::RaycastHit candidate{};
        if (!Raycast::RaycastBox(ray, box.localBox, candidate) || !std::isfinite(candidate.distanceM))
            continue;
        // 같은 경계에서는 머리를 우선합니다.
        if (!found || candidate.distanceM < hit.distanceM ||
            (candidate.distanceM == hit.distanceM && box.part == NpcHitPart::Head))
        {
            hit = {box.part, candidate.distanceM};
            found = true;
        }
    }
    return found;
}
