#include "Pch.h"

#include "CollisionManager.h"

#include "PhysicsSettings.h"
#include "TerrainColliderComponent.h"
#include "../../Shared/Physics/CollisionQueries.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <optional>

namespace Kimgane::Engine
{
namespace
{
namespace SharedPhysics = Kimgane::Shared::Physics;
namespace SharedQueries = Kimgane::Shared::Physics::CollisionQueries;

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

SharedPhysics::Vec3 ToSharedVec3(const DirectX::XMFLOAT3& value) noexcept
{
    return {value.x, value.y, value.z};
}

DirectX::XMFLOAT3 ToEngineVec3(const SharedPhysics::Vec3& value) noexcept
{
    return {value.x, value.y, value.z};
}

SharedPhysics::Box ToSharedBox(const BoxColliderComponent& collider) noexcept
{
    const DirectX::BoundingBox& aabb = collider.GetWorldAabb();
    return {ToSharedVec3(aabb.Center), ToSharedVec3(aabb.Extents)};
}

SharedPhysics::Sphere ToSharedSphere(const SphereColliderComponent& collider) noexcept
{
    const DirectX::BoundingSphere& sphere = collider.GetWorldSphere();
    return SharedPhysics::MakeSphere(ToSharedVec3(sphere.Center), sphere.Radius);
}

SharedPhysics::Capsule ToSharedCapsule(const CapsuleColliderComponent& collider) noexcept
{
    const DirectX::XMFLOAT3& startM = collider.GetWorldSegmentStartM();
    const DirectX::XMFLOAT3& endM = collider.GetWorldSegmentEndM();
    const DirectX::XMFLOAT3 deltaM = {endM.x - startM.x, endM.y - startM.y, endM.z - startM.z};
    const float segmentLengthM = std::sqrt(deltaM.x * deltaM.x + deltaM.y * deltaM.y + deltaM.z * deltaM.z);

    // Shared capsule은 서버 이동 판정에 맞춘 세로 capsule입니다.
    // 현재 클라 플레이어/NPC capsule도 회전 없이 +Y 기준으로 쓰기 때문에 동일하게 변환됩니다.
    // 나중에 회전 capsule이 필요해지면 Shared 쪽에 OrientedCapsule 타입을 새로 추가하는 편이 안전합니다.
    return SharedPhysics::MakeCapsuleFromCenter(ToSharedVec3(collider.GetWorldCenterM()),
                                                collider.GetWorldRadiusM(),
                                                segmentLengthM + collider.GetWorldRadiusM() * 2.0F);
}

SharedPhysics::Cylinder ToSharedCylinder(const CylinderColliderComponent& collider) noexcept
{
    return collider.GetWorldCylinder();
}

SharedPhysics::Ramp ToSharedRamp(const RampColliderComponent& collider) noexcept
{
    return collider.GetWorldRamp();
}

void CopySharedContact(const SharedPhysics::ContactInfo& sharedContact, ContactInfo& outContact) noexcept
{
    outContact.normal = ToEngineVec3(sharedContact.normal);
    outContact.surfaceNormal = ToEngineVec3(sharedContact.surfaceNormal);
    outContact.penetrationM = sharedContact.penetrationM;
    outContact.isTerrainContact = sharedContact.isTerrainContact;
    outContact.isGroundCandidate = sharedContact.isGroundCandidate;
    outContact.isWalkable = sharedContact.isWalkable;
    outContact.slopeAngleRad = sharedContact.slopeAngleRad;
}

class ClientTerrainSampler final : public SharedPhysics::TerrainSampler
{
public:
    explicit ClientTerrainSampler(const TerrainColliderComponent& terrain) noexcept
        : mTerrain(terrain)
    {
    }

    [[nodiscard]] bool SampleHeightAtWorld(const SharedPhysics::Vec3& worldPositionM,
                                           SharedPhysics::TerrainSample& outSample) const noexcept override
    {
        float heightM = 0.0F;
        DirectX::XMFLOAT3 normal = {};
        if (!mTerrain.GetHeightAtWorld(ToEngineVec3(worldPositionM), heightM, normal))
        {
            outSample = {};
            return false;
        }

        // Shared/Physics는 DirectX를 모르기 때문에 여기서 클라 지형 normal을 순수 Vec3로 바꿔줍니다.
        // 이 어댑터를 서버가 그대로 쓰는 것은 아니고, 서버는 ServerTerrainSampler를 따로 만들면 됩니다.
        outSample.heightM = heightM;
        outSample.normal = ToSharedVec3(normal);
        return true;
    }

private:
    const TerrainColliderComponent& mTerrain;
};

bool TryBuildSharedBody(ColliderComponent& collider,
                        const ClientTerrainSampler* terrainSampler,
                        SharedPhysics::CollisionBody& outBody) noexcept
{
    outBody = {};
    outBody.objectId = SharedPhysics::INVALID_OBJECT_ID;

    switch (collider.GetType())
    {
    case ColliderType::Box:
        if (const auto* boxCollider = dynamic_cast<BoxColliderComponent*>(&collider))
        {
            outBody.shape = ToSharedBox(*boxCollider);
            return true;
        }
        break;
    case ColliderType::Sphere:
        if (const auto* sphereCollider = dynamic_cast<SphereColliderComponent*>(&collider))
        {
            outBody.shape = ToSharedSphere(*sphereCollider);
            return true;
        }
        break;
    case ColliderType::Capsule:
        if (const auto* capsuleCollider = dynamic_cast<CapsuleColliderComponent*>(&collider))
        {
            outBody.shape = ToSharedCapsule(*capsuleCollider);
            return true;
        }
        break;
    case ColliderType::Cylinder:
        if (const auto* cylinderCollider = dynamic_cast<CylinderColliderComponent*>(&collider))
        {
            outBody.shape = ToSharedCylinder(*cylinderCollider);
            return true;
        }
        break;
    case ColliderType::Ramp:
        if (const auto* rampCollider = dynamic_cast<RampColliderComponent*>(&collider))
        {
            outBody.shape = ToSharedRamp(*rampCollider);
            return true;
        }
        break;
    case ColliderType::Terrain:
        if (terrainSampler != nullptr)
        {
            outBody.shape = SharedPhysics::TerrainSurface{terrainSampler};
            return true;
        }
        break;
    }

    return false;
}

bool CheckSharedCollision(ColliderComponent& a, ColliderComponent& b, ContactInfo& outContact) noexcept
{
    std::optional<ClientTerrainSampler> terrainSamplerA;
    std::optional<ClientTerrainSampler> terrainSamplerB;

    if (auto* terrainA = dynamic_cast<TerrainColliderComponent*>(&a))
    {
        terrainSamplerA.emplace(*terrainA);
    }
    if (auto* terrainB = dynamic_cast<TerrainColliderComponent*>(&b))
    {
        terrainSamplerB.emplace(*terrainB);
    }

    SharedPhysics::CollisionBody sharedA = {};
    SharedPhysics::CollisionBody sharedB = {};
    if (!TryBuildSharedBody(a, terrainSamplerA ? &(*terrainSamplerA) : nullptr, sharedA) ||
        !TryBuildSharedBody(b, terrainSamplerB ? &(*terrainSamplerB) : nullptr, sharedB))
    {
        return false;
    }

    SharedPhysics::ContactInfo sharedContact = {};
    if (!SharedQueries::CheckCollision(sharedA, sharedB, sharedContact))
    {
        return false;
    }

    CopySharedContact(sharedContact, outContact);
    return true;
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

bool CollisionManager::CheckCollision(ColliderComponent& a,
                                      ColliderComponent& b,
                                      ContactInfo& outContact) const noexcept
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

    if (a.GetType() == ColliderType::Ramp && b.GetType() == ColliderType::Capsule)
    {
        return CheckRampCapsule(a, b, outContact);
    }

    if (a.GetType() == ColliderType::Capsule && b.GetType() == ColliderType::Ramp)
    {
        return CheckCapsuleRamp(a, b, outContact);
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

    return CheckSharedCollision(a, b, outContact);
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
    outContact.isGroundCandidate = false;
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
    if (boxCollider == nullptr || capsuleCollider == nullptr)
    {
        return false;
    }

    SharedPhysics::ContactInfo sharedContact = {};
    if (!SharedQueries::CheckBoxCapsule(ToSharedBox(*boxCollider), ToSharedCapsule(*capsuleCollider), sharedContact))
    {
        return false;
    }

    CopySharedContact(sharedContact, outContact);
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

    SharedPhysics::ContactInfo sharedContact = {};
    if (!SharedQueries::CheckCapsuleCapsule(ToSharedCapsule(*lhs), ToSharedCapsule(*rhs), sharedContact))
    {
        return false;
    }

    CopySharedContact(sharedContact, outContact);
    return true;
}

bool CollisionManager::CheckRampCapsule(ColliderComponent& ramp,
                                        ColliderComponent& capsule,
                                        ContactInfo& outContact) noexcept
{
    const auto* rampCollider = dynamic_cast<RampColliderComponent*>(&ramp);
    const auto* capsuleCollider = dynamic_cast<CapsuleColliderComponent*>(&capsule);
    if (rampCollider == nullptr || capsuleCollider == nullptr)
    {
        return false;
    }

    SharedPhysics::ContactInfo sharedContact = {};
    if (!SharedQueries::CheckRampCapsule(ToSharedRamp(*rampCollider), ToSharedCapsule(*capsuleCollider), sharedContact))
    {
        return false;
    }

    CopySharedContact(sharedContact, outContact);
    return true;
}

bool CollisionManager::CheckCapsuleRamp(ColliderComponent& capsule,
                                        ColliderComponent& ramp,
                                        ContactInfo& outContact) noexcept
{
    const auto* capsuleCollider = dynamic_cast<CapsuleColliderComponent*>(&capsule);
    const auto* rampCollider = dynamic_cast<RampColliderComponent*>(&ramp);
    if (capsuleCollider == nullptr || rampCollider == nullptr)
    {
        return false;
    }

    SharedPhysics::ContactInfo sharedContact = {};
    if (!SharedQueries::CheckCapsuleRamp(ToSharedCapsule(*capsuleCollider), ToSharedRamp(*rampCollider), sharedContact))
    {
        return false;
    }

    CopySharedContact(sharedContact, outContact);
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
    outContact.isGroundCandidate = true;
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

    ClientTerrainSampler terrainSampler(*terrainCollider);
    const SharedPhysics::TerrainSurface terrainSurface{&terrainSampler};
    SharedPhysics::ContactInfo sharedContact = {};
    if (!SharedQueries::CheckTerrainCapsule(terrainSurface, ToSharedCapsule(*capsuleCollider), sharedContact))
    {
        return false;
    }

    CopySharedContact(sharedContact, outContact);
    return true;
}
} // namespace Kimgane::Engine
