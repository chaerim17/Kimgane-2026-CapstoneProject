#pragma once

#include "../Core/Component.h"

#include <DirectXMath.h>

namespace Kimgane::Engine
{
class Camera;
class InputManager;
class NetworkManager;

class PlayerControllerComponent final : public Component
{
public:
    PlayerControllerComponent(GameObject& owner, const InputManager& inputManager, NetworkManager& networkManager) noexcept;

    void Update(float deltaTimeSec) override;

    void SetCamera(const Camera* camera) noexcept;
    void SetMoveSpeedMps(float moveSpeedMps) noexcept;
    void SetJumpVelocityMps(float jumpVelocityMps) noexcept;
    void SetJumpEnabled(bool jumpEnabled) noexcept;

    [[nodiscard]] float GetMoveSpeedMps() const noexcept;
    [[nodiscard]] float GetJumpVelocityMps() const noexcept;
    [[nodiscard]] bool IsJumpEnabled() const noexcept;

private:
    [[nodiscard]] DirectX::XMFLOAT3 BuildMovementDirection() const noexcept;
    void ApplyMovement(const DirectX::XMFLOAT3& direction, float deltaTimeSec) noexcept;
    void ApplyJump() noexcept;
    void FaceMovementDirection(const DirectX::XMFLOAT3& direction) noexcept;

    [[nodiscard]] static DirectX::XMFLOAT3 ProjectPlanar(const DirectX::XMFLOAT3& value,
                                                         const DirectX::XMFLOAT3& fallback) noexcept;

    const InputManager& mInputManager;
    NetworkManager& mNetworkManager;
    const Camera* mCamera = nullptr;
    float mMoveSpeedMps = 0.0F;
    float mJumpVelocityMps = 0.0F;
    bool mJumpEnabled = false;
};
} // namespace Kimgane::Engine
