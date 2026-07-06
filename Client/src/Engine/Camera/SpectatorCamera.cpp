#include "Pch.h"

#include "SpectatorCamera.h"

#include "../Math/VectorMath.h"
#include "CameraSettings.h"

#include <algorithm>
#include <cmath>

namespace Kimgane::Engine
{
SpectatorCamera::SpectatorCamera()
    : mYawRad(CameraSettings::DEFAULT_FIRST_PERSON_YAW_RAD)
{
    UpdateEye({0.0F, 0.0F, 0.0F});
}

void SpectatorCamera::UpdateEye(const DirectX::XMFLOAT3& targetPositionM)
{
    (void)targetPositionM;

    const DirectX::XMFLOAT3 lookDirection = {
        std::sinf(mYawRad) * std::cosf(mPitchRad),
        std::sinf(mPitchRad),
        std::cosf(mYawRad) * std::cosf(mPitchRad)};

    SetEyeAndLookAt(mEyeM, VectorMath::Add(mEyeM, VectorMath::NormalizeOrFallback(lookDirection, {0.0F, 0.0F, 1.0F})));
}

void SpectatorCamera::RotatePitchRad(float pitchDeltaRad)
{
    mPitchRad = std::clamp(mPitchRad + pitchDeltaRad,
                           CameraSettings::FIRST_PERSON_MIN_PITCH_RAD,
                           CameraSettings::FIRST_PERSON_MAX_PITCH_RAD);
    UpdateEye(mEyeM);
}

void SpectatorCamera::RotateYawRad(float yawDeltaRad)
{
    mYawRad += yawDeltaRad;
    UpdateEye(mEyeM);
}

void SpectatorCamera::SetPose(const DirectX::XMFLOAT3& eyeM, const DirectX::XMFLOAT3& forward)
{
    const DirectX::XMFLOAT3 direction = VectorMath::NormalizeOrFallback(forward, {0.0F, 0.0F, 1.0F});
    mEyeM = eyeM;
    mPitchRad = std::asinf(std::clamp(direction.y, -1.0F, 1.0F));
    mYawRad = std::atan2f(direction.x, direction.z);
    UpdateEye(mEyeM);
}

void SpectatorCamera::MoveM(const DirectX::XMFLOAT3& displacementM)
{
    mEyeM = VectorMath::Add(mEyeM, displacementM);
    UpdateEye(mEyeM);
}
} // namespace Kimgane::Engine
