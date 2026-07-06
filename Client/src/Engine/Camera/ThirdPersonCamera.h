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
    float radiusM_ = 0.0F;
    float pitchRad_ = 0.0F;
    float yawRad_ = 0.0F;
};
} // namespace Kimgane::Engine
