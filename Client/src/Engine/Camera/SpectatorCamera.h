#pragma once

#include "Camera.h"

namespace Kimgane::Engine
{
class SpectatorCamera final : public Camera
{
public:
    SpectatorCamera();

    void UpdateEye(const DirectX::XMFLOAT3& targetPositionM) override;
    void RotatePitchRad(float pitchDeltaRad) override;
    void RotateYawRad(float yawDeltaRad) override;

    void SetPose(const DirectX::XMFLOAT3& eyeM, const DirectX::XMFLOAT3& forward);
    void MoveM(const DirectX::XMFLOAT3& displacementM);

private:
    DirectX::XMFLOAT3 mEyeM = {0.0F, 1.4F, -5.0F};
    float mPitchRad = 0.0F;
    float mYawRad = 0.0F;
};
} // namespace Kimgane::Engine
