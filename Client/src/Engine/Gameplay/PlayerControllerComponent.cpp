#include "Pch.h"

#include <iostream>

#include "PlayerControllerComponent.h"

#include "../Camera/Camera.h"
#include "../Core/GameObject.h"
#include "../Input/InputManager.h"
#include "../Math/VectorMath.h"
#include "../Physics/RigidbodyComponent.h"
#include "PlayerControllerSettings.h"

#include "../Network/NetworkManager.h"

#include <algorithm>
#include <cmath>

namespace Kimgane::Engine
{
    PlayerControllerComponent::PlayerControllerComponent(GameObject& owner, const InputManager& inputManager, NetworkManager& networkManager) noexcept
        : Component(owner), mInputManager(inputManager), mNetworkManager(networkManager),
          mMoveSpeedMps(PlayerControllerSettings::DEFAULT_MOVE_SPEED_MPS),
          mJumpVelocityMps(PlayerControllerSettings::DEFAULT_JUMP_VELOCITY_MPS),
          mJumpEnabled(PlayerControllerSettings::DEFAULT_JUMP_ENABLED)
    {
    }

void PlayerControllerComponent::Update(float deltaTimeSec)
{
    (void)deltaTimeSec;

    const DirectX::XMFLOAT3 movementDirection = BuildMovementDirection();

    float yaw = GetOwner().GetTransform().GetRotationRad().y;

    if (mInputManager.WasKeyPressed(InputKey::MoveForward))
    {
        mNetworkManager.SendMoveStart(UP,yaw);
    }

    if (mInputManager.WasKeyReleased(InputKey::MoveForward))
    {
        mNetworkManager.SendMoveStop(UP);
    }

    if (mInputManager.WasKeyPressed(InputKey::MoveBackward))
    {
        mNetworkManager.SendMoveStart(DOWN,yaw);
    }

    if (mInputManager.WasKeyReleased(InputKey::MoveBackward))
    {
        mNetworkManager.SendMoveStop(DOWN);
    }

    if (mInputManager.WasKeyPressed(InputKey::MoveRight))
    {
        mNetworkManager.SendMoveStart(RIGHT,yaw);
    }

    if (mInputManager.WasKeyReleased(InputKey::MoveRight))
    {
        mNetworkManager.SendMoveStop(RIGHT);
    }

    if (mInputManager.WasKeyPressed(InputKey::MoveLeft))
    {
        mNetworkManager.SendMoveStart(LEFT,yaw);
    }

    if (mInputManager.WasKeyReleased(InputKey::MoveLeft))
    {
        mNetworkManager.SendMoveStop(LEFT);
    }

    //ApplyMovement(movementDirection, deltaTimeSec);
    ApplyJump();
    FaceMovementDirection(movementDirection);
}

void PlayerControllerComponent::SetCamera(const Camera* camera) noexcept
{
    mCamera = camera;
}

void PlayerControllerComponent::SetMoveSpeedMps(float moveSpeedMps) noexcept
{
    mMoveSpeedMps = std::max(0.0F, moveSpeedMps);
}

void PlayerControllerComponent::SetJumpVelocityMps(float jumpVelocityMps) noexcept
{
    mJumpVelocityMps = std::max(0.0F, jumpVelocityMps);
}

void PlayerControllerComponent::SetJumpEnabled(bool jumpEnabled) noexcept
{
    mJumpEnabled = jumpEnabled;
}

float PlayerControllerComponent::GetMoveSpeedMps() const noexcept
{
    return mMoveSpeedMps;
}

float PlayerControllerComponent::GetJumpVelocityMps() const noexcept
{
    return mJumpVelocityMps;
}

bool PlayerControllerComponent::IsJumpEnabled() const noexcept
{
    return mJumpEnabled;
}

DirectX::XMFLOAT3 PlayerControllerComponent::BuildMovementDirection() const noexcept
{
    const InputState inputState = mInputManager.GetState();
    if (std::fabs(inputState.mMoveAxis.x) <= 0.000001F && std::fabs(inputState.mMoveAxis.y) <= 0.000001F)
    {
        return {0.0F, 0.0F, 0.0F};
    }

    const DirectX::XMFLOAT3 forward =
        ProjectPlanar(mCamera != nullptr ? mCamera->GetForward() : PlayerControllerSettings::DEFAULT_FALLBACK_FORWARD,
                      PlayerControllerSettings::DEFAULT_FALLBACK_FORWARD);
    const DirectX::XMFLOAT3 right =
        ProjectPlanar(mCamera != nullptr ? mCamera->GetRight() : PlayerControllerSettings::DEFAULT_FALLBACK_RIGHT,
                      PlayerControllerSettings::DEFAULT_FALLBACK_RIGHT);
    const DirectX::XMFLOAT3 forwardMove = VectorMath::Scale(forward, inputState.mMoveAxis.y);
    const DirectX::XMFLOAT3 rightMove = VectorMath::Scale(right, inputState.mMoveAxis.x);
    return VectorMath::NormalizeOrFallback(VectorMath::Add(forwardMove, rightMove), {0.0F, 0.0F, 0.0F});
}

void PlayerControllerComponent::ApplyMovement(const DirectX::XMFLOAT3& direction, float deltaTimeSec) noexcept
{
    RigidbodyComponent* rigidbody = GetOwner().GetComponent<RigidbodyComponent>();
    if (rigidbody != nullptr)
    {
        DirectX::XMFLOAT3 velocityMps = rigidbody->GetVelocityMps();
        velocityMps.x = direction.x * mMoveSpeedMps;
        velocityMps.z = direction.z * mMoveSpeedMps;
        rigidbody->SetVelocityMps(velocityMps);
        return;
    }

    if (deltaTimeSec > 0.0F)
    {
        GetOwner().GetTransform().TranslateM(VectorMath::Scale(direction, mMoveSpeedMps * deltaTimeSec));
    }
}

void PlayerControllerComponent::ApplyJump() noexcept
{
    if (!mJumpEnabled || !mInputManager.WasKeyPressed(InputKey::Jump))
    {
        return;
    }

    RigidbodyComponent* rigidbody = GetOwner().GetComponent<RigidbodyComponent>();
    if (rigidbody == nullptr || !rigidbody->IsGrounded())
    {
        return;
    }

    rigidbody->AddForce({0.0F, mJumpVelocityMps, 0.0F}, ForceMode::VelocityChange);
    rigidbody->SetGrounded(false);
}

void PlayerControllerComponent::FaceMovementDirection(const DirectX::XMFLOAT3& direction) noexcept
{
    if (std::fabs(direction.x) <= 0.000001F && std::fabs(direction.z) <= 0.000001F)
    {
        return;
    }

    DirectX::XMFLOAT3 rotationRad = GetOwner().GetTransform().GetRotationRad();
    rotationRad.y = std::atan2(direction.x, direction.z);
    GetOwner().GetTransform().SetRotationRad(rotationRad);
    
    if (std::fabs(rotationRad.y - mLastSentYawRad) > 0.001F) // Yaw값이 이전에 보낸 값과 충분히 다를 때만 서버로 전송
    {
        mNetworkManager.SendRotate(rotationRad.y); // 서버로 Yaw값 전송
        mLastSentYawRad = rotationRad.y;
    }
}

DirectX::XMFLOAT3 PlayerControllerComponent::ProjectPlanar(const DirectX::XMFLOAT3& value,
                                                           const DirectX::XMFLOAT3& fallback) noexcept
{
    return VectorMath::NormalizeOrFallback({value.x, 0.0F, value.z}, fallback);
}
} // namespace Kimgane::Engine
