#pragma once

#include "ColliderComponent.h"

#include <DirectXMath.h>

#include <queue>
#include <vector>

namespace Kimgane::Engine
{
struct CollisionEvent
{
    ColliderComponent* colliderA = nullptr;
    ColliderComponent* colliderB = nullptr;
};

struct ContactInfo
{
    DirectX::XMFLOAT3 normal = {0.0F, 1.0F, 0.0F};
    DirectX::XMFLOAT3 surfaceNormal = {0.0F, 1.0F, 0.0F};
    float penetrationM = 0.0F;
    bool isTerrainContact = false;
    bool isWalkable = true;
    float slopeAngleRad = 0.0F;
};

class CollisionManager final
{
public:
    void AddCollider(ColliderComponent& collider);
    void RemoveCollider(const ColliderComponent& collider);
    void ClearColliders() noexcept;
    void Update(bool dispatchEvents = false);

    [[nodiscard]] bool Raycast(const DirectX::XMFLOAT3& originM,
                               const DirectX::XMFLOAT3& direction,
                               float& outHitDistanceM,
                               const ColliderComponent* ignoreCollider = nullptr) const noexcept;
    [[nodiscard]] bool Raycast(const DirectX::XMFLOAT3& originM,
                               const DirectX::XMFLOAT3& direction,
                               float& outHitDistanceM,
                               ColliderComponent*& outHitCollider,
                               const ColliderComponent* ignoreCollider = nullptr) const noexcept;
    [[nodiscard]] bool CheckCollision(ColliderComponent& a, ColliderComponent& b, ContactInfo& outContact) const noexcept;

private:
    void ProcessCollisions(bool dispatchEvents);
    [[nodiscard]] static bool CheckBoxBox(ColliderComponent& a,
                                          ColliderComponent& b,
                                          ContactInfo& outContact) noexcept;
    [[nodiscard]] static bool CheckBoxCapsule(ColliderComponent& box,
                                              ColliderComponent& capsule,
                                              ContactInfo& outContact) noexcept;
    [[nodiscard]] static bool CheckCapsuleCapsule(ColliderComponent& a,
                                                  ColliderComponent& b,
                                                  ContactInfo& outContact) noexcept;
    [[nodiscard]] static bool CheckTerrainBox(ColliderComponent& terrain,
                                              ColliderComponent& box,
                                              ContactInfo& outContact) noexcept;
    [[nodiscard]] static bool CheckTerrainCapsule(ColliderComponent& terrain,
                                                  ColliderComponent& capsule,
                                                  ContactInfo& outContact) noexcept;

    std::vector<ColliderComponent*> mColliders;
    std::queue<CollisionEvent> mEventQueue;
};
} // namespace Kimgane::Engine
