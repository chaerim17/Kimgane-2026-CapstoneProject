#include "Pch.h"

#include "ThirdPersonCamera.h"

#include "../Math/VectorMath.h"
#include "CameraSettings.h"

#include <algorithm>
#include <cmath>

namespace Kimgane::Engine
{
ThirdPersonCamera::ThirdPersonCamera()
    : radiusM_(CameraSettings::kDefaultThirdPersonRadiusM),
      pitchRad_(CameraSettings::kDefaultThirdPersonPitchRad),
      yawRad_(CameraSettings::kDefaultThirdPersonYawRad)
{
}

void ThirdPersonCamera::UpdateEye(const DirectX::XMFLOAT3& targetPositionM)
{
    const DirectX::XMFLOAT3 offsetM = {radiusM_ * std::sinf(pitchRad_) * std::cosf(yawRad_),
                                      radiusM_ * std::cosf(pitchRad_),
                                      radiusM_ * std::sinf(pitchRad_) * std::sinf(yawRad_)};
    SetEyeAndLookAt(VectorMath::Add(targetPositionM, offsetM), targetPositionM);
}

void ThirdPersonCamera::RotatePitchRad(float pitchDeltaRad)
{
    pitchRad_ = std::clamp(pitchRad_ + pitchDeltaRad,
                           CameraSettings::kThirdPersonMinPitchRad,
                           CameraSettings::kThirdPersonMaxPitchRad);
}

void ThirdPersonCamera::RotateYawRad(float yawDeltaRad)
{
    yawRad_ += yawDeltaRad;
}

void ThirdPersonCamera::SetRadiusM(float radiusM) noexcept
{
    radiusM_ = std::max(0.1F, radiusM);
}
} // namespace Kimgane::Engine
