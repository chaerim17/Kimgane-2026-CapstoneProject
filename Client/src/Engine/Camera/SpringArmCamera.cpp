#include "Pch.h"

#include "SpringArmCamera.h"

#include "../Math/VectorMath.h"
#include "CameraSettings.h"

#include <algorithm>
#include <cmath>

namespace Kimgane::Engine
{
SpringArmCamera::SpringArmCamera()
    : currentArmLengthM_(CameraSettings::kDefaultThirdPersonRadiusM),
      targetArmLengthM_(CameraSettings::kDefaultThirdPersonRadiusM),
      pitchRad_(CameraSettings::kDefaultThirdPersonPitchRad),
      yawRad_(CameraSettings::kDefaultThirdPersonYawRad)
{
}

void SpringArmCamera::Update(float deltaTimeSec)
{
    currentArmLengthM_ +=
        (targetArmLengthM_ - currentArmLengthM_) * CameraSettings::kSpringArmLerpSpeed * deltaTimeSec;
}

void SpringArmCamera::UpdateEye(const DirectX::XMFLOAT3& targetPositionM)
{
    const DirectX::XMFLOAT3 lookAtM = VectorMath::Add(targetPositionM, lookAtOffsetM_);
    const DirectX::XMFLOAT3 offsetM = BuildArmOffsetM(currentArmLengthM_);
    SetEyeAndLookAt(VectorMath::Add(lookAtM, offsetM), lookAtM);
}

void SpringArmCamera::RotatePitchRad(float pitchDeltaRad)
{
    pitchRad_ = std::clamp(pitchRad_ + pitchDeltaRad,
                           CameraSettings::kThirdPersonMinPitchRad,
                           CameraSettings::kThirdPersonMaxPitchRad);
}

void SpringArmCamera::RotateYawRad(float yawDeltaRad)
{
    yawRad_ += yawDeltaRad;
}

void SpringArmCamera::SetArmLengthM(float armLengthM) noexcept
{
    targetArmLengthM_ = std::clamp(armLengthM,
                                   CameraSettings::kSpringArmMinLengthM,
                                   CameraSettings::kSpringArmMaxLengthM);
}

void SpringArmCamera::AddArmLengthM(float armLengthM) noexcept
{
    SetArmLengthM(targetArmLengthM_ + armLengthM);
}

void SpringArmCamera::SetCollisionDistanceM(float distanceM)
{
    const float adjustedDistanceM = std::max(0.0F, distanceM - CameraSettings::kSpringArmCollisionMarginM);
    SetEyeAndLookAt(VectorMath::Add(GetLookAtM(), BuildArmOffsetM(adjustedDistanceM)), GetLookAtM());
}

float SpringArmCamera::GetTargetArmLengthM() const noexcept
{
    return targetArmLengthM_;
}

DirectX::XMFLOAT3 SpringArmCamera::GetDirectionToCamera() const noexcept
{
    return VectorMath::NormalizeOrFallback(BuildArmOffsetM(1.0F), {0.0F, 0.0F, -1.0F});
}

DirectX::XMFLOAT3 SpringArmCamera::BuildArmOffsetM(float armLengthM) const noexcept
{
    return {armLengthM * std::sinf(pitchRad_) * std::cosf(yawRad_),
            armLengthM * std::cosf(pitchRad_),
            armLengthM * std::sinf(pitchRad_) * std::sinf(yawRad_)};
}
} // namespace Kimgane::Engine
