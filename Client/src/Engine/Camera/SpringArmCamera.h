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

    float currentArmLengthM_ = 0.0F;
    float targetArmLengthM_ = 0.0F;
    float pitchRad_ = 0.0F;
    float yawRad_ = 0.0F;
    DirectX::XMFLOAT3 lookAtOffsetM_ = {0.0F, 0.0F, 0.0F};
};
} // namespace Kimgane::Engine
