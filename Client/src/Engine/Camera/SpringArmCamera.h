#pragma once

#include "Camera.h"

namespace Kimgane::Engine
{
class SpringArmCamera final : public Camera
{
public:
    SpringArmCamera();

    void Update(float deltaTimeSec) override;
    void UpdateEye(const DirectX::XMFLOAT3& targetPositionM) override;
    void RotatePitchRad(float pitchDeltaRad) override;
    void RotateYawRad(float yawDeltaRad) override;

    void SetArmLengthM(float armLengthM) noexcept;
    void AddArmLengthM(float armLengthM) noexcept;
    void SetCollisionDistanceM(float distanceM);

    [[nodiscard]] float GetTargetArmLengthM() const noexcept;
    [[nodiscard]] DirectX::XMFLOAT3 GetDirectionToCamera() const noexcept;

private:
    [[nodiscard]] DirectX::XMFLOAT3 BuildArmOffsetM(float armLengthM) const noexcept;

    float mCurrentArmLengthM = 0.0F;
    float mTargetArmLengthM = 0.0F;
    float mPitchRad = 0.0F;
    float mYawRad = 0.0F;
    DirectX::XMFLOAT3 mLookAtOffsetM = {0.0F, 0.0F, 0.0F};
};
} // namespace Kimgane::Engine
