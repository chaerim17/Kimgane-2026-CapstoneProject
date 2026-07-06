#pragma once

#include <DirectXMath.h>

namespace Kimgane::Engine
{
class Camera
{
public:
    Camera();
    virtual ~Camera() = default;

    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;
    Camera(Camera&&) = delete;
    Camera& operator=(Camera&&) = delete;

    virtual void Update(float deltaTimeSec);
    virtual void UpdateEye(const DirectX::XMFLOAT3& targetPositionM) = 0;
    virtual void RotatePitchRad(float pitchDeltaRad) = 0;
    virtual void RotateYawRad(float yawDeltaRad) = 0;

    void SetLens(float fovYRad, float aspectRatio, float nearZM, float farZM) noexcept;

    [[nodiscard]] DirectX::XMMATRIX GetViewMatrix() const noexcept;
    [[nodiscard]] DirectX::XMMATRIX GetProjectionMatrix() const noexcept;
    [[nodiscard]] DirectX::XMMATRIX GetViewProjectionMatrix() const noexcept;
    [[nodiscard]] DirectX::XMFLOAT4X4 GetViewProjectionMatrix4x4() const noexcept;

    [[nodiscard]] const DirectX::XMFLOAT3& GetEyeM() const noexcept;
    [[nodiscard]] const DirectX::XMFLOAT3& GetLookAtM() const noexcept;
    [[nodiscard]] const DirectX::XMFLOAT3& GetRight() const noexcept;
    [[nodiscard]] const DirectX::XMFLOAT3& GetUp() const noexcept;
    [[nodiscard]] const DirectX::XMFLOAT3& GetForward() const noexcept;
    [[nodiscard]] virtual bool IsFirstPerson() const noexcept;

protected:
    void SetEyeAndLookAt(const DirectX::XMFLOAT3& eyeM, const DirectX::XMFLOAT3& lookAtM) noexcept;
    void RefreshViewMatrix() noexcept;
    void UpdateBasis() noexcept;

private:
    DirectX::XMFLOAT4X4 mViewMatrix = {};
    DirectX::XMFLOAT4X4 mProjectionMatrix = {};

    DirectX::XMFLOAT3 mEyeM = {0.0F, 0.0F, -1.0F};
    DirectX::XMFLOAT3 mLookAtM = {0.0F, 0.0F, 0.0F};
    DirectX::XMFLOAT3 mWorldUp = {0.0F, 1.0F, 0.0F};

    DirectX::XMFLOAT3 mRight = {1.0F, 0.0F, 0.0F};
    DirectX::XMFLOAT3 mUp = {0.0F, 1.0F, 0.0F};
    DirectX::XMFLOAT3 mForward = {0.0F, 0.0F, 1.0F};
};
} // namespace Kimgane::Engine
