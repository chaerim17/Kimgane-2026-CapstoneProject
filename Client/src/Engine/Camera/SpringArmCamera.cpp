#include "Pch.h"

#include "SpringArmCamera.h"

#include "../Math/VectorMath.h"
#include "CameraSettings.h"

#include <algorithm>
#include <cmath>

namespace Kimgane::Engine
{
SpringArmCamera::SpringArmCamera()
    : mCurrentArmLengthM(CameraSettings::DEFAULT_THIRD_PERSON_RADIUS_M),
      mTargetArmLengthM(CameraSettings::DEFAULT_THIRD_PERSON_RADIUS_M),
      mPitchRad(CameraSettings::DEFAULT_THIRD_PERSON_PITCH_RAD),
      mYawRad(CameraSettings::DEFAULT_THIRD_PERSON_YAW_RAD)
{
}

void SpringArmCamera::Update(float deltaTimeSec)
{
    mCurrentArmLengthM +=
        (mTargetArmLengthM - mCurrentArmLengthM) * CameraSettings::SPRING_ARM_LERP_SPEED * deltaTimeSec;
}

void SpringArmCamera::UpdateEye(const DirectX::XMFLOAT3& targetPositionM)
{
    const DirectX::XMFLOAT3 lookAtM = VectorMath::Add(targetPositionM, mLookAtOffsetM);
    const DirectX::XMFLOAT3 offsetM = BuildArmOffsetM(mCurrentArmLengthM);
    SetEyeAndLookAt(VectorMath::Add(lookAtM, offsetM), lookAtM);
}

void SpringArmCamera::RotatePitchRad(float pitchDeltaRad)
{
    mPitchRad = std::clamp(mPitchRad + pitchDeltaRad,
                           CameraSettings::THIRD_PERSON_MIN_PITCH_RAD,
                           CameraSettings::THIRD_PERSON_MAX_PITCH_RAD);
}

void SpringArmCamera::RotateYawRad(float yawDeltaRad)
{
    mYawRad += yawDeltaRad;
}

void SpringArmCamera::SetArmLengthM(float armLengthM) noexcept
{
    mTargetArmLengthM = std::clamp(armLengthM,
                                   CameraSettings::SPRING_ARM_MIN_LENGTH_M,
                                   CameraSettings::SPRING_ARM_MAX_LENGTH_M);
}

void SpringArmCamera::AddArmLengthM(float armLengthM) noexcept
{
    SetArmLengthM(mTargetArmLengthM + armLengthM);
}

void SpringArmCamera::SetCollisionDistanceM(float distanceM)
{
    const float adjustedDistanceM = std::max(0.0F, distanceM - CameraSettings::SPRING_ARM_COLLISION_MARGIN_M);
    SetEyeAndLookAt(VectorMath::Add(GetLookAtM(), BuildArmOffsetM(adjustedDistanceM)), GetLookAtM());
}

float SpringArmCamera::GetTargetArmLengthM() const noexcept
{
    return mTargetArmLengthM;
}

DirectX::XMFLOAT3 SpringArmCamera::GetDirectionToCamera() const noexcept
{
    return VectorMath::NormalizeOrFallback(BuildArmOffsetM(1.0F), {0.0F, 0.0F, -1.0F});
}

DirectX::XMFLOAT3 SpringArmCamera::BuildArmOffsetM(float armLengthM) const noexcept
{
    return {armLengthM * std::sinf(mPitchRad) * std::cosf(mYawRad),
            armLengthM * std::cosf(mPitchRad),
            armLengthM * std::sinf(mPitchRad) * std::sinf(mYawRad)};
}
} // namespace Kimgane::Engine
