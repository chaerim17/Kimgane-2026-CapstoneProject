#pragma once

#include "../Core/Component.h"
#include "../../Shared/Physics/RigidbodyTypes.h"

#include <DirectXMath.h>

namespace Kimgane::Engine
{
using ForceMode = Kimgane::Shared::Physics::ForceMode;

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
    [[nodiscard]] const Kimgane::Shared::Physics::RigidbodyState& GetSharedState() const noexcept;

    void SetVelocityMps(const DirectX::XMFLOAT3& velocityMps) noexcept;
    void SetMassKg(float massKg) noexcept;
    void SetDragPerSec(float dragPerSec) noexcept;
    void SetGroundFrictionPerSec(float groundFrictionPerSec) noexcept;
    void SetRestitution(float restitution) noexcept;
    void SetKinematic(bool kinematic) noexcept;
    void SetGrounded(bool grounded) noexcept;
    void SetUseGravity(bool useGravity) noexcept;
    void SetSharedState(const Kimgane::Shared::Physics::RigidbodyState& state) noexcept;

private:
    void RefreshVelocityCache() noexcept;

    Kimgane::Shared::Physics::RigidbodyState mState = {};
    DirectX::XMFLOAT3 mVelocityCacheMps = {0.0F, 0.0F, 0.0F};
};
} // namespace Kimgane::Engine
