#pragma once

namespace Kimgane::Engine::CameraSettings
{
inline constexpr float kPi = 3.14159265358979323846F;

constexpr float DegreesToRadians(float degrees) noexcept
{
    return degrees * kPi / 180.0F;
}

inline constexpr float kDefaultFovYRad = DegreesToRadians(60.0F);
inline constexpr float kDefaultNearClipM = 0.1F;
inline constexpr float kDefaultFarClipM = 100.0F;

inline constexpr float kFirstPersonEyeHeightM = 1.6F;
inline constexpr float kDefaultFirstPersonYawRad = 0.0F;
inline constexpr float kDefaultFirstPersonPitchRad = 0.0F;
inline constexpr float kFirstPersonMinPitchRad = DegreesToRadians(-85.0F);
inline constexpr float kFirstPersonMaxPitchRad = DegreesToRadians(85.0F);

inline constexpr float kDefaultThirdPersonRadiusM = 5.2F;
inline constexpr float kDefaultThirdPersonPitchRad = DegreesToRadians(74.0F);
inline constexpr float kDefaultThirdPersonYawRad = DegreesToRadians(-90.0F);
inline constexpr float kThirdPersonMinPitchRad = DegreesToRadians(20.0F);
inline constexpr float kThirdPersonMaxPitchRad = DegreesToRadians(88.0F);

inline constexpr float kSpringArmMinLengthM = 0.4F;
inline constexpr float kSpringArmMaxLengthM = 15.0F;
inline constexpr float kSpringArmLerpSpeed = 10.0F;
inline constexpr float kSpringArmCollisionMarginM = 0.2F;
} // namespace Kimgane::Engine::CameraSettings
