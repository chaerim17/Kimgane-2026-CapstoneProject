#include "Pch.h"

#include "ColliderComponent.h"

#include "../Core/GameObject.h"

#include <array>
#include <cfloat>

namespace Kimgane::Engine
{
ColliderComponent::ColliderComponent(GameObject& owner, ColliderType type) noexcept
    : Component(owner), type_(type)
{
}

ColliderType ColliderComponent::GetType() const noexcept
{
    return type_;
}

BoxColliderComponent::BoxColliderComponent(GameObject& owner, const DirectX::XMFLOAT3& centerM,
                                           const DirectX::XMFLOAT3& sizeM)
    : ColliderComponent(owner, ColliderType::Box)
{
    localBox_.Center = centerM;
    localBox_.Extents = {Max(sizeM.x, 0.001F) * 0.5F,
                         Max(sizeM.y, 0.001F) * 0.5F,
                         Max(sizeM.z, 0.001F) * 0.5F};
    localBox_.Orientation = {0.0F, 0.0F, 0.0F, 1.0F};
    Update(0.0F);
}

void BoxColliderComponent::Update(float deltaTimeSec)
{
    (void)deltaTimeSec;

    const DirectX::XMMATRIX worldMatrix = GetOwner().GetTransform().GetWorldMatrix();
    localBox_.Transform(worldBox_, worldMatrix);

    std::array<DirectX::XMFLOAT3, 8> corners = {};
    worldBox_.GetCorners(corners.data());
    DirectX::BoundingBox::CreateFromPoints(worldAabb_, corners.size(), corners.data(), sizeof(DirectX::XMFLOAT3));
}

const DirectX::BoundingOrientedBox& BoxColliderComponent::GetWorldBox() const noexcept
{
    return worldBox_;
}

const DirectX::BoundingBox& BoxColliderComponent::GetWorldAabb() const noexcept
{
    return worldAabb_;
}

bool BoxColliderComponent::Raycast(const DirectX::XMFLOAT3& originM, const DirectX::XMFLOAT3& direction,
                                   float& outDistanceM) const noexcept
{
    DirectX::XMVECTOR rayDirection = DirectX::XMLoadFloat3(&direction);
    const float directionLengthSq = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(rayDirection));
    if (directionLengthSq <= 0.000001F)
    {
        outDistanceM = FLT_MAX;
        return false;
    }

    rayDirection = DirectX::XMVector3Normalize(rayDirection);
    return worldBox_.Intersects(DirectX::XMLoadFloat3(&originM), rayDirection, outDistanceM);
}

float BoxColliderComponent::Max(float lhs, float rhs) noexcept
{
    return lhs > rhs ? lhs : rhs;
}
} // namespace Kimgane::Engine
