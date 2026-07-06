#pragma once

#include "Camera.h"
#include "CameraSettings.h"

namespace Kimgane::Engine
{
class FirstPersonCamera final : public Camera
{
public:
    FirstPersonCamera();

    void UpdateEye(const DirectX::XMFLOAT3& targetPositionM) override;
    void RotatePitchRad(float pitchDeltaRad) override;
    void RotateYawRad(float yawDeltaRad) override;

    [[nodiscard]] bool IsFirstPerson() const noexcept override;

private:
    float pitchRad_ = 0.0F;
    float yawRad_ = 0.0F;
    DirectX::XMFLOAT3 eyeOffsetM_ = {0.0F, CameraSettings::kFirstPersonEyeHeightM, 0.0F};
};
} // namespace Kimgane::Engine
