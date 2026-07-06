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
    float mPitchRad = 0.0F;
    float mYawRad = 0.0F;
    DirectX::XMFLOAT3 mEyeOffsetM = {0.0F, CameraSettings::FIRST_PERSON_EYE_HEIGHT_M, 0.0F};
};
} // namespace Kimgane::Engine
