#include "CollisionQueries.h"

#include <algorithm>
#include <cmath>

// shared narrow-phase 구현입니다.
// 클라/서버의 보정 정책이 다르므로 위치를 직접 수정하지 않고 ContactInfo만 반환합니다.
namespace Kimgane::Shared::Physics::CollisionQueries
{
namespace
{
// std::clamp 의존을 줄이기 위한 작은 로컬 헬퍼입니다.
[[nodiscard]] float Clamp(float value, float minValue, float maxValue) noexcept
{
    return std::max(minValue, std::min(value, maxValue));
}

// Capsule-Box에서 중심선 점과 box 사이의 최단 거리를 구할 때 사용합니다.
[[nodiscard]] Vec3 ClampPointToAabb(const Vec3& pointM, const Aabb& aabb) noexcept
{
    return {Clamp(pointM.x, aabb.centerM.x - aabb.extentsM.x, aabb.centerM.x + aabb.extentsM.x),
            Clamp(pointM.y, aabb.centerM.y - aabb.extentsM.y, aabb.centerM.y + aabb.extentsM.y),
            Clamp(pointM.z, aabb.centerM.z - aabb.extentsM.z, aabb.centerM.z + aabb.extentsM.z)};
}

[[nodiscard]] Vec3 ClampPointToSegment(const Vec3& pointM, const Vec3& startM, const Vec3& endM) noexcept
{
    const Vec3 segmentM = Subtract(endM, startM);
    const float segmentLengthSqM = LengthSquared(segmentM);
    if (segmentLengthSqM <= Settings::MIN_DIRECTION_LENGTH_SQ)
    {
        return startM;
    }

    const float t = Clamp(Dot(Subtract(pointM, startM), segmentM) / segmentLengthSqM, 0.0F, 1.0F);
    return Add(startM, Scale(segmentM, t));
}

[[nodiscard]] Vec3 NormalizeHorizontalOrX(const Vec3& value) noexcept
{
    const Vec3 horizontal = {value.x, 0.0F, value.z};
    if (LengthSquared(horizontal) <= Settings::MIN_DIRECTION_LENGTH_SQ)
    {
        return {1.0F, 0.0F, 0.0F};
    }

    return NormalizeOrUp(horizontal);
}

[[nodiscard]] Vec3 BuildVerticalNormal(float centerAY, float centerBY) noexcept
{
    return centerBY >= centerAY ? Vec3{0.0F, 1.0F, 0.0F} : Vec3{0.0F, -1.0F, 0.0F};
}

[[nodiscard]] float ComputeIntervalOverlapM(float minA, float maxA, float minB, float maxB) noexcept
{
    return std::min(maxA, maxB) - std::max(minA, minB);
}

[[nodiscard]] float ComputeHorizontalAabbPenetrationM(const Aabb& lhs, const Aabb& rhs) noexcept
{
    const float overlapX = lhs.extentsM.x + rhs.extentsM.x - std::fabs(lhs.centerM.x - rhs.centerM.x);
    const float overlapZ = lhs.extentsM.z + rhs.extentsM.z - std::fabs(lhs.centerM.z - rhs.centerM.z);
    return std::max(0.0F, std::min(overlapX, overlapZ));
}

[[nodiscard]] float GetCapsuleTopM(const Capsule& capsule) noexcept
{
    return GetCapsuleSegmentEndM(capsule).y + std::max(capsule.radiusM, Settings::MIN_COLLIDER_SIZE_M);
}

[[nodiscard]] float GetCylinderTopM(const Cylinder& cylinder) noexcept
{
    const float heightM = std::max(cylinder.heightM, Settings::MIN_COLLIDER_SIZE_M);
    return cylinder.centerM.y + heightM * 0.5F;
}

[[nodiscard]] float GetRampRunLengthM(const Ramp& ramp) noexcept
{
    switch (ramp.direction)
    {
    case RampDirection::PositiveX:
    case RampDirection::NegativeX:
        return std::max(ramp.sizeM.x, Settings::MIN_COLLIDER_SIZE_M);
    case RampDirection::PositiveZ:
    case RampDirection::NegativeZ:
    default:
        return std::max(ramp.sizeM.z, Settings::MIN_COLLIDER_SIZE_M);
    }
}

[[nodiscard]] Vec3 ClampPointToRampFootprint(const Vec3& pointM, const Aabb& rampAabb) noexcept
{
    return {Clamp(pointM.x, rampAabb.centerM.x - rampAabb.extentsM.x, rampAabb.centerM.x + rampAabb.extentsM.x),
            pointM.y,
            Clamp(pointM.z, rampAabb.centerM.z - rampAabb.extentsM.z, rampAabb.centerM.z + rampAabb.extentsM.z)};
}

[[nodiscard]] float ComputeRampSurfaceHeightM(const Ramp& ramp, const Vec3& footprintPointM) noexcept
{
    const Aabb rampAabb = BuildAabb(ramp);
    const float minX = rampAabb.centerM.x - rampAabb.extentsM.x;
    const float maxX = rampAabb.centerM.x + rampAabb.extentsM.x;
    const float minZ = rampAabb.centerM.z - rampAabb.extentsM.z;
    const float maxZ = rampAabb.centerM.z + rampAabb.extentsM.z;
    const float bottomY = rampAabb.centerM.y - rampAabb.extentsM.y;
    const float riseM = rampAabb.extentsM.y * 2.0F;

    float progress = 0.0F;
    switch (ramp.direction)
    {
    case RampDirection::PositiveX:
        progress = (footprintPointM.x - minX) / std::max(maxX - minX, Settings::MIN_COLLIDER_SIZE_M);
        break;
    case RampDirection::NegativeX:
        progress = (maxX - footprintPointM.x) / std::max(maxX - minX, Settings::MIN_COLLIDER_SIZE_M);
        break;
    case RampDirection::PositiveZ:
        progress = (footprintPointM.z - minZ) / std::max(maxZ - minZ, Settings::MIN_COLLIDER_SIZE_M);
        break;
    case RampDirection::NegativeZ:
        progress = (maxZ - footprintPointM.z) / std::max(maxZ - minZ, Settings::MIN_COLLIDER_SIZE_M);
        break;
    }

    return bottomY + riseM * Clamp(progress, 0.0F, 1.0F);
}

// walkable 판단을 위해 지형 normal과 world up 사이 각도를 구합니다.
[[nodiscard]] float BuildSlopeAngleRad(const Vec3& normal) noexcept
{
    return std::acos(Clamp(normal.y, -1.0F, 1.0F));
}

// normal이 0에 가까운 겹침에서 penetration fallback으로 사용합니다.
[[nodiscard]] float ComputeAabbPenetrationM(const Aabb& lhs, const Aabb& rhs) noexcept
{
    const float overlapX = lhs.extentsM.x + rhs.extentsM.x - std::fabs(lhs.centerM.x - rhs.centerM.x);
    const float overlapY = lhs.extentsM.y + rhs.extentsM.y - std::fabs(lhs.centerM.y - rhs.centerM.y);
    const float overlapZ = lhs.extentsM.z + rhs.extentsM.z - std::fabs(lhs.centerM.z - rhs.centerM.z);
    return std::max(0.0F, std::min({overlapX, overlapY, overlapZ}));
}

// Capsule-Capsule은 중심 선분 간 최단 거리로 검사합니다.
[[nodiscard]] float ComputeSegmentSegmentDistanceSquared(const Vec3& startA,
                                                         const Vec3& endA,
                                                         const Vec3& startB,
                                                         const Vec3& endB,
                                                         Vec3& outClosestA,
                                                         Vec3& outClosestB) noexcept
{
    // s/t는 각 선분 위의 0~1 위치입니다. clamp해서 선분 밖 closest point를 막습니다.
    const Vec3 d1 = Subtract(endA, startA);
    const Vec3 d2 = Subtract(endB, startB);
    const Vec3 r = Subtract(startA, startB);

    // 너무 짧은 segment는 점처럼 접힌 예외 상황으로 처리합니다.
    const float a = Dot(d1, d1);
    const float e = Dot(d2, d2);

    const float f = Dot(d2, r);

    float s = 0.0F;
    float t = 0.0F;

    if (a <= Settings::MIN_DIRECTION_LENGTH_SQ && e <= Settings::MIN_DIRECTION_LENGTH_SQ)
    {
        outClosestA = startA;
        outClosestB = startB;
        return LengthSquared(Subtract(outClosestA, outClosestB));
    }

    if (a <= Settings::MIN_DIRECTION_LENGTH_SQ)
    {
        t = Clamp(f / e, 0.0F, 1.0F);
    }
    else
    {
        const float c = Dot(d1, r);
        if (e <= Settings::MIN_DIRECTION_LENGTH_SQ)
        {
            s = Clamp(-c / a, 0.0F, 1.0F);
        }
        else
        {
            const float b = Dot(d1, d2);
            const float denominator = a * e - b * b;
            if (std::fabs(denominator) > Settings::MIN_DIRECTION_LENGTH_SQ)
            {
                s = Clamp((b * f - c * e) / denominator, 0.0F, 1.0F);
            }

            t = (b * s + f) / e;
            if (t < 0.0F)
            {
                t = 0.0F;
                s = Clamp(-c / a, 0.0F, 1.0F);
            }
            else if (t > 1.0F)
            {
                t = 1.0F;
                s = Clamp((b - c) / a, 0.0F, 1.0F);
            }
        }
    }

    outClosestA = Add(startA, Scale(d1, s));
    outClosestB = Add(startB, Scale(d2, t));
    return LengthSquared(Subtract(outClosestA, outClosestB));
}

// 위치 보정은 호출자가 결정하므로 여기서는 normal/penetration만 채웁니다.
void FillContactFromNormal(const Vec3& normal,
                           float penetrationM,
                           bool isTerrainContact,
                           ContactInfo& outContact) noexcept
{
    outContact.normal = NormalizeOrUp(normal);
    outContact.surfaceNormal = outContact.normal;
    outContact.penetrationM = std::max(0.0F, penetrationM);
    outContact.isTerrainContact = isTerrainContact;
    outContact.slopeAngleRad = BuildSlopeAngleRad(outContact.surfaceNormal);
    outContact.isWalkable = outContact.slopeAngleRad <= Settings::DEFAULT_WALKABLE_SLOPE_RAD;
}

void FillSurfaceContact(const Vec3& surfaceNormal,
                        float penetrationM,
                        ContactInfo& outContact) noexcept
{
    outContact.normal = NormalizeOrUp(surfaceNormal);
    outContact.surfaceNormal = outContact.normal;
    outContact.penetrationM = std::max(0.0F, penetrationM);
    outContact.isTerrainContact = true;
    outContact.slopeAngleRad = BuildSlopeAngleRad(outContact.surfaceNormal);
    outContact.isWalkable = outContact.slopeAngleRad <= Settings::DEFAULT_WALKABLE_SLOPE_RAD;
}

bool FillVerticalRoundContact(const Vec3& centerA,
                              float radiusA,
                              float minYA,
                              float maxYA,
                              const Vec3& centerB,
                              float radiusB,
                              float minYB,
                              float maxYB,
                              ContactInfo& outContact) noexcept
{
    const float radiusSumM = std::max(radiusA, Settings::MIN_COLLIDER_SIZE_M) +
                             std::max(radiusB, Settings::MIN_COLLIDER_SIZE_M);
    const Vec3 horizontalDeltaM = {centerB.x - centerA.x, 0.0F, centerB.z - centerA.z};
    const float horizontalDistanceSqM = LengthSquared(horizontalDeltaM);
    if (horizontalDistanceSqM > radiusSumM * radiusSumM)
    {
        return false;
    }

    const float verticalOverlapM = ComputeIntervalOverlapM(minYA, maxYA, minYB, maxYB);
    if (verticalOverlapM < 0.0F)
    {
        return false;
    }

    const float horizontalDistanceM = std::sqrt(std::max(horizontalDistanceSqM, 0.0F));
    const float horizontalPenetrationM = radiusSumM - horizontalDistanceM;
    if (verticalOverlapM < horizontalPenetrationM)
    {
        FillContactFromNormal(BuildVerticalNormal(centerA.y, centerB.y), verticalOverlapM, false, outContact);
        return true;
    }

    FillContactFromNormal(NormalizeHorizontalOrX(horizontalDeltaM), horizontalPenetrationM, false, outContact);
    return true;
}

// 반대 순서 API에서도 normal이 objectA -> objectB가 되도록 맞춥니다.
void InvertContactNormal(ContactInfo& contact) noexcept
{
    contact.normal = Scale(contact.normal, -1.0F);
    if (!contact.isTerrainContact)
    {
        contact.surfaceNormal = Scale(contact.surfaceNormal, -1.0F);
    }
}
} // namespace

Vec3 NormalizeOrUp(const Vec3& value) noexcept
{
    // normal이 0이면 디버깅 가능한 고정 방향을 사용합니다.
    const float lengthSq = LengthSquared(value);
    if (lengthSq <= Settings::MIN_DIRECTION_LENGTH_SQ)
    {
        return {0.0F, 1.0F, 0.0F};
    }

    return Scale(value, 1.0F / std::sqrt(lengthSq));
}

Vec3 GetCapsuleSegmentStartM(const Capsule& capsule) noexcept
{
    // height에는 반구가 포함되므로 중심 segment는 height - 2 * radius입니다.
    const float radiusM = std::max(capsule.radiusM, Settings::MIN_COLLIDER_SIZE_M);
    const float heightM = std::max(capsule.heightM, radiusM * 2.0F);
    const float halfSegmentM = std::max((heightM * 0.5F) - radiusM, 0.0F);
    return {capsule.centerM.x, capsule.centerM.y - halfSegmentM, capsule.centerM.z};
}

Vec3 GetCapsuleSegmentEndM(const Capsule& capsule) noexcept
{
    const float radiusM = std::max(capsule.radiusM, Settings::MIN_COLLIDER_SIZE_M);
    const float heightM = std::max(capsule.heightM, radiusM * 2.0F);
    const float halfSegmentM = std::max((heightM * 0.5F) - radiusM, 0.0F);
    return {capsule.centerM.x, capsule.centerM.y + halfSegmentM, capsule.centerM.z};
}

float GetCapsuleBottomM(const Capsule& capsule) noexcept
{
    // 지형 접지용 bottom Y입니다. 발 위치로 만든 capsule이면 foot Y와 같습니다.
    return GetCapsuleSegmentStartM(capsule).y - std::max(capsule.radiusM, Settings::MIN_COLLIDER_SIZE_M);
}

float GetSphereBottomM(const Sphere& sphere) noexcept
{
    return sphere.centerM.y - std::max(sphere.radiusM, Settings::MIN_COLLIDER_SIZE_M);
}

float GetCylinderBottomM(const Cylinder& cylinder) noexcept
{
    const float heightM = std::max(cylinder.heightM, Settings::MIN_COLLIDER_SIZE_M);
    return cylinder.centerM.y - heightM * 0.5F;
}

Aabb BuildAabb(const Box& box) noexcept
{
    // 빈 collider가 broad-phase를 망치지 않도록 최소 크기를 보정합니다.
    return {box.centerM,
            {std::max(box.halfExtentsM.x, Settings::MIN_COLLIDER_SIZE_M),
             std::max(box.halfExtentsM.y, Settings::MIN_COLLIDER_SIZE_M),
             std::max(box.halfExtentsM.z, Settings::MIN_COLLIDER_SIZE_M)}};
}

Aabb BuildAabb(const Sphere& sphere) noexcept
{
    const float radiusM = std::max(sphere.radiusM, Settings::MIN_COLLIDER_SIZE_M);
    return {sphere.centerM, {radiusM, radiusM, radiusM}};
}

Aabb BuildAabb(const Capsule& capsule) noexcept
{
    // narrow-phase 전에 빠르게 거르기 위한 보수적 AABB입니다.
    const float radiusM = std::max(capsule.radiusM, Settings::MIN_COLLIDER_SIZE_M);
    const Vec3 startM = GetCapsuleSegmentStartM(capsule);
    const Vec3 endM = GetCapsuleSegmentEndM(capsule);
    const Vec3 minPointM = {std::min(startM.x, endM.x) - radiusM,
                            std::min(startM.y, endM.y) - radiusM,
                            std::min(startM.z, endM.z) - radiusM};
    const Vec3 maxPointM = {std::max(startM.x, endM.x) + radiusM,
                            std::max(startM.y, endM.y) + radiusM,
                            std::max(startM.z, endM.z) + radiusM};
    return {{(minPointM.x + maxPointM.x) * 0.5F,
             (minPointM.y + maxPointM.y) * 0.5F,
             (minPointM.z + maxPointM.z) * 0.5F},
            {(maxPointM.x - minPointM.x) * 0.5F,
             (maxPointM.y - minPointM.y) * 0.5F,
             (maxPointM.z - minPointM.z) * 0.5F}};
}

Aabb BuildAabb(const Cylinder& cylinder) noexcept
{
    const float radiusM = std::max(cylinder.radiusM, Settings::MIN_COLLIDER_SIZE_M);
    const float halfHeightM = std::max(cylinder.heightM, Settings::MIN_COLLIDER_SIZE_M) * 0.5F;
    return {cylinder.centerM, {radiusM, halfHeightM, radiusM}};
}

Aabb BuildAabb(const Ramp& ramp) noexcept
{
    // Ramp는 계단을 감싸는 axis-aligned wedge로 사용합니다.
    return {ramp.centerM,
            {std::max(ramp.sizeM.x * 0.5F, Settings::MIN_COLLIDER_SIZE_M),
             std::max(ramp.sizeM.y * 0.5F, Settings::MIN_COLLIDER_SIZE_M),
             std::max(ramp.sizeM.z * 0.5F, Settings::MIN_COLLIDER_SIZE_M)}};
}

Vec3 GetRampSurfaceNormal(const Ramp& ramp) noexcept
{
    const float riseM = std::max(ramp.sizeM.y, Settings::MIN_COLLIDER_SIZE_M);
    const float runM = GetRampRunLengthM(ramp);

    switch (ramp.direction)
    {
    case RampDirection::PositiveX:
        return NormalizeOrUp({-riseM, runM, 0.0F});
    case RampDirection::NegativeX:
        return NormalizeOrUp({riseM, runM, 0.0F});
    case RampDirection::NegativeZ:
        return NormalizeOrUp({0.0F, runM, riseM});
    case RampDirection::PositiveZ:
    default:
        return NormalizeOrUp({0.0F, runM, -riseM});
    }
}

bool Intersects(const Aabb& lhs, const Aabb& rhs) noexcept
{
    // 세 축이 모두 겹칠 때만 broad-phase를 통과합니다.
    return std::fabs(lhs.centerM.x - rhs.centerM.x) <= lhs.extentsM.x + rhs.extentsM.x &&
           std::fabs(lhs.centerM.y - rhs.centerM.y) <= lhs.extentsM.y + rhs.extentsM.y &&
           std::fabs(lhs.centerM.z - rhs.centerM.z) <= lhs.extentsM.z + rhs.extentsM.z;
}

bool CheckCapsuleCapsule(const Capsule& lhs, const Capsule& rhs, ContactInfo& outContact) noexcept
{
    // 캐릭터끼리 겹치는지 검사합니다. true면 밀어낼 방향/깊이가 ContactInfo에 담깁니다.
    Vec3 closestA = {};
    Vec3 closestB = {};
    const float radiusSumM = std::max(lhs.radiusM, Settings::MIN_COLLIDER_SIZE_M) +
                             std::max(rhs.radiusM, Settings::MIN_COLLIDER_SIZE_M);
    const float distanceSqM = ComputeSegmentSegmentDistanceSquared(GetCapsuleSegmentStartM(lhs),
                                                                   GetCapsuleSegmentEndM(lhs),
                                                                   GetCapsuleSegmentStartM(rhs),
                                                                   GetCapsuleSegmentEndM(rhs),
                                                                   closestA,
                                                                   closestB);
    if (distanceSqM > radiusSumM * radiusSumM)
    {
        return false;
    }

    const float distanceM = std::sqrt(std::max(distanceSqM, 0.0F));

    // closestA -> closestB가 lhs에서 rhs 방향 normal입니다.
    FillContactFromNormal(Subtract(closestB, closestA), radiusSumM - distanceM, false, outContact);
    return true;
}

bool CheckSphereSphere(const Sphere& lhs, const Sphere& rhs, ContactInfo& outContact) noexcept
{
    const float radiusSumM = std::max(lhs.radiusM, Settings::MIN_COLLIDER_SIZE_M) +
                             std::max(rhs.radiusM, Settings::MIN_COLLIDER_SIZE_M);
    const Vec3 deltaM = Subtract(rhs.centerM, lhs.centerM);
    const float distanceSqM = LengthSquared(deltaM);
    if (distanceSqM > radiusSumM * radiusSumM)
    {
        return false;
    }

    const float distanceM = std::sqrt(std::max(distanceSqM, 0.0F));
    FillContactFromNormal(deltaM, radiusSumM - distanceM, false, outContact);
    return true;
}

bool CheckCapsuleSphere(const Capsule& capsule, const Sphere& sphere, ContactInfo& outContact) noexcept
{
    const float radiusSumM = std::max(capsule.radiusM, Settings::MIN_COLLIDER_SIZE_M) +
                             std::max(sphere.radiusM, Settings::MIN_COLLIDER_SIZE_M);
    const Vec3 closestCapsulePointM =
        ClampPointToSegment(sphere.centerM, GetCapsuleSegmentStartM(capsule), GetCapsuleSegmentEndM(capsule));
    const Vec3 deltaM = Subtract(sphere.centerM, closestCapsulePointM);
    const float distanceSqM = LengthSquared(deltaM);
    if (distanceSqM > radiusSumM * radiusSumM)
    {
        return false;
    }

    const float distanceM = std::sqrt(std::max(distanceSqM, 0.0F));
    FillContactFromNormal(distanceM > Settings::MIN_COLLIDER_SIZE_M ? deltaM
                                                                    : Subtract(sphere.centerM, capsule.centerM),
                          radiusSumM - distanceM,
                          false,
                          outContact);
    return true;
}

bool CheckSphereCapsule(const Sphere& sphere, const Capsule& capsule, ContactInfo& outContact) noexcept
{
    if (!CheckCapsuleSphere(capsule, sphere, outContact))
    {
        return false;
    }

    InvertContactNormal(outContact);
    return true;
}

bool CheckCapsuleBox(const Capsule& capsule, const Box& box, ContactInfo& outContact) noexcept
{
    // AABB broad-phase로 멀리 있는 장애물을 먼저 거릅니다.
    const Aabb boxAabb = BuildAabb(box);
    const Aabb capsuleAabb = BuildAabb(capsule);
    if (!Intersects(capsuleAabb, boxAabb))
    {
        return false;
    }

    const float radiusM = std::max(capsule.radiusM, Settings::MIN_COLLIDER_SIZE_M);
    const Vec3 segmentStartM = GetCapsuleSegmentStartM(capsule);
    const Vec3 segmentEndM = GetCapsuleSegmentEndM(capsule);

    // 세로 capsule 가정: 중심선 점과 box의 closest point 거리를 비교합니다.
    const Vec3 centerLinePointM = {capsule.centerM.x,
                                   Clamp(box.centerM.y, segmentStartM.y, segmentEndM.y),
                                   capsule.centerM.z};
    const Vec3 closestBoxPointM = ClampPointToAabb(centerLinePointM, boxAabb);
    const Vec3 deltaM = Subtract(closestBoxPointM, centerLinePointM);
    const float distanceSqM = LengthSquared(deltaM);
    if (distanceSqM > radiusM * radiusM)
    {
        return false;
    }

    const float distanceM = std::sqrt(std::max(distanceSqM, 0.0F));
    const float penetrationM = distanceM > Settings::MIN_COLLIDER_SIZE_M
                                   ? radiusM - distanceM
                                   : ComputeAabbPenetrationM(capsuleAabb, boxAabb);

    // 중심선이 box 내부면 normal이 0이므로 center 방향과 AABB overlap으로 보정합니다.
    FillContactFromNormal(distanceM > Settings::MIN_COLLIDER_SIZE_M ? deltaM : Subtract(box.centerM, capsule.centerM),
                          penetrationM,
                          false,
                          outContact);
    return true;
}

bool CheckBoxCapsule(const Box& box, const Capsule& capsule, ContactInfo& outContact) noexcept
{
    if (!CheckCapsuleBox(capsule, box, outContact))
    {
        return false;
    }

    InvertContactNormal(outContact);
    return true;
}

bool CheckSphereBox(const Sphere& sphere, const Box& box, ContactInfo& outContact) noexcept
{
    const Aabb sphereAabb = BuildAabb(sphere);
    const Aabb boxAabb = BuildAabb(box);
    if (!Intersects(sphereAabb, boxAabb))
    {
        return false;
    }

    const float radiusM = std::max(sphere.radiusM, Settings::MIN_COLLIDER_SIZE_M);
    const Vec3 closestBoxPointM = ClampPointToAabb(sphere.centerM, boxAabb);
    const Vec3 deltaM = Subtract(closestBoxPointM, sphere.centerM);
    const float distanceSqM = LengthSquared(deltaM);
    if (distanceSqM > radiusM * radiusM)
    {
        return false;
    }

    const float distanceM = std::sqrt(std::max(distanceSqM, 0.0F));
    const float penetrationM = distanceM > Settings::MIN_COLLIDER_SIZE_M
                                   ? radiusM - distanceM
                                   : ComputeAabbPenetrationM(sphereAabb, boxAabb);
    FillContactFromNormal(distanceM > Settings::MIN_COLLIDER_SIZE_M ? deltaM : Subtract(box.centerM, sphere.centerM),
                          penetrationM,
                          false,
                          outContact);
    return true;
}

bool CheckBoxSphere(const Box& box, const Sphere& sphere, ContactInfo& outContact) noexcept
{
    if (!CheckSphereBox(sphere, box, outContact))
    {
        return false;
    }

    InvertContactNormal(outContact);
    return true;
}

bool CheckCylinderCylinder(const Cylinder& lhs, const Cylinder& rhs, ContactInfo& outContact) noexcept
{
    return FillVerticalRoundContact(lhs.centerM,
                                    lhs.radiusM,
                                    GetCylinderBottomM(lhs),
                                    GetCylinderTopM(lhs),
                                    rhs.centerM,
                                    rhs.radiusM,
                                    GetCylinderBottomM(rhs),
                                    GetCylinderTopM(rhs),
                                    outContact);
}

bool CheckCylinderSphere(const Cylinder& cylinder, const Sphere& sphere, ContactInfo& outContact) noexcept
{
    return FillVerticalRoundContact(cylinder.centerM,
                                    cylinder.radiusM,
                                    GetCylinderBottomM(cylinder),
                                    GetCylinderTopM(cylinder),
                                    sphere.centerM,
                                    sphere.radiusM,
                                    GetSphereBottomM(sphere),
                                    sphere.centerM.y + std::max(sphere.radiusM, Settings::MIN_COLLIDER_SIZE_M),
                                    outContact);
}

bool CheckSphereCylinder(const Sphere& sphere, const Cylinder& cylinder, ContactInfo& outContact) noexcept
{
    if (!CheckCylinderSphere(cylinder, sphere, outContact))
    {
        return false;
    }

    InvertContactNormal(outContact);
    return true;
}

bool CheckCylinderCapsule(const Cylinder& cylinder, const Capsule& capsule, ContactInfo& outContact) noexcept
{
    return FillVerticalRoundContact(cylinder.centerM,
                                    cylinder.radiusM,
                                    GetCylinderBottomM(cylinder),
                                    GetCylinderTopM(cylinder),
                                    capsule.centerM,
                                    capsule.radiusM,
                                    GetCapsuleBottomM(capsule),
                                    GetCapsuleTopM(capsule),
                                    outContact);
}

bool CheckCapsuleCylinder(const Capsule& capsule, const Cylinder& cylinder, ContactInfo& outContact) noexcept
{
    if (!CheckCylinderCapsule(cylinder, capsule, outContact))
    {
        return false;
    }

    InvertContactNormal(outContact);
    return true;
}

bool CheckCylinderBox(const Cylinder& cylinder, const Box& box, ContactInfo& outContact) noexcept
{
    const Aabb cylinderAabb = BuildAabb(cylinder);
    const Aabb boxAabb = BuildAabb(box);
    if (!Intersects(cylinderAabb, boxAabb))
    {
        return false;
    }

    const float radiusM = std::max(cylinder.radiusM, Settings::MIN_COLLIDER_SIZE_M);
    const Vec3 closestBoxPointM = {Clamp(cylinder.centerM.x,
                                         boxAabb.centerM.x - boxAabb.extentsM.x,
                                         boxAabb.centerM.x + boxAabb.extentsM.x),
                                   cylinder.centerM.y,
                                   Clamp(cylinder.centerM.z,
                                         boxAabb.centerM.z - boxAabb.extentsM.z,
                                         boxAabb.centerM.z + boxAabb.extentsM.z)};
    const Vec3 horizontalDeltaM = {closestBoxPointM.x - cylinder.centerM.x,
                                  0.0F,
                                  closestBoxPointM.z - cylinder.centerM.z};
    const float horizontalDistanceSqM = LengthSquared(horizontalDeltaM);
    if (horizontalDistanceSqM > radiusM * radiusM)
    {
        return false;
    }

    const float verticalOverlapM = ComputeIntervalOverlapM(GetCylinderBottomM(cylinder),
                                                          GetCylinderTopM(cylinder),
                                                          boxAabb.centerM.y - boxAabb.extentsM.y,
                                                          boxAabb.centerM.y + boxAabb.extentsM.y);
    if (verticalOverlapM < 0.0F)
    {
        return false;
    }

    const float horizontalDistanceM = std::sqrt(std::max(horizontalDistanceSqM, 0.0F));
    const float horizontalPenetrationM = horizontalDistanceM > Settings::MIN_COLLIDER_SIZE_M
                                             ? radiusM - horizontalDistanceM
                                             : ComputeHorizontalAabbPenetrationM(cylinderAabb, boxAabb);
    if (verticalOverlapM < horizontalPenetrationM)
    {
        FillContactFromNormal(BuildVerticalNormal(cylinder.centerM.y, box.centerM.y),
                              verticalOverlapM,
                              false,
                              outContact);
        return true;
    }

    FillContactFromNormal(horizontalDistanceM > Settings::MIN_COLLIDER_SIZE_M
                              ? horizontalDeltaM
                              : NormalizeHorizontalOrX(Subtract(box.centerM, cylinder.centerM)),
                          horizontalPenetrationM,
                          false,
                          outContact);
    return true;
}

bool CheckBoxCylinder(const Box& box, const Cylinder& cylinder, ContactInfo& outContact) noexcept
{
    if (!CheckCylinderBox(cylinder, box, outContact))
    {
        return false;
    }

    InvertContactNormal(outContact);
    return true;
}

bool CheckRampCapsule(const Ramp& ramp, const Capsule& capsule, ContactInfo& outContact) noexcept
{
    const Aabb rampAabb = BuildAabb(ramp);
    const float radiusM = std::max(capsule.radiusM, Settings::MIN_COLLIDER_SIZE_M);
    const Vec3 footprintPointM = ClampPointToRampFootprint(capsule.centerM, rampAabb);
    const float horizontalDeltaX = capsule.centerM.x - footprintPointM.x;
    const float horizontalDeltaZ = capsule.centerM.z - footprintPointM.z;
    if ((horizontalDeltaX * horizontalDeltaX + horizontalDeltaZ * horizontalDeltaZ) > radiusM * radiusM)
    {
        return false;
    }

    const float rampSurfaceHeightM = ComputeRampSurfaceHeightM(ramp, footprintPointM);
    const float capsuleBottomM = GetCapsuleBottomM(capsule);
    if (capsuleBottomM > rampSurfaceHeightM)
    {
        return false;
    }

    outContact.normal = GetRampSurfaceNormal(ramp);
    outContact.surfaceNormal = outContact.normal;
    outContact.penetrationM = rampSurfaceHeightM - capsuleBottomM;
    // CollisionResolver는 terrain contact를 walkable ground로 해석합니다.
    // Ramp도 계단용 walkable surface라서 같은 플래그를 공유합니다.
    outContact.isTerrainContact = true;
    outContact.slopeAngleRad = BuildSlopeAngleRad(outContact.surfaceNormal);
    outContact.isWalkable = outContact.slopeAngleRad <= Settings::DEFAULT_WALKABLE_SLOPE_RAD;
    return true;
}

bool CheckCapsuleRamp(const Capsule& capsule, const Ramp& ramp, ContactInfo& outContact) noexcept
{
    if (!CheckRampCapsule(ramp, capsule, outContact))
    {
        return false;
    }

    InvertContactNormal(outContact);
    return true;
}

bool CheckRampSphere(const Ramp& ramp, const Sphere& sphere, ContactInfo& outContact) noexcept
{
    const Aabb rampAabb = BuildAabb(ramp);
    const float radiusM = std::max(sphere.radiusM, Settings::MIN_COLLIDER_SIZE_M);
    const Vec3 footprintPointM = ClampPointToRampFootprint(sphere.centerM, rampAabb);
    const float horizontalDeltaX = sphere.centerM.x - footprintPointM.x;
    const float horizontalDeltaZ = sphere.centerM.z - footprintPointM.z;
    if ((horizontalDeltaX * horizontalDeltaX + horizontalDeltaZ * horizontalDeltaZ) > radiusM * radiusM)
    {
        return false;
    }

    const float rampSurfaceHeightM = ComputeRampSurfaceHeightM(ramp, footprintPointM);
    const float sphereBottomM = GetSphereBottomM(sphere);
    if (sphereBottomM > rampSurfaceHeightM)
    {
        return false;
    }

    FillSurfaceContact(GetRampSurfaceNormal(ramp), rampSurfaceHeightM - sphereBottomM, outContact);
    return true;
}

bool CheckSphereRamp(const Sphere& sphere, const Ramp& ramp, ContactInfo& outContact) noexcept
{
    if (!CheckRampSphere(ramp, sphere, outContact))
    {
        return false;
    }

    InvertContactNormal(outContact);
    return true;
}

bool CheckRampCylinder(const Ramp& ramp, const Cylinder& cylinder, ContactInfo& outContact) noexcept
{
    const Aabb rampAabb = BuildAabb(ramp);
    const float radiusM = std::max(cylinder.radiusM, Settings::MIN_COLLIDER_SIZE_M);
    const Vec3 footprintPointM = ClampPointToRampFootprint(cylinder.centerM, rampAabb);
    const float horizontalDeltaX = cylinder.centerM.x - footprintPointM.x;
    const float horizontalDeltaZ = cylinder.centerM.z - footprintPointM.z;
    if ((horizontalDeltaX * horizontalDeltaX + horizontalDeltaZ * horizontalDeltaZ) > radiusM * radiusM)
    {
        return false;
    }

    const float rampSurfaceHeightM = ComputeRampSurfaceHeightM(ramp, footprintPointM);
    const float cylinderBottomM = GetCylinderBottomM(cylinder);
    if (cylinderBottomM > rampSurfaceHeightM)
    {
        return false;
    }

    FillSurfaceContact(GetRampSurfaceNormal(ramp), rampSurfaceHeightM - cylinderBottomM, outContact);
    return true;
}

bool CheckCylinderRamp(const Cylinder& cylinder, const Ramp& ramp, ContactInfo& outContact) noexcept
{
    if (!CheckRampCylinder(ramp, cylinder, outContact))
    {
        return false;
    }

    InvertContactNormal(outContact);
    return true;
}

bool CheckTerrainCapsule(const TerrainSurface& terrain, const Capsule& capsule, ContactInfo& outContact) noexcept
{
    // TerrainSurface는 지형 구현을 숨기는 adapter입니다. 서버는 TerrainSampler만 구현하면 됩니다.
    if (terrain.sampler == nullptr)
    {
        return false;
    }

    TerrainSample sample = {};
    // 현재는 캐릭터 중심 XZ 한 점만 샘플링합니다. 넓은 캐릭터는 주변 샘플을 추가하면 됩니다.
    if (!terrain.sampler->SampleHeightAtWorld(capsule.centerM, sample))
    {
        return false;
    }

    const float capsuleBottomM = GetCapsuleBottomM(capsule);
    // 바닥이 지형보다 위에 있으면 공중 상태로 보고 충돌하지 않습니다.
    if (capsuleBottomM > sample.heightM)
    {
        return false;
    }

    // 지형 접촉은 무조건 차단이 아니므로 walkable 여부만 표시합니다.
    outContact.normal = NormalizeOrUp(sample.normal);
    outContact.surfaceNormal = outContact.normal;
    outContact.penetrationM = sample.heightM - capsuleBottomM;
    outContact.isTerrainContact = true;
    outContact.slopeAngleRad = BuildSlopeAngleRad(outContact.surfaceNormal);
    outContact.isWalkable = outContact.slopeAngleRad <= Settings::DEFAULT_WALKABLE_SLOPE_RAD;
    return true;
}

bool CheckCapsuleTerrain(const Capsule& capsule, const TerrainSurface& terrain, ContactInfo& outContact) noexcept
{
    if (!CheckTerrainCapsule(terrain, capsule, outContact))
    {
        return false;
    }

    InvertContactNormal(outContact);
    return true;
}

bool CheckTerrainSphere(const TerrainSurface& terrain, const Sphere& sphere, ContactInfo& outContact) noexcept
{
    if (terrain.sampler == nullptr)
    {
        return false;
    }

    TerrainSample sample = {};
    if (!terrain.sampler->SampleHeightAtWorld(sphere.centerM, sample))
    {
        return false;
    }

    const float sphereBottomM = GetSphereBottomM(sphere);
    if (sphereBottomM > sample.heightM)
    {
        return false;
    }

    FillSurfaceContact(sample.normal, sample.heightM - sphereBottomM, outContact);
    return true;
}

bool CheckSphereTerrain(const Sphere& sphere, const TerrainSurface& terrain, ContactInfo& outContact) noexcept
{
    if (!CheckTerrainSphere(terrain, sphere, outContact))
    {
        return false;
    }

    InvertContactNormal(outContact);
    return true;
}

bool CheckTerrainCylinder(const TerrainSurface& terrain, const Cylinder& cylinder, ContactInfo& outContact) noexcept
{
    if (terrain.sampler == nullptr)
    {
        return false;
    }

    TerrainSample sample = {};
    if (!terrain.sampler->SampleHeightAtWorld(cylinder.centerM, sample))
    {
        return false;
    }

    const float cylinderBottomM = GetCylinderBottomM(cylinder);
    if (cylinderBottomM > sample.heightM)
    {
        return false;
    }

    FillSurfaceContact(sample.normal, sample.heightM - cylinderBottomM, outContact);
    return true;
}

bool CheckCylinderTerrain(const Cylinder& cylinder, const TerrainSurface& terrain, ContactInfo& outContact) noexcept
{
    if (!CheckTerrainCylinder(terrain, cylinder, outContact))
    {
        return false;
    }

    InvertContactNormal(outContact);
    return true;
}

bool CheckCollision(const CollisionBody& lhs, const CollisionBody& rhs, ContactInfo& outContact) noexcept
{
    // 지원 조합을 명시적으로 보여주기 위해 variant dispatch를 직접 작성합니다.
    bool hasCollision = false;

    if (const auto* lhsCapsule = std::get_if<Capsule>(&lhs.shape))
    {
        if (const auto* rhsCapsule = std::get_if<Capsule>(&rhs.shape))
        {
            hasCollision = CheckCapsuleCapsule(*lhsCapsule, *rhsCapsule, outContact);
        }
        else if (const auto* rhsSphere = std::get_if<Sphere>(&rhs.shape))
        {
            hasCollision = CheckCapsuleSphere(*lhsCapsule, *rhsSphere, outContact);
        }
        else if (const auto* rhsCylinder = std::get_if<Cylinder>(&rhs.shape))
        {
            hasCollision = CheckCapsuleCylinder(*lhsCapsule, *rhsCylinder, outContact);
        }
        else if (const auto* rhsBox = std::get_if<Box>(&rhs.shape))
        {
            hasCollision = CheckCapsuleBox(*lhsCapsule, *rhsBox, outContact);
        }
        else if (const auto* rhsRamp = std::get_if<Ramp>(&rhs.shape))
        {
            hasCollision = CheckCapsuleRamp(*lhsCapsule, *rhsRamp, outContact);
        }
        else if (const auto* rhsTerrain = std::get_if<TerrainSurface>(&rhs.shape))
        {
            hasCollision = CheckCapsuleTerrain(*lhsCapsule, *rhsTerrain, outContact);
        }
    }
    else if (const auto* lhsSphere = std::get_if<Sphere>(&lhs.shape))
    {
        if (const auto* rhsSphere = std::get_if<Sphere>(&rhs.shape))
        {
            hasCollision = CheckSphereSphere(*lhsSphere, *rhsSphere, outContact);
        }
        else if (const auto* rhsCapsule = std::get_if<Capsule>(&rhs.shape))
        {
            hasCollision = CheckSphereCapsule(*lhsSphere, *rhsCapsule, outContact);
        }
        else if (const auto* rhsCylinder = std::get_if<Cylinder>(&rhs.shape))
        {
            hasCollision = CheckSphereCylinder(*lhsSphere, *rhsCylinder, outContact);
        }
        else if (const auto* rhsBox = std::get_if<Box>(&rhs.shape))
        {
            hasCollision = CheckSphereBox(*lhsSphere, *rhsBox, outContact);
        }
        else if (const auto* rhsRamp = std::get_if<Ramp>(&rhs.shape))
        {
            hasCollision = CheckSphereRamp(*lhsSphere, *rhsRamp, outContact);
        }
        else if (const auto* rhsTerrain = std::get_if<TerrainSurface>(&rhs.shape))
        {
            hasCollision = CheckSphereTerrain(*lhsSphere, *rhsTerrain, outContact);
        }
    }
    else if (const auto* lhsCylinder = std::get_if<Cylinder>(&lhs.shape))
    {
        if (const auto* rhsCylinder = std::get_if<Cylinder>(&rhs.shape))
        {
            hasCollision = CheckCylinderCylinder(*lhsCylinder, *rhsCylinder, outContact);
        }
        else if (const auto* rhsSphere = std::get_if<Sphere>(&rhs.shape))
        {
            hasCollision = CheckCylinderSphere(*lhsCylinder, *rhsSphere, outContact);
        }
        else if (const auto* rhsCapsule = std::get_if<Capsule>(&rhs.shape))
        {
            hasCollision = CheckCylinderCapsule(*lhsCylinder, *rhsCapsule, outContact);
        }
        else if (const auto* rhsBox = std::get_if<Box>(&rhs.shape))
        {
            hasCollision = CheckCylinderBox(*lhsCylinder, *rhsBox, outContact);
        }
        else if (const auto* rhsRamp = std::get_if<Ramp>(&rhs.shape))
        {
            hasCollision = CheckCylinderRamp(*lhsCylinder, *rhsRamp, outContact);
        }
        else if (const auto* rhsTerrain = std::get_if<TerrainSurface>(&rhs.shape))
        {
            hasCollision = CheckCylinderTerrain(*lhsCylinder, *rhsTerrain, outContact);
        }
    }
    else if (const auto* lhsBox = std::get_if<Box>(&lhs.shape))
    {
        if (const auto* rhsCapsule = std::get_if<Capsule>(&rhs.shape))
        {
            hasCollision = CheckBoxCapsule(*lhsBox, *rhsCapsule, outContact);
        }
        else if (const auto* rhsSphere = std::get_if<Sphere>(&rhs.shape))
        {
            hasCollision = CheckBoxSphere(*lhsBox, *rhsSphere, outContact);
        }
        else if (const auto* rhsCylinder = std::get_if<Cylinder>(&rhs.shape))
        {
            hasCollision = CheckBoxCylinder(*lhsBox, *rhsCylinder, outContact);
        }
    }
    else if (const auto* lhsRamp = std::get_if<Ramp>(&lhs.shape))
    {
        if (const auto* rhsCapsule = std::get_if<Capsule>(&rhs.shape))
        {
            hasCollision = CheckRampCapsule(*lhsRamp, *rhsCapsule, outContact);
        }
        else if (const auto* rhsSphere = std::get_if<Sphere>(&rhs.shape))
        {
            hasCollision = CheckRampSphere(*lhsRamp, *rhsSphere, outContact);
        }
        else if (const auto* rhsCylinder = std::get_if<Cylinder>(&rhs.shape))
        {
            hasCollision = CheckRampCylinder(*lhsRamp, *rhsCylinder, outContact);
        }
    }
    else if (const auto* lhsTerrain = std::get_if<TerrainSurface>(&lhs.shape))
    {
        if (const auto* rhsCapsule = std::get_if<Capsule>(&rhs.shape))
        {
            hasCollision = CheckTerrainCapsule(*lhsTerrain, *rhsCapsule, outContact);
        }
        else if (const auto* rhsSphere = std::get_if<Sphere>(&rhs.shape))
        {
            hasCollision = CheckTerrainSphere(*lhsTerrain, *rhsSphere, outContact);
        }
        else if (const auto* rhsCylinder = std::get_if<Cylinder>(&rhs.shape))
        {
            hasCollision = CheckTerrainCylinder(*lhsTerrain, *rhsCylinder, outContact);
        }
    }

    if (hasCollision)
    {
        outContact.objectA = lhs.objectId;
        outContact.objectB = rhs.objectId;
    }

    return hasCollision;
}
} // namespace Kimgane::Shared::Physics::CollisionQueries
