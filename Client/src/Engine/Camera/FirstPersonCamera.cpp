#include "Pch.h"

#include "FirstPersonCamera.h"

#include "../Math/VectorMath.h"
#include "CameraSettings.h"

#include <algorithm>
#include <cmath>

namespace Kimgane::Engine
{
FirstPersonCamera::FirstPersonCamera()
    : mPitchRad(CameraSettings::DEFAULT_FIRST_PERSON_PITCH_RAD), mYawRad(CameraSettings::DEFAULT_FIRST_PERSON_YAW_RAD)
{
}

void FirstPersonCamera::UpdateEye(const DirectX::XMFLOAT3& targetPositionM)
{
    const DirectX::XMFLOAT3 eyeM = VectorMath::Add(targetPositionM, mEyeOffsetM);
    const DirectX::XMFLOAT3 lookDirection = {
        std::sinf(mYawRad) * std::cosf(mPitchRad),
        std::sinf(mPitchRad),
        std::cosf(mYawRad) * std::cosf(mPitchRad)};

    SetEyeAndLookAt(eyeM, VectorMath::Add(eyeM, VectorMath::NormalizeOrFallback(lookDirection, {0.0F, 0.0F, 1.0F})));
}

void FirstPersonCamera::RotatePitchRad(float pitchDeltaRad)
{
    mPitchRad = std::clamp(mPitchRad + pitchDeltaRad,
                           CameraSettings::FIRST_PERSON_MIN_PITCH_RAD,
                           CameraSettings::FIRST_PERSON_MAX_PITCH_RAD);
}

void FirstPersonCamera::RotateYawRad(float yawDeltaRad)
{
    mYawRad += yawDeltaRad;
}

bool FirstPersonCamera::IsFirstPerson() const noexcept
{
    return true;
}
} // namespace Kimgane::Engine
