#pragma once

#include "Camera.h"

namespace Kimgane::Engine
{
class ThirdPersonCamera final : public Camera
{
public:
    ThirdPersonCamera();

    void UpdateEye(const DirectX::XMFLOAT3& targetPositionM) override;
    void RotatePitchRad(float pitchDeltaRad) override;
    void RotateYawRad(float yawDeltaRad) override;

    void SetRadiusM(float radiusM) noexcept;

private:
    float mRadiusM = 0.0F;
    float mPitchRad = 0.0F;
    float mYawRad = 0.0F;
};
} // namespace Kimgane::Engine
