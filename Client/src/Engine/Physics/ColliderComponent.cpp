#include "Pch.h"

#include "ColliderComponent.h"

#include "../Core/GameObject.h"

#include <array>
#include <cfloat>
#include <cmath>

namespace Kimgane::Engine
{
namespace
{
namespace SharedPhysics = Kimgane::Shared::Physics;
} // namespace

ColliderComponent::ColliderComponent(GameObject& owner, ColliderType type) noexcept
    : Component(owner), mType(type)
{
}

ColliderType ColliderComponent::GetType() const noexcept
{
    return mType;
}

BoxColliderComponent::BoxColliderComponent(GameObject& owner, const DirectX::XMFLOAT3& centerM,
                                           const DirectX::XMFLOAT3& sizeM)
    : ColliderComponent(owner, ColliderType::Box)
{
    mLocalBox.Center = centerM;
    mLocalBox.Extents = {Max(sizeM.x, 0.001F) * 0.5F,
                         Max(sizeM.y, 0.001F) * 0.5F,
                         Max(sizeM.z, 0.001F) * 0.5F};
    mLocalBox.Orientation = {0.0F, 0.0F, 0.0F, 1.0F};
    Update(0.0F);
}

void BoxColliderComponent::Update(float deltaTimeSec)
{
    (void)deltaTimeSec;

    const DirectX::XMMATRIX worldMatrix = GetOwner().GetTransform().GetWorldMatrix();
    mLocalBox.Transform(mWorldBox, worldMatrix);

    std::array<DirectX::XMFLOAT3, 8> corners = {};
    mWorldBox.GetCorners(corners.data());
    DirectX::BoundingBox::CreateFromPoints(mWorldAabb, corners.size(), corners.data(), sizeof(DirectX::XMFLOAT3));
}

const DirectX::BoundingOrientedBox& BoxColliderComponent::GetWorldBox() const noexcept
{
    return mWorldBox;
}

const DirectX::BoundingBox& BoxColliderComponent::GetWorldAabb() const noexcept
{
    return mWorldAabb;
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
    return mWorldBox.Intersects(DirectX::XMLoadFloat3(&originM), rayDirection, outDistanceM);
}

float BoxColliderComponent::Max(float lhs, float rhs) noexcept
{
    return lhs > rhs ? lhs : rhs;
}

SphereColliderComponent::SphereColliderComponent(GameObject& owner,
                                                 const DirectX::XMFLOAT3& centerM,
                                                 float radiusM)
    : ColliderComponent(owner, ColliderType::Sphere)
{
    mLocalSphere.Center = centerM;
    mLocalSphere.Radius = Max(radiusM, 0.001F);
    Update(0.0F);
}

void SphereColliderComponent::Update(float deltaTimeSec)
{
    (void)deltaTimeSec;

    const DirectX::XMMATRIX worldMatrix = GetOwner().GetTransform().GetWorldMatrix();
    DirectX::XMStoreFloat3(&mWorldSphere.Center,
                           DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&mLocalSphere.Center), worldMatrix));

    const DirectX::XMFLOAT3& scale = GetOwner().GetTransform().GetScale();
    const float radiusScale = Max(Max(std::fabs(scale.x), std::fabs(scale.y)), std::fabs(scale.z));
    mWorldSphere.Radius = mLocalSphere.Radius * Max(radiusScale, 0.001F);
    mWorldAabb.Center = mWorldSphere.Center;
    mWorldAabb.Extents = {mWorldSphere.Radius, mWorldSphere.Radius, mWorldSphere.Radius};
}

const DirectX::BoundingSphere& SphereColliderComponent::GetWorldSphere() const noexcept
{
    return mWorldSphere;
}

const DirectX::BoundingBox& SphereColliderComponent::GetWorldAabb() const noexcept
{
    return mWorldAabb;
}

bool SphereColliderComponent::Raycast(const DirectX::XMFLOAT3& originM,
                                      const DirectX::XMFLOAT3& direction,
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
    return mWorldSphere.Intersects(DirectX::XMLoadFloat3(&originM), rayDirection, outDistanceM);
}

float SphereColliderComponent::Max(float lhs, float rhs) noexcept
{
    return lhs > rhs ? lhs : rhs;
}

CapsuleColliderComponent::CapsuleColliderComponent(GameObject& owner,
                                                   const DirectX::XMFLOAT3& centerM,
                                                   float radiusM,
                                                   float heightM)
    : ColliderComponent(owner, ColliderType::Capsule),
      mLocalCenterM(centerM),
      mLocalRadiusM(Max(radiusM, 0.001F)),
      mLocalHeightM(Max(heightM, Max(radiusM, 0.001F) * 2.0F))
{
    Update(0.0F);
}

void CapsuleColliderComponent::Update(float deltaTimeSec)
{
    (void)deltaTimeSec;

    const float halfSegmentM = Max((mLocalHeightM * 0.5F) - mLocalRadiusM, 0.0F);
    const DirectX::XMFLOAT3 localStartM = {mLocalCenterM.x, mLocalCenterM.y - halfSegmentM, mLocalCenterM.z};
    const DirectX::XMFLOAT3 localEndM = {mLocalCenterM.x, mLocalCenterM.y + halfSegmentM, mLocalCenterM.z};
    const DirectX::XMMATRIX worldMatrix = GetOwner().GetTransform().GetWorldMatrix();
    DirectX::XMStoreFloat3(&mWorldSegmentStartM,
                           DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&localStartM), worldMatrix));
    DirectX::XMStoreFloat3(&mWorldSegmentEndM,
                           DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&localEndM), worldMatrix));

    const DirectX::XMFLOAT3& scale = GetOwner().GetTransform().GetScale();
    const float radiusScale = Max(Max(std::fabs(scale.x), std::fabs(scale.y)), std::fabs(scale.z));
    mWorldRadiusM = mLocalRadiusM * Max(radiusScale, 0.001F);
    mWorldCenterM = {(mWorldSegmentStartM.x + mWorldSegmentEndM.x) * 0.5F,
                     (mWorldSegmentStartM.y + mWorldSegmentEndM.y) * 0.5F,
                     (mWorldSegmentStartM.z + mWorldSegmentEndM.z) * 0.5F};

    const DirectX::XMFLOAT3 minPointM = {std::min(mWorldSegmentStartM.x, mWorldSegmentEndM.x) - mWorldRadiusM,
                                        std::min(mWorldSegmentStartM.y, mWorldSegmentEndM.y) - mWorldRadiusM,
                                        std::min(mWorldSegmentStartM.z, mWorldSegmentEndM.z) - mWorldRadiusM};
    const DirectX::XMFLOAT3 maxPointM = {std::max(mWorldSegmentStartM.x, mWorldSegmentEndM.x) + mWorldRadiusM,
                                        std::max(mWorldSegmentStartM.y, mWorldSegmentEndM.y) + mWorldRadiusM,
                                        std::max(mWorldSegmentStartM.z, mWorldSegmentEndM.z) + mWorldRadiusM};
    mWorldAabb.Center = {(minPointM.x + maxPointM.x) * 0.5F,
                         (minPointM.y + maxPointM.y) * 0.5F,
                         (minPointM.z + maxPointM.z) * 0.5F};
    mWorldAabb.Extents = {(maxPointM.x - minPointM.x) * 0.5F,
                          (maxPointM.y - minPointM.y) * 0.5F,
                          (maxPointM.z - minPointM.z) * 0.5F};
}

const DirectX::XMFLOAT3& CapsuleColliderComponent::GetWorldSegmentStartM() const noexcept
{
    return mWorldSegmentStartM;
}

const DirectX::XMFLOAT3& CapsuleColliderComponent::GetWorldSegmentEndM() const noexcept
{
    return mWorldSegmentEndM;
}

const DirectX::XMFLOAT3& CapsuleColliderComponent::GetWorldCenterM() const noexcept
{
    return mWorldCenterM;
}

float CapsuleColliderComponent::GetWorldRadiusM() const noexcept
{
    return mWorldRadiusM;
}

float CapsuleColliderComponent::GetWorldBottomM() const noexcept
{
    return std::min(mWorldSegmentStartM.y, mWorldSegmentEndM.y) - mWorldRadiusM;
}

const DirectX::BoundingBox& CapsuleColliderComponent::GetWorldAabb() const noexcept
{
    return mWorldAabb;
}

bool CapsuleColliderComponent::Raycast(const DirectX::XMFLOAT3& originM,
                                       const DirectX::XMFLOAT3& direction,
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
    return mWorldAabb.Intersects(DirectX::XMLoadFloat3(&originM), rayDirection, outDistanceM);
}

float CapsuleColliderComponent::Max(float lhs, float rhs) noexcept
{
    return lhs > rhs ? lhs : rhs;
}

CylinderColliderComponent::CylinderColliderComponent(GameObject& owner,
                                                     const DirectX::XMFLOAT3& centerM,
                                                     float radiusM,
                                                     float heightM)
    : ColliderComponent(owner, ColliderType::Cylinder),
      mLocalCenterM(centerM),
      mLocalRadiusM(Max(radiusM, 0.001F)),
      mLocalHeightM(Max(heightM, 0.001F))
{
    Update(0.0F);
}

void CylinderColliderComponent::Update(float deltaTimeSec)
{
    (void)deltaTimeSec;

    DirectX::XMFLOAT3 worldCenterM = {};
    const DirectX::XMMATRIX worldMatrix = GetOwner().GetTransform().GetWorldMatrix();
    DirectX::XMStoreFloat3(&worldCenterM,
                           DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&mLocalCenterM), worldMatrix));

    const DirectX::XMFLOAT3& scale = GetOwner().GetTransform().GetScale();
    const float radiusScale = Max(std::fabs(scale.x), std::fabs(scale.z));
    const float heightScale = std::fabs(scale.y);
    mWorldCylinder = SharedPhysics::MakeCylinderFromCenter({worldCenterM.x, worldCenterM.y, worldCenterM.z},
                                                           mLocalRadiusM * Max(radiusScale, 0.001F),
                                                           mLocalHeightM * Max(heightScale, 0.001F));
    mWorldAabb.Center = worldCenterM;
    mWorldAabb.Extents = {mWorldCylinder.radiusM, mWorldCylinder.heightM * 0.5F, mWorldCylinder.radiusM};
}

const SharedPhysics::Cylinder& CylinderColliderComponent::GetWorldCylinder() const noexcept
{
    return mWorldCylinder;
}

const DirectX::BoundingBox& CylinderColliderComponent::GetWorldAabb() const noexcept
{
    return mWorldAabb;
}

bool CylinderColliderComponent::Raycast(const DirectX::XMFLOAT3& originM,
                                        const DirectX::XMFLOAT3& direction,
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
    return mWorldAabb.Intersects(DirectX::XMLoadFloat3(&originM), rayDirection, outDistanceM);
}

float CylinderColliderComponent::Max(float lhs, float rhs) noexcept
{
    return lhs > rhs ? lhs : rhs;
}

RampColliderComponent::RampColliderComponent(GameObject& owner,
                                             const DirectX::XMFLOAT3& centerM,
                                             const DirectX::XMFLOAT3& sizeM,
                                             RampDirection direction)
    : ColliderComponent(owner, ColliderType::Ramp),
      mLocalDirection(direction)
{
    mLocalBounds.Center = centerM;
    mLocalBounds.Extents = {Max(sizeM.x, 0.001F) * 0.5F,
                            Max(sizeM.y, 0.001F) * 0.5F,
                            Max(sizeM.z, 0.001F) * 0.5F};
    mLocalBounds.Orientation = {0.0F, 0.0F, 0.0F, 1.0F};
    Update(0.0F);
}

void RampColliderComponent::Update(float deltaTimeSec)
{
    (void)deltaTimeSec;

    DirectX::BoundingOrientedBox worldBounds = {};
    const DirectX::XMMATRIX worldMatrix = GetOwner().GetTransform().GetWorldMatrix();
    mLocalBounds.Transform(worldBounds, worldMatrix);

    std::array<DirectX::XMFLOAT3, 8> corners = {};
    worldBounds.GetCorners(corners.data());
    DirectX::BoundingBox::CreateFromPoints(mWorldAabb, corners.size(), corners.data(), sizeof(DirectX::XMFLOAT3));

    const DirectX::XMFLOAT3 localDirection = ToLocalDirection(mLocalDirection);
    DirectX::XMVECTOR worldDirection =
        DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&localDirection), worldMatrix);
    worldDirection = DirectX::XMVectorSetY(worldDirection, 0.0F);

    DirectX::XMFLOAT3 worldDirectionM = {};
    DirectX::XMStoreFloat3(&worldDirectionM, worldDirection);

    mWorldRamp = SharedPhysics::MakeRamp({mWorldAabb.Center.x, mWorldAabb.Center.y, mWorldAabb.Center.z},
                                         {mWorldAabb.Extents.x * 2.0F,
                                          mWorldAabb.Extents.y * 2.0F,
                                          mWorldAabb.Extents.z * 2.0F},
                                         PickDominantWorldDirection(worldDirectionM));
}

const SharedPhysics::Ramp& RampColliderComponent::GetWorldRamp() const noexcept
{
    return mWorldRamp;
}

const DirectX::BoundingBox& RampColliderComponent::GetWorldAabb() const noexcept
{
    return mWorldAabb;
}

bool RampColliderComponent::Raycast(const DirectX::XMFLOAT3& originM,
                                    const DirectX::XMFLOAT3& direction,
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
    return mWorldAabb.Intersects(DirectX::XMLoadFloat3(&originM), rayDirection, outDistanceM);
}

float RampColliderComponent::Max(float lhs, float rhs) noexcept
{
    return lhs > rhs ? lhs : rhs;
}

DirectX::XMFLOAT3 RampColliderComponent::ToLocalDirection(RampDirection direction) noexcept
{
    switch (direction)
    {
    case RampDirection::PositiveX:
        return {1.0F, 0.0F, 0.0F};
    case RampDirection::NegativeX:
        return {-1.0F, 0.0F, 0.0F};
    case RampDirection::NegativeZ:
        return {0.0F, 0.0F, -1.0F};
    case RampDirection::PositiveZ:
    default:
        return {0.0F, 0.0F, 1.0F};
    }
}

RampColliderComponent::RampDirection RampColliderComponent::PickDominantWorldDirection(
    const DirectX::XMFLOAT3& direction) noexcept
{
    if (std::fabs(direction.x) >= std::fabs(direction.z))
    {
        return direction.x >= 0.0F ? RampDirection::PositiveX : RampDirection::NegativeX;
    }

    return direction.z >= 0.0F ? RampDirection::PositiveZ : RampDirection::NegativeZ;
}
} // namespace Kimgane::Engine
