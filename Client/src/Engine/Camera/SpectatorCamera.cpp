#include "Pch.h"

#include "SpectatorCamera.h"

#include "../Math/VectorMath.h"
#include "CameraSettings.h"

#include <algorithm>
#include <cmath>

namespace Kimgane::Engine
{
SpectatorCamera::SpectatorCamera()
    : yawRad_(CameraSettings::kDefaultFirstPersonYawRad)
{
    UpdateEye({0.0F, 0.0F, 0.0F});
}

void SpectatorCamera::UpdateEye(const DirectX::XMFLOAT3& targetPositionM)
{
    (void)targetPositionM;

    const DirectX::XMFLOAT3 lookDirection = {
        std::sinf(yawRad_) * std::cosf(pitchRad_),
        std::sinf(pitchRad_),
        std::cosf(yawRad_) * std::cosf(pitchRad_)};

    SetEyeAndLookAt(eyeM_, VectorMath::Add(eyeM_, VectorMath::NormalizeOrFallback(lookDirection, {0.0F, 0.0F, 1.0F})));
}

void SpectatorCamera::RotatePitchRad(float pitchDeltaRad)
{
    pitchRad_ = std::clamp(pitchRad_ + pitchDeltaRad,
                           CameraSettings::kFirstPersonMinPitchRad,
                           CameraSettings::kFirstPersonMaxPitchRad);
    UpdateEye(eyeM_);
}

void SpectatorCamera::RotateYawRad(float yawDeltaRad)
{
    yawRad_ += yawDeltaRad;
    UpdateEye(eyeM_);
}

void SpectatorCamera::SetPose(const DirectX::XMFLOAT3& eyeM, const DirectX::XMFLOAT3& forward)
{
    const DirectX::XMFLOAT3 direction = VectorMath::NormalizeOrFallback(forward, {0.0F, 0.0F, 1.0F});
    eyeM_ = eyeM;
    pitchRad_ = std::asinf(std::clamp(direction.y, -1.0F, 1.0F));
    yawRad_ = std::atan2f(direction.x, direction.z);
    UpdateEye(eyeM_);
}

void SpectatorCamera::MoveM(const DirectX::XMFLOAT3& displacementM)
{
    eyeM_ = VectorMath::Add(eyeM_, displacementM);
    UpdateEye(eyeM_);
}
} // namespace Kimgane::Engine
