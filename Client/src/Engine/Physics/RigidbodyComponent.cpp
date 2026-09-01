#include "Pch.h"

#include "RigidbodyComponent.h"

#include "../Core/GameObject.h"
#include "../../Shared/Physics/RigidbodyIntegrator.h"

#include <algorithm>

namespace Kimgane::Engine
{
namespace
{
namespace SharedPhysics = Kimgane::Shared::Physics;
namespace SharedRigidbody = Kimgane::Shared::Physics::RigidbodyIntegrator;

SharedPhysics::Vec3 ToSharedVec3(const DirectX::XMFLOAT3& value) noexcept
{
    return {value.x, value.y, value.z};
}

DirectX::XMFLOAT3 ToEngineVec3(const SharedPhysics::Vec3& value) noexcept
{
    return {value.x, value.y, value.z};
}
} // namespace

RigidbodyComponent::RigidbodyComponent(GameObject& owner) noexcept
    : Component(owner)
{
    mState.positionM = ToSharedVec3(owner.GetTransform().GetPositionM());
    RefreshVelocityCache();
}

void RigidbodyComponent::Update(float deltaTimeSec)
{
    mState.positionM = ToSharedVec3(GetOwner().GetTransform().GetPositionM());
    SharedRigidbody::Integrate(mState, deltaTimeSec);
    GetOwner().GetTransform().SetPositionM(ToEngineVec3(mState.positionM));
    RefreshVelocityCache();
}

void RigidbodyComponent::AddForce(const DirectX::XMFLOAT3& force, ForceMode mode) noexcept
{
    SharedRigidbody::AddForce(mState, ToSharedVec3(force), mode);
    RefreshVelocityCache();
}

const DirectX::XMFLOAT3& RigidbodyComponent::GetVelocityMps() const noexcept
{
    return mVelocityCacheMps;
}

float RigidbodyComponent::GetMassKg() const noexcept
{
    return mState.massKg;
}

bool RigidbodyComponent::IsKinematic() const noexcept
{
    return mState.isKinematic;
}

bool RigidbodyComponent::IsGrounded() const noexcept
{
    return mState.isGrounded;
}

bool RigidbodyComponent::UsesGravity() const noexcept
{
    return mState.useGravity;
}

const SharedPhysics::RigidbodyState& RigidbodyComponent::GetSharedState() const noexcept
{
    return mState;
}

void RigidbodyComponent::SetVelocityMps(const DirectX::XMFLOAT3& velocityMps) noexcept
{
    mState.velocityMps = ToSharedVec3(velocityMps);
    RefreshVelocityCache();
}

void RigidbodyComponent::SetMassKg(float massKg) noexcept
{
    mState.massKg = std::max(massKg, SharedPhysics::Settings::MIN_MASS_KG);
}

void RigidbodyComponent::SetDragPerSec(float dragPerSec) noexcept
{
    mState.dragPerSec = std::max(dragPerSec, 0.0F);
}

void RigidbodyComponent::SetGroundFrictionPerSec(float groundFrictionPerSec) noexcept
{
    mState.groundFrictionPerSec = std::max(groundFrictionPerSec, 0.0F);
}

void RigidbodyComponent::SetRestitution(float restitution) noexcept
{
    mState.restitution = std::clamp(restitution, 0.0F, 1.0F);
}

void RigidbodyComponent::SetKinematic(bool kinematic) noexcept
{
    mState.isKinematic = kinematic;
}

void RigidbodyComponent::SetGrounded(bool grounded) noexcept
{
    mState.isGrounded = grounded;
}

void RigidbodyComponent::SetUseGravity(bool useGravity) noexcept
{
    mState.useGravity = useGravity;
}

void RigidbodyComponent::SetSharedState(const SharedPhysics::RigidbodyState& state) noexcept
{
    mState = state;
    GetOwner().GetTransform().SetPositionM(ToEngineVec3(mState.positionM));
    RefreshVelocityCache();
}

void RigidbodyComponent::RefreshVelocityCache() noexcept
{
    mVelocityCacheMps = ToEngineVec3(mState.velocityMps);
}
} // namespace Kimgane::Engine
