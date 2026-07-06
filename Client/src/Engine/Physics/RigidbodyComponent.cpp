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
    if (mIsKinematic || deltaTimeSec <= 0.0F)
    {
        mAccumulatedForceN = {0.0F, 0.0F, 0.0F};
        return;
    }

    const DirectX::XMFLOAT3 accelerationMps2 = BuildAccelerationMps2();
    mVelocityMps = Add(mVelocityMps, Scale(accelerationMps2, deltaTimeSec));
    ApplyDamping(deltaTimeSec);
    GetOwner().GetTransform().TranslateM(Scale(mVelocityMps, deltaTimeSec));
    mAccumulatedForceN = {0.0F, 0.0F, 0.0F};
}

void RigidbodyComponent::AddForce(const DirectX::XMFLOAT3& force, ForceMode mode) noexcept
{
    switch (mode)
    {
    case ForceMode::Force:
        mAccumulatedForceN = Add(mAccumulatedForceN, force);
        break;
    case ForceMode::Impulse:
        mVelocityMps = Add(mVelocityMps, Scale(force, 1.0F / mMassKg));
        break;
    case ForceMode::VelocityChange:
        mVelocityMps = Add(mVelocityMps, force);
        break;
    }
}

const DirectX::XMFLOAT3& RigidbodyComponent::GetVelocityMps() const noexcept
{
    return mVelocityMps;
}

float RigidbodyComponent::GetMassKg() const noexcept
{
    return mMassKg;
}

bool RigidbodyComponent::IsKinematic() const noexcept
{
    return mIsKinematic;
}

bool RigidbodyComponent::IsGrounded() const noexcept
{
    return mIsGrounded;
}

bool RigidbodyComponent::UsesGravity() const noexcept
{
    return mUseGravity;
}

void RigidbodyComponent::SetVelocityMps(const DirectX::XMFLOAT3& velocityMps) noexcept
{
    mVelocityMps = velocityMps;
}

void RigidbodyComponent::SetMassKg(float massKg) noexcept
{
    mMassKg = std::max(massKg, PhysicsSettings::MIN_MASS_KG);
}

void RigidbodyComponent::SetDragPerSec(float dragPerSec) noexcept
{
    mDragPerSec = std::max(dragPerSec, 0.0F);
}

void RigidbodyComponent::SetGroundFrictionPerSec(float groundFrictionPerSec) noexcept
{
    mGroundFrictionPerSec = std::max(groundFrictionPerSec, 0.0F);
}

void RigidbodyComponent::SetRestitution(float restitution) noexcept
{
    mRestitution = std::clamp(restitution, 0.0F, 1.0F);
}

void RigidbodyComponent::SetKinematic(bool kinematic) noexcept
{
    mIsKinematic = kinematic;
}

void RigidbodyComponent::SetGrounded(bool grounded) noexcept
{
    mIsGrounded = grounded;
}

void RigidbodyComponent::SetUseGravity(bool useGravity) noexcept
{
    mUseGravity = useGravity;
}

DirectX::XMFLOAT3 RigidbodyComponent::BuildAccelerationMps2() const noexcept
{
    DirectX::XMFLOAT3 accelerationMps2 = Scale(mAccumulatedForceN, 1.0F / mMassKg);
    if (mUseGravity && !mIsGrounded)
    {
        accelerationMps2 = Add(accelerationMps2, Scale(PhysicsSettings::GRAVITY_MPS2, mGravityScale));
    }

    return accelerationMps2;
}

void RigidbodyComponent::ApplyDamping(float deltaTimeSec) noexcept
{
    const float dragScale = std::clamp(1.0F - mDragPerSec * deltaTimeSec, 0.0F, 1.0F);
    mVelocityMps = Scale(mVelocityMps, dragScale);

    if (mIsGrounded)
    {
        const float frictionScale = std::clamp(1.0F - mGroundFrictionPerSec * deltaTimeSec, 0.0F, 1.0F);
        mVelocityMps.x *= frictionScale;
        mVelocityMps.z *= frictionScale;
    }
}
} // namespace Kimgane::Engine
