#include "Pch.h"

#include "CameraComponent.h"

#include "../Core/GameObject.h"
#include "../Math/VectorMath.h"

namespace Kimgane::Engine
{
CameraComponent::CameraComponent(GameObject& owner, const DirectX::XMFLOAT3& targetOffsetM) noexcept
    : Component(owner), mTargetOffsetM(targetOffsetM)
{
    Refresh();
}

void CameraComponent::Update(float deltaTimeSec)
{
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
