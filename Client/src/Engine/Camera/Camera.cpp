#include "Pch.h"

#include "Camera.h"

#include "../Math/VectorMath.h"

namespace Kimgane::Engine
{
Camera::Camera()
{
    DirectX::XMStoreFloat4x4(&viewMatrix_, DirectX::XMMatrixIdentity());
    DirectX::XMStoreFloat4x4(&projectionMatrix_, DirectX::XMMatrixIdentity());
    RefreshViewMatrix();
}

void Camera::Update(float deltaTimeSec)
{
    (void)deltaTimeSec;
}

void Camera::SetLens(float fovYRad, float aspectRatio, float nearZM, float farZM) noexcept
{
    DirectX::XMStoreFloat4x4(&projectionMatrix_,
                             DirectX::XMMatrixPerspectiveFovLH(fovYRad, aspectRatio, nearZM, farZM));
}

DirectX::XMMATRIX Camera::GetViewMatrix() const noexcept
{
    return DirectX::XMLoadFloat4x4(&viewMatrix_);
}

DirectX::XMMATRIX Camera::GetProjectionMatrix() const noexcept
{
    return DirectX::XMLoadFloat4x4(&projectionMatrix_);
}

DirectX::XMMATRIX Camera::GetViewProjectionMatrix() const noexcept
{
    return GetViewMatrix() * GetProjectionMatrix();
}

DirectX::XMFLOAT4X4 Camera::GetViewProjectionMatrix4x4() const noexcept
{
    DirectX::XMFLOAT4X4 viewProjectionMatrix = {};
    DirectX::XMStoreFloat4x4(&viewProjectionMatrix, GetViewProjectionMatrix());
    return viewProjectionMatrix;
}

const DirectX::XMFLOAT3& Camera::GetEyeM() const noexcept
{
    return eyeM_;
}

const DirectX::XMFLOAT3& Camera::GetLookAtM() const noexcept
{
    return lookAtM_;
}

const DirectX::XMFLOAT3& Camera::GetRight() const noexcept
{
    return right_;
}

const DirectX::XMFLOAT3& Camera::GetUp() const noexcept
{
    return up_;
}

const DirectX::XMFLOAT3& Camera::GetForward() const noexcept
{
    return forward_;
}

bool Camera::IsFirstPerson() const noexcept
{
    return false;
}

void Camera::SetEyeAndLookAt(const DirectX::XMFLOAT3& eyeM, const DirectX::XMFLOAT3& lookAtM) noexcept
{
    eyeM_ = eyeM;
    lookAtM_ = lookAtM;
    RefreshViewMatrix();
    UpdateBasis();
}

void Camera::RefreshViewMatrix() noexcept
{
    DirectX::XMStoreFloat4x4(&viewMatrix_,
                             DirectX::XMMatrixLookAtLH(DirectX::XMLoadFloat3(&eyeM_),
                                                       DirectX::XMLoadFloat3(&lookAtM_),
                                                       DirectX::XMLoadFloat3(&worldUp_)));
}

void Camera::UpdateBasis() noexcept
{
    forward_ = VectorMath::NormalizeOrFallback(VectorMath::Subtract(lookAtM_, eyeM_), {0.0F, 0.0F, 1.0F});
    right_ = VectorMath::NormalizeOrFallback(VectorMath::Cross(worldUp_, forward_), {1.0F, 0.0F, 0.0F});
    up_ = VectorMath::NormalizeOrFallback(VectorMath::Cross(forward_, right_), {0.0F, 1.0F, 0.0F});
}
} // namespace Kimgane::Engine
