#pragma once

#include "../Core/Component.h"
#include "PhysicsSettings.h"

#include <DirectXMath.h>

namespace Kimgane::Engine
{
enum class ForceMode
{
    Force,
    Impulse,
    VelocityChange
};

class RigidbodyComponent final : public Component
{
public:
    explicit RigidbodyComponent(GameObject& owner) noexcept;

    void Update(float deltaTimeSec) override;
    void AddForce(const DirectX::XMFLOAT3& force, ForceMode mode) noexcept;

    [[nodiscard]] const DirectX::XMFLOAT3& GetVelocityMps() const noexcept;
    [[nodiscard]] float GetMassKg() const noexcept;
    [[nodiscard]] bool IsKinematic() const noexcept;
    [[nodiscard]] bool IsGrounded() const noexcept;
    [[nodiscard]] bool UsesGravity() const noexcept;

    void SetVelocityMps(const DirectX::XMFLOAT3& velocityMps) noexcept;
    void SetMassKg(float massKg) noexcept;
    void SetDragPerSec(float dragPerSec) noexcept;
    void SetGroundFrictionPerSec(float groundFrictionPerSec) noexcept;
    void SetRestitution(float restitution) noexcept;
    void SetKinematic(bool kinematic) noexcept;
    void SetGrounded(bool grounded) noexcept;
    void SetUseGravity(bool useGravity) noexcept;

private:
    [[nodiscard]] DirectX::XMFLOAT3 BuildAccelerationMps2() const noexcept;
    void ApplyDamping(float deltaTimeSec) noexcept;

    float mMassKg = PhysicsSettings::DEFAULT_MASS_KG;
    float mDragPerSec = PhysicsSettings::DEFAULT_DRAG_PER_SEC;
    float mGroundFrictionPerSec = PhysicsSettings::DEFAULT_GROUND_FRICTION_PER_SEC;
    float mRestitution = PhysicsSettings::DEFAULT_RESTITUTION;
    float mGravityScale = 1.0F;
    bool mIsKinematic = false;
    bool mUseGravity = true;
    bool mIsGrounded = false;
    DirectX::XMFLOAT3 mVelocityMps = {0.0F, 0.0F, 0.0F};
    DirectX::XMFLOAT3 mAccumulatedForceN = {0.0F, 0.0F, 0.0F};
};
} // namespace Kimgane::Engine
