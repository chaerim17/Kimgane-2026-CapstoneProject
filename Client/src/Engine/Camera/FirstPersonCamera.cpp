#include "Pch.h"

#include "FirstPersonCamera.h"

#include "../Math/VectorMath.h"
#include "CameraSettings.h"

#include <algorithm>
#include <cmath>

namespace Kimgane::Engine
{
FirstPersonCamera::FirstPersonCamera()
    : pitchRad_(CameraSettings::kDefaultFirstPersonPitchRad), yawRad_(CameraSettings::kDefaultFirstPersonYawRad)
{
}

void FirstPersonCamera::UpdateEye(const DirectX::XMFLOAT3& targetPositionM)
{
    const DirectX::XMFLOAT3 eyeM = VectorMath::Add(targetPositionM, eyeOffsetM_);
    const DirectX::XMFLOAT3 lookDirection = {
        std::sinf(yawRad_) * std::cosf(pitchRad_),
        std::sinf(pitchRad_),
        std::cosf(yawRad_) * std::cosf(pitchRad_)};

    SetEyeAndLookAt(eyeM, VectorMath::Add(eyeM, VectorMath::NormalizeOrFallback(lookDirection, {0.0F, 0.0F, 1.0F})));
}

void FirstPersonCamera::RotatePitchRad(float pitchDeltaRad)
{
    pitchRad_ = std::clamp(pitchRad_ + pitchDeltaRad,
                           CameraSettings::kFirstPersonMinPitchRad,
                           CameraSettings::kFirstPersonMaxPitchRad);
}

void FirstPersonCamera::RotateYawRad(float yawDeltaRad)
{
    yawRad_ += yawDeltaRad;
}

bool FirstPersonCamera::IsFirstPerson() const noexcept
{
    return true;
}
} // namespace Kimgane::Engine
