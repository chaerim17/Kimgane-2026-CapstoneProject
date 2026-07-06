#include "Pch.h"

#include "ThirdPersonCamera.h"

#include "../Math/VectorMath.h"
#include "CameraSettings.h"

#include <algorithm>
#include <cmath>

namespace Kimgane::Engine
{
ThirdPersonCamera::ThirdPersonCamera()
    : mRadiusM(CameraSettings::DEFAULT_THIRD_PERSON_RADIUS_M),
      mPitchRad(CameraSettings::DEFAULT_THIRD_PERSON_PITCH_RAD),
      mYawRad(CameraSettings::DEFAULT_THIRD_PERSON_YAW_RAD)
{
}

void ThirdPersonCamera::UpdateEye(const DirectX::XMFLOAT3& targetPositionM)
{
    const DirectX::XMFLOAT3 offsetM = {mRadiusM * std::sinf(mPitchRad) * std::cosf(mYawRad),
                                      mRadiusM * std::cosf(mPitchRad),
                                      mRadiusM * std::sinf(mPitchRad) * std::sinf(mYawRad)};
    SetEyeAndLookAt(VectorMath::Add(targetPositionM, offsetM), targetPositionM);
}

void ThirdPersonCamera::RotatePitchRad(float pitchDeltaRad)
{
    mPitchRad = std::clamp(mPitchRad + pitchDeltaRad,
                           CameraSettings::THIRD_PERSON_MIN_PITCH_RAD,
                           CameraSettings::THIRD_PERSON_MAX_PITCH_RAD);
}

void ThirdPersonCamera::RotateYawRad(float yawDeltaRad)
{
    mYawRad += yawDeltaRad;
}

void ThirdPersonCamera::SetRadiusM(float radiusM) noexcept
{
    mRadiusM = std::max(0.1F, radiusM);
}
} // namespace Kimgane::Engine
