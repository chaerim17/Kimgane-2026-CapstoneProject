#pragma once

#include "../Core/Component.h"
#include "../../Shared/Physics/CharacterMovement.h"

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
    void SetNetworkInputEnabled(bool enabled) noexcept;

    [[nodiscard]] float GetMoveSpeedMps() const noexcept;
    [[nodiscard]] float GetJumpVelocityMps() const noexcept;
    [[nodiscard]] bool IsJumpEnabled() const noexcept;
    [[nodiscard]] bool IsNetworkInputEnabled() const noexcept;
    [[nodiscard]] const Kimgane::Shared::Physics::CharacterMotionInput& GetMovementInput() const noexcept;
    // 고정 물리 스텝이 실행될 때만 호출합니다. 점프 요청은 한 번 소비하고 방향은 유지합니다.
    [[nodiscard]] Kimgane::Shared::Physics::CharacterMotionInput ConsumeMovementInput() noexcept;

private:
    [[nodiscard]] DirectX::XMFLOAT3 BuildMovementDirection() const noexcept;
    void ApplyMovement(const DirectX::XMFLOAT3& direction, float deltaTimeSec) noexcept;
    void ApplyJump() noexcept;
    void FaceMovementDirection(const DirectX::XMFLOAT3& direction) noexcept;
    void FaceCameraDirection() noexcept;
    void SendMovementInputPackets(float yawRad);

    [[nodiscard]] static DirectX::XMFLOAT3 ProjectPlanar(const DirectX::XMFLOAT3& value,
                                                         const DirectX::XMFLOAT3& fallback) noexcept;

    const InputManager& mInputManager;
    NetworkManager& mNetworkManager;
    const Camera* mCamera = nullptr;
    Kimgane::Shared::Physics::CharacterMotionInput mMovementInput = {};
    float mMoveSpeedMps = 0.0F;
    float mJumpVelocityMps = 0.0F;
    bool mJumpEnabled = false;
    bool mNetworkInputEnabled = true;
    float mLastSentYawRad = 0.0F; // 서버에 마지막으로 보낸 Yaw값
};
} // namespace Kimgane::Engine
