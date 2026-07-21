#include "Pch.h"

#include "CollisionManager.h"

#include "PhysicsSettings.h"
#include "TerrainColliderComponent.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

namespace Kimgane::Engine
{
namespace
{
DirectX::XMFLOAT3 NormalizeOrUp(const DirectX::XMFLOAT3& value) noexcept
{
    const DirectX::XMVECTOR vector = DirectX::XMLoadFloat3(&value);
    if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(vector)) <= 0.000001F)
    {
        return {0.0F, 1.0F, 0.0F};
    }

    DirectX::XMFLOAT3 result = {};
    DirectX::XMStoreFloat3(&result, DirectX::XMVector3Normalize(vector));
    return result;
}

float BuildSlopeAngleRad(const DirectX::XMFLOAT3& normal) noexcept
{
    const float upDot = std::clamp(normal.y, -1.0F, 1.0F);
    return std::acos(upDot);
}

float ComputeAabbPenetrationM(const DirectX::BoundingBox& lhs, const DirectX::BoundingBox& rhs) noexcept
{
    const float overlapX = lhs.Extents.x + rhs.Extents.x - std::fabs(lhs.Center.x - rhs.Center.x);
    const float overlapY = lhs.Extents.y + rhs.Extents.y - std::fabs(lhs.Center.y - rhs.Center.y);
    const float overlapZ = lhs.Extents.z + rhs.Extents.z - std::fabs(lhs.Center.z - rhs.Center.z);
    return std::max(0.0F, std::min({overlapX, overlapY, overlapZ}));
}

DirectX::XMFLOAT3 Add(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs) noexcept
{
    return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

DirectX::XMFLOAT3 Subtract(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs) noexcept
{
    return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

DirectX::XMFLOAT3 Scale(const DirectX::XMFLOAT3& value, float scalar) noexcept
{
    return {value.x * scalar, value.y * scalar, value.z * scalar};
}

float Dot(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs) noexcept
{
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

float LengthSquared(const DirectX::XMFLOAT3& value) noexcept
{
    return Dot(value, value);
}

bool IntersectsAabb(const DirectX::BoundingBox& lhs, const DirectX::BoundingBox& rhs) noexcept
{
    return std::fabs(lhs.Center.x - rhs.Center.x) <= lhs.Extents.x + rhs.Extents.x &&
           std::fabs(lhs.Center.y - rhs.Center.y) <= lhs.Extents.y + rhs.Extents.y &&
           std::fabs(lhs.Center.z - rhs.Center.z) <= lhs.Extents.z + rhs.Extents.z;
}

float ComputeSegmentSegmentDistanceSquared(const DirectX::XMFLOAT3& startA,
                                           const DirectX::XMFLOAT3& endA,
                                           const DirectX::XMFLOAT3& startB,
                                           const DirectX::XMFLOAT3& endB) noexcept
{
    constexpr float EPSILON = 0.000001F;

    const DirectX::XMFLOAT3 d1 = Subtract(endA, startA);
    const DirectX::XMFLOAT3 d2 = Subtract(endB, startB);
    const DirectX::XMFLOAT3 r = Subtract(startA, startB);
    const float a = Dot(d1, d1);
    const float e = Dot(d2, d2);
    const float f = Dot(d2, r);

    float s = 0.0F;
    float t = 0.0F;

    if (a <= EPSILON && e <= EPSILON)
    {
        return LengthSquared(Subtract(startA, startB));
    }

    if (a <= EPSILON)
    {
        t = std::clamp(f / e, 0.0F, 1.0F);
    }
    else
    {
        const float c = Dot(d1, r);
        if (e <= EPSILON)
        {
            s = std::clamp(-c / a, 0.0F, 1.0F);
        }
        else
        {
            const float b = Dot(d1, d2);
            const float denominator = a * e - b * b;
            if (std::fabs(denominator) > EPSILON)
            {
                s = std::clamp((b * f - c * e) / denominator, 0.0F, 1.0F);
            }

            t = (b * s + f) / e;
            if (t < 0.0F)
            {
                t = 0.0F;
                s = std::clamp(-c / a, 0.0F, 1.0F);
            }
            else if (t > 1.0F)
            {
                t = 1.0F;
                s = std::clamp((b - c) / a, 0.0F, 1.0F);
            }
        }
    }

    const DirectX::XMFLOAT3 closestA = Add(startA, Scale(d1, s));
    const DirectX::XMFLOAT3 closestB = Add(startB, Scale(d2, t));
    return LengthSquared(Subtract(closestA, closestB));
}

void FillAabbContact(const DirectX::BoundingBox& lhs,
                     const DirectX::BoundingBox& rhs,
                     ContactInfo& outContact) noexcept
{
    outContact.normal = NormalizeOrUp({rhs.Center.x - lhs.Center.x,
                                       rhs.Center.y - lhs.Center.y,
                                       rhs.Center.z - lhs.Center.z});
    outContact.surfaceNormal = outContact.normal;
    outContact.penetrationM = ComputeAabbPenetrationM(lhs, rhs);
    outContact.isTerrainContact = false;
    outContact.slopeAngleRad = BuildSlopeAngleRad(outContact.surfaceNormal);
    outContact.isWalkable = outContact.slopeAngleRad <= PhysicsSettings::DEFAULT_WALKABLE_SLOPE_RAD;
}
} // namespace

void CollisionManager::AddCollider(ColliderComponent& collider)
{
    if (std::find(mColliders.begin(), mColliders.end(), &collider) == mColliders.end())
    {
        mColliders.push_back(&collider);
    }
}

void CollisionManager::RemoveCollider(const ColliderComponent& collider)
{
    mColliders.erase(std::remove(mColliders.begin(), mColliders.end(), &collider), mColliders.end());
}

void CollisionManager::ClearColliders() noexcept
{
    mColliders.clear();
    while (!mEventQueue.empty())
    {
        mEventQueue.pop();
    }
}

void CollisionManager::Update(bool dispatchEvents)
{
    ProcessCollisions(dispatchEvents);
}

bool CollisionManager::Raycast(const DirectX::XMFLOAT3& originM,
                               const DirectX::XMFLOAT3& direction,
                               float& outHitDistanceM,
                               const ColliderComponent* ignoreCollider) const noexcept
{
    ColliderComponent* ignoredHitCollider = nullptr;
    return Raycast(originM, direction, outHitDistanceM, ignoredHitCollider, ignoreCollider);
}

bool CollisionManager::Raycast(const DirectX::XMFLOAT3& originM,
                               const DirectX::XMFLOAT3& direction,
                               float& outHitDistanceM,
                               ColliderComponent*& outHitCollider,
                               const ColliderComponent* ignoreCollider) const noexcept
{
    bool hasHit = false;
    outHitDistanceM = FLT_MAX;
    outHitCollider = nullptr;

    for (ColliderComponent* collider : mColliders)
    {
        if (collider == nullptr || collider == ignoreCollider)
        {
            continue;
        }

        float distanceM = FLT_MAX;
        if (collider->Raycast(originM, direction, distanceM) && distanceM < outHitDistanceM)
        {
            hasHit = true;
            outHitDistanceM = distanceM;
            outHitCollider = collider;
        }
    }

    return hasHit;
}

bool CollisionManager::CheckCollision(ColliderComponent& a, ColliderComponent& b, ContactInfo& outContact) const noexcept
{
    if (a.GetType() == ColliderType::Box && b.GetType() == ColliderType::Box)
    {
        return CheckBoxBox(a, b, outContact);
    }

    if (a.GetType() == ColliderType::Box && b.GetType() == ColliderType::Capsule)
    {
        return CheckBoxCapsule(a, b, outContact);
    }

    if (a.GetType() == ColliderType::Capsule && b.GetType() == ColliderType::Box)
    {
        const bool hasCollision = CheckBoxCapsule(b, a, outContact);
        outContact.normal = {-outContact.normal.x, -outContact.normal.y, -outContact.normal.z};
        return hasCollision;
    }

    if (a.GetType() == ColliderType::Capsule && b.GetType() == ColliderType::Capsule)
    {
        return CheckCapsuleCapsule(a, b, outContact);
    }

    if (a.GetType() == ColliderType::Terrain && b.GetType() == ColliderType::Box)
    {
        return CheckTerrainBox(a, b, outContact);
    }

    if (a.GetType() == ColliderType::Box && b.GetType() == ColliderType::Terrain)
    {
        const bool hasCollision = CheckTerrainBox(b, a, outContact);
        outContact.normal = {-outContact.normal.x, -outContact.normal.y, -outContact.normal.z};
        return hasCollision;
    }

    if (a.GetType() == ColliderType::Terrain && b.GetType() == ColliderType::Capsule)
    {
        return CheckTerrainCapsule(a, b, outContact);
    }

    if (a.GetType() == ColliderType::Capsule && b.GetType() == ColliderType::Terrain)
    {
        const bool hasCollision = CheckTerrainCapsule(b, a, outContact);
        outContact.normal = {-outContact.normal.x, -outContact.normal.y, -outContact.normal.z};
        return hasCollision;
    }

    return false;
}

void CollisionManager::ProcessCollisions(bool dispatchEvents)
{
    while (!mEventQueue.empty())
    {
        mEventQueue.pop();
    }

    for (std::size_t lhsIndex = 0; lhsIndex < mColliders.size(); ++lhsIndex)
    {
        ColliderComponent* lhs = mColliders[lhsIndex];
        if (lhs == nullptr)
        {
            continue;
        }

        for (std::size_t rhsIndex = lhsIndex + 1U; rhsIndex < mColliders.size(); ++rhsIndex)
        {
            ColliderComponent* rhs = mColliders[rhsIndex];
            if (rhs == nullptr)
            {
                continue;
            }

            ContactInfo contact = {};
            if (CheckCollision(*lhs, *rhs, contact))
            {
                mEventQueue.push({lhs, rhs});
            }
        }
    }

    if (!dispatchEvents)
    {
        return;
    }

    while (!mEventQueue.empty())
    {
        mEventQueue.pop();
    }
}

bool CollisionManager::CheckBoxBox(ColliderComponent& a, ColliderComponent& b, ContactInfo& outContact) noexcept
{
    const auto* lhs = dynamic_cast<BoxColliderComponent*>(&a);
    const auto* rhs = dynamic_cast<BoxColliderComponent*>(&b);
    if (lhs == nullptr || rhs == nullptr || !lhs->GetWorldBox().Intersects(rhs->GetWorldBox()))
    {
        return false;
    }

    const DirectX::BoundingBox& lhsAabb = lhs->GetWorldAabb();
    const DirectX::BoundingBox& rhsAabb = rhs->GetWorldAabb();
    outContact.normal = NormalizeOrUp({rhsAabb.Center.x - lhsAabb.Center.x,
                                       rhsAabb.Center.y - lhsAabb.Center.y,
                                       rhsAabb.Center.z - lhsAabb.Center.z});
    outContact.surfaceNormal = outContact.normal;
    outContact.penetrationM = ComputeAabbPenetrationM(lhsAabb, rhsAabb);
    outContact.isTerrainContact = false;
    outContact.slopeAngleRad = BuildSlopeAngleRad(outContact.surfaceNormal);
    outContact.isWalkable = outContact.slopeAngleRad <= PhysicsSettings::DEFAULT_WALKABLE_SLOPE_RAD;
    return true;
}

bool CollisionManager::CheckBoxCapsule(ColliderComponent& box,
                                       ColliderComponent& capsule,
                                       ContactInfo& outContact) noexcept
{
    const auto* boxCollider = dynamic_cast<BoxColliderComponent*>(&box);
    const auto* capsuleCollider = dynamic_cast<CapsuleColliderComponent*>(&capsule);
    if (boxCollider == nullptr || capsuleCollider == nullptr ||
        !IntersectsAabb(boxCollider->GetWorldAabb(), capsuleCollider->GetWorldAabb()))
    {
        return false;
    }

    FillAabbContact(boxCollider->GetWorldAabb(), capsuleCollider->GetWorldAabb(), outContact);
    return true;
}

bool CollisionManager::CheckCapsuleCapsule(ColliderComponent& a,
                                           ColliderComponent& b,
                                           ContactInfo& outContact) noexcept
{
    const auto* lhs = dynamic_cast<CapsuleColliderComponent*>(&a);
    const auto* rhs = dynamic_cast<CapsuleColliderComponent*>(&b);
    if (lhs == nullptr || rhs == nullptr)
    {
        return false;
    }

    const float radiusSumM = lhs->GetWorldRadiusM() + rhs->GetWorldRadiusM();
    const float distanceSqM =
        ComputeSegmentSegmentDistanceSquared(lhs->GetWorldSegmentStartM(),
                                             lhs->GetWorldSegmentEndM(),
                                             rhs->GetWorldSegmentStartM(),
                                             rhs->GetWorldSegmentEndM());
    if (distanceSqM > radiusSumM * radiusSumM)
    {
        return false;
    }

    FillAabbContact(lhs->GetWorldAabb(), rhs->GetWorldAabb(), outContact);
    outContact.penetrationM = std::max(0.0F, radiusSumM - std::sqrt(distanceSqM));
    return true;
}

bool CollisionManager::CheckTerrainBox(ColliderComponent& terrain,
                                       ColliderComponent& box,
                                       ContactInfo& outContact) noexcept
{
    const auto* terrainCollider = dynamic_cast<TerrainColliderComponent*>(&terrain);
    const auto* boxCollider = dynamic_cast<BoxColliderComponent*>(&box);
    if (terrainCollider == nullptr || boxCollider == nullptr)
    {
        return false;
    }

    const DirectX::BoundingBox& boxAabb = boxCollider->GetWorldAabb();
    const DirectX::XMFLOAT3 samplePositionM = {boxAabb.Center.x,
                                               boxAabb.Center.y - boxAabb.Extents.y,
                                               boxAabb.Center.z};
    float terrainHeightM = 0.0F;
    DirectX::XMFLOAT3 terrainNormal = {};
    if (!terrainCollider->GetHeightAtWorld(samplePositionM, terrainHeightM, terrainNormal))
    {
        return false;
    }

    const float boxBottomM = boxAabb.Center.y - boxAabb.Extents.y;
    if (boxBottomM > terrainHeightM)
    {
        return false;
    }

    outContact.normal = terrainNormal;
    outContact.surfaceNormal = terrainNormal;
    outContact.penetrationM = terrainHeightM - boxBottomM;
    outContact.isTerrainContact = true;
    outContact.slopeAngleRad = BuildSlopeAngleRad(terrainNormal);
    outContact.isWalkable = outContact.slopeAngleRad <= PhysicsSettings::DEFAULT_WALKABLE_SLOPE_RAD;
    return true;
}

bool CollisionManager::CheckTerrainCapsule(ColliderComponent& terrain,
                                           ColliderComponent& capsule,
                                           ContactInfo& outContact) noexcept
{
    const auto* terrainCollider = dynamic_cast<TerrainColliderComponent*>(&terrain);
    const auto* capsuleCollider = dynamic_cast<CapsuleColliderComponent*>(&capsule);
    if (terrainCollider == nullptr || capsuleCollider == nullptr)
    {
        return false;
    }

    const DirectX::XMFLOAT3 samplePositionM = capsuleCollider->GetWorldCenterM();
    float terrainHeightM = 0.0F;
    DirectX::XMFLOAT3 terrainNormal = {};
    if (!terrainCollider->GetHeightAtWorld(samplePositionM, terrainHeightM, terrainNormal))
    {
        return false;
    }

    const float capsuleBottomM = capsuleCollider->GetWorldBottomM();
    if (capsuleBottomM > terrainHeightM)
    {
        return false;
    }

    outContact.normal = terrainNormal;
    outContact.surfaceNormal = terrainNormal;
    outContact.penetrationM = terrainHeightM - capsuleBottomM;
    outContact.isTerrainContact = true;
    outContact.slopeAngleRad = BuildSlopeAngleRad(terrainNormal);
    outContact.isWalkable = outContact.slopeAngleRad <= PhysicsSettings::DEFAULT_WALKABLE_SLOPE_RAD;
    return true;
}
} // namespace Kimgane::Engine
