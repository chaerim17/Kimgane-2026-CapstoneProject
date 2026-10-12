#pragma once

#include "../../../../Shared/Npc/NpcHitBoxes.h"
#include "../../../../Shared/Physics/RaycastQueries.h"
#include <span>

using NpcHitPart = Kimgane::Shared::Npc::HitPart;

struct NpcShotHit
{
    NpcHitPart part = NpcHitPart::Body;
    float distanceM = 0.0F;
};

namespace NpcHitDetection
{
[[nodiscard]] bool RaycastHitBoxes(
    const Kimgane::Shared::Physics::RaycastQueries::Ray& worldRay,
    const Kimgane::Shared::Physics::Vec3& positionM, float yawRad,
    std::span<const Kimgane::Shared::Npc::HitBox> hitBoxes, NpcShotHit& hit);
}
