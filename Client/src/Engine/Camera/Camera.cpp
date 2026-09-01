#include "Pch.h"

#include "Camera.h"

#include "../Math/VectorMath.h"

namespace Kimgane::Engine
{
Camera::Camera()
{
    DirectX::XMStoreFloat4x4(&mViewMatrix, DirectX::XMMatrixIdentity());
    DirectX::XMStoreFloat4x4(&mProjectionMatrix, DirectX::XMMatrixIdentity());
    RefreshViewMatrix();
}

void Camera::Update(float deltaTimeSec)
{
    (void)deltaTimeSec;
}

void Camera::SetLens(float fovYRad, float aspectRatio, float nearZM, float farZM) noexcept
{
    DirectX::XMStoreFloat4x4(&mProjectionMatrix,
                             DirectX::XMMatrixPerspectiveFovLH(fovYRad, aspectRatio, nearZM, farZM));
}

void Camera::SetOrthographic(float widthM, float heightM, float nearZM, float farZM) noexcept
{
    DirectX::XMStoreFloat4x4(&mProjectionMatrix,
                             DirectX::XMMatrixOrthographicLH(widthM, heightM, nearZM, farZM));
}

DirectX::XMMATRIX Camera::GetViewMatrix() const noexcept
{
    return DirectX::XMLoadFloat4x4(&mViewMatrix);
}

DirectX::XMMATRIX Camera::GetProjectionMatrix() const noexcept
{
    return DirectX::XMLoadFloat4x4(&mProjectionMatrix);
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
    return mEyeM;
}

const DirectX::XMFLOAT3& Camera::GetLookAtM() const noexcept
{
    return mLookAtM;
}

const DirectX::XMFLOAT3& Camera::GetRight() const noexcept
{
    return mRight;
}

const DirectX::XMFLOAT3& Camera::GetUp() const noexcept
{
    return mUp;
}

const DirectX::XMFLOAT3& Camera::GetForward() const noexcept
{
    return mForward;
}

bool Camera::IsFirstPerson() const noexcept
{
    return false;
}

void Camera::SetEyeAndLookAt(const DirectX::XMFLOAT3& eyeM, const DirectX::XMFLOAT3& lookAtM) noexcept
{
    mEyeM = eyeM;
    mLookAtM = lookAtM;
    RefreshViewMatrix();
    UpdateBasis();
}

void Camera::RefreshViewMatrix() noexcept
{
    DirectX::XMStoreFloat4x4(&mViewMatrix,
                             DirectX::XMMatrixLookAtLH(DirectX::XMLoadFloat3(&mEyeM),
                                                       DirectX::XMLoadFloat3(&mLookAtM),
                                                       DirectX::XMLoadFloat3(&mWorldUp)));
}

void Camera::UpdateBasis() noexcept
{
    mForward = VectorMath::NormalizeOrFallback(VectorMath::Subtract(mLookAtM, mEyeM), {0.0F, 0.0F, 1.0F});
    mRight = VectorMath::NormalizeOrFallback(VectorMath::Cross(mWorldUp, mForward), {1.0F, 0.0F, 0.0F});
    mUp = VectorMath::NormalizeOrFallback(VectorMath::Cross(mForward, mRight), {0.0F, 1.0F, 0.0F});
}
} // namespace Kimgane::Engine
