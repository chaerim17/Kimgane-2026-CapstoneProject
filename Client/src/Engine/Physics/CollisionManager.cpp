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
} // namespace

void CollisionManager::AddCollider(ColliderComponent& collider)
{
    if (std::find(colliders_.begin(), colliders_.end(), &collider) == colliders_.end())
    {
        colliders_.push_back(&collider);
    }
}

void CollisionManager::RemoveCollider(const ColliderComponent& collider)
{
    colliders_.erase(std::remove(colliders_.begin(), colliders_.end(), &collider), colliders_.end());
}

void CollisionManager::ClearColliders() noexcept
{
    colliders_.clear();
    while (!eventQueue_.empty())
    {
        eventQueue_.pop();
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

    for (ColliderComponent* collider : colliders_)
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

    return false;
}

void CollisionManager::ProcessCollisions(bool dispatchEvents)
{
    while (!eventQueue_.empty())
    {
        eventQueue_.pop();
    }

    for (std::size_t lhsIndex = 0; lhsIndex < colliders_.size(); ++lhsIndex)
    {
        ColliderComponent* lhs = colliders_[lhsIndex];
        if (lhs == nullptr)
        {
            continue;
        }

        for (std::size_t rhsIndex = lhsIndex + 1U; rhsIndex < colliders_.size(); ++rhsIndex)
        {
            ColliderComponent* rhs = colliders_[rhsIndex];
            if (rhs == nullptr)
            {
                continue;
            }

            ContactInfo contact = {};
            if (CheckCollision(*lhs, *rhs, contact))
            {
                eventQueue_.push({lhs, rhs});
            }
        }
    }

    if (!dispatchEvents)
    {
        return;
    }

    while (!eventQueue_.empty())
    {
        eventQueue_.pop();
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
    outContact.isWalkable = outContact.slopeAngleRad <= PhysicsSettings::kDefaultWalkableSlopeRad;
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
    outContact.isWalkable = outContact.slopeAngleRad <= PhysicsSettings::kDefaultWalkableSlopeRad;
    return true;
}
} // namespace Kimgane::Engine
