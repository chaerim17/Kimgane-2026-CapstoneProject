#include "Pch.h"

#include "RigidbodyComponent.h"

#include "../Core/GameObject.h"

#include <algorithm>

namespace Kimgane::Engine
{
namespace
{
DirectX::XMFLOAT3 Add(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs) noexcept
{
    return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

DirectX::XMFLOAT3 Scale(const DirectX::XMFLOAT3& value, float scalar) noexcept
{
    return {value.x * scalar, value.y * scalar, value.z * scalar};
}
} // namespace

RigidbodyComponent::RigidbodyComponent(GameObject& owner) noexcept
    : Component(owner)
{
}

void RigidbodyComponent::Update(float deltaTimeSec)
{
    if (isKinematic_ || deltaTimeSec <= 0.0F)
    {
        accumulatedForceN_ = {0.0F, 0.0F, 0.0F};
        return;
    }

    const DirectX::XMFLOAT3 accelerationMps2 = BuildAccelerationMps2();
    velocityMps_ = Add(velocityMps_, Scale(accelerationMps2, deltaTimeSec));
    ApplyDamping(deltaTimeSec);
    GetOwner().GetTransform().TranslateM(Scale(velocityMps_, deltaTimeSec));
    accumulatedForceN_ = {0.0F, 0.0F, 0.0F};
}

void RigidbodyComponent::AddForce(const DirectX::XMFLOAT3& force, ForceMode mode) noexcept
{
    switch (mode)
    {
    case ForceMode::Force:
        accumulatedForceN_ = Add(accumulatedForceN_, force);
        break;
    case ForceMode::Impulse:
        velocityMps_ = Add(velocityMps_, Scale(force, 1.0F / massKg_));
        break;
    case ForceMode::VelocityChange:
        velocityMps_ = Add(velocityMps_, force);
        break;
    }
}

const DirectX::XMFLOAT3& RigidbodyComponent::GetVelocityMps() const noexcept
{
    return velocityMps_;
}

float RigidbodyComponent::GetMassKg() const noexcept
{
    return massKg_;
}

bool RigidbodyComponent::IsKinematic() const noexcept
{
    return isKinematic_;
}

bool RigidbodyComponent::IsGrounded() const noexcept
{
    return isGrounded_;
}

bool RigidbodyComponent::UsesGravity() const noexcept
{
    return useGravity_;
}

void RigidbodyComponent::SetVelocityMps(const DirectX::XMFLOAT3& velocityMps) noexcept
{
    velocityMps_ = velocityMps;
}

void RigidbodyComponent::SetMassKg(float massKg) noexcept
{
    massKg_ = std::max(massKg, PhysicsSettings::kMinMassKg);
}

void RigidbodyComponent::SetDragPerSec(float dragPerSec) noexcept
{
    dragPerSec_ = std::max(dragPerSec, 0.0F);
}

void RigidbodyComponent::SetGroundFrictionPerSec(float groundFrictionPerSec) noexcept
{
    groundFrictionPerSec_ = std::max(groundFrictionPerSec, 0.0F);
}

void RigidbodyComponent::SetRestitution(float restitution) noexcept
{
    restitution_ = std::clamp(restitution, 0.0F, 1.0F);
}

void RigidbodyComponent::SetKinematic(bool kinematic) noexcept
{
    isKinematic_ = kinematic;
}

void RigidbodyComponent::SetGrounded(bool grounded) noexcept
{
    isGrounded_ = grounded;
}

void RigidbodyComponent::SetUseGravity(bool useGravity) noexcept
{
    useGravity_ = useGravity;
}

DirectX::XMFLOAT3 RigidbodyComponent::BuildAccelerationMps2() const noexcept
{
    DirectX::XMFLOAT3 accelerationMps2 = Scale(accumulatedForceN_, 1.0F / massKg_);
    if (useGravity_ && !isGrounded_)
    {
        accelerationMps2 = Add(accelerationMps2, Scale(PhysicsSettings::kGravityMps2, gravityScale_));
    }

    return accelerationMps2;
}

void RigidbodyComponent::ApplyDamping(float deltaTimeSec) noexcept
{
    const float dragScale = std::clamp(1.0F - dragPerSec_ * deltaTimeSec, 0.0F, 1.0F);
    velocityMps_ = Scale(velocityMps_, dragScale);

    if (isGrounded_)
    {
        const float frictionScale = std::clamp(1.0F - groundFrictionPerSec_ * deltaTimeSec, 0.0F, 1.0F);
        velocityMps_.x *= frictionScale;
        velocityMps_.z *= frictionScale;
    }
}
} // namespace Kimgane::Engine
