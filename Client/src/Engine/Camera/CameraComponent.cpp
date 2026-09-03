#include "Pch.h"

#include "CameraComponent.h"

#include "../Core/GameObject.h"
#include "../Input/InputManager.h"
#include "../Math/VectorMath.h"
#include "CameraSettings.h"

namespace Kimgane::Engine
{
    CameraComponent::CameraComponent(GameObject& owner, const InputManager& inputManager, const DirectX::XMFLOAT3& targetOffsetM) noexcept
        : Component(owner), mInputManager(inputManager), mTargetOffsetM(targetOffsetM)
{
    Refresh();
}

void CameraComponent::Update(float deltaTimeSec)
{
    const InputState inputState = mInputManager.GetState();
    mCamera.RotateYawRad(-inputState.mMouseDeltaPx.x * CameraSettings::MOUSE_ORBIT_SENSITIVITY_RAD_PER_PX);
    mCamera.RotatePitchRad(-inputState.mMouseDeltaPx.y * CameraSettings::MOUSE_ORBIT_SENSITIVITY_RAD_PER_PX);

    mCamera.Update(deltaTimeSec);
    Refresh();
}

void CameraComponent::SetLens(float fovYRad, float aspectRatio, float nearZM, float farZM) noexcept
{
    mCamera.SetLens(fovYRad, aspectRatio, nearZM, farZM);
}

void CameraComponent::SetTargetOffsetM(const DirectX::XMFLOAT3& targetOffsetM) noexcept
{
    mTargetOffsetM = targetOffsetM;
    Refresh();
}

void CameraComponent::Refresh() noexcept
{
    const DirectX::XMFLOAT3 targetPositionM =
        VectorMath::Add(GetOwner().GetTransform().GetPositionM(), mTargetOffsetM);
    mCamera.UpdateEye(targetPositionM);
}

Camera& CameraComponent::GetCamera() noexcept
{
    return mCamera;
}

const Camera& CameraComponent::GetCamera() const noexcept
{
    return mCamera;
}

SpringArmCamera& CameraComponent::GetSpringArmCamera() noexcept
{
    return mCamera;
}

const SpringArmCamera& CameraComponent::GetSpringArmCamera() const noexcept
{
    return mCamera;
}

const DirectX::XMFLOAT3& CameraComponent::GetTargetOffsetM() const noexcept
{
    return mTargetOffsetM;
}
} // namespace Kimgane::Engine
