#pragma once

namespace Kimgane::Engine::CameraSettings
{
inline constexpr float PI = 3.14159265358979323846F;

constexpr float DegreesToRadians(float degrees) noexcept
{
    return degrees * PI / 180.0F;
}

inline constexpr float DEFAULT_FOV_Y_RAD = DegreesToRadians(60.0F);
inline constexpr float DEFAULT_NEAR_CLIP_M = 0.1F;
inline constexpr float DEFAULT_FAR_CLIP_M = 100.0F;

inline constexpr float FIRST_PERSON_EYE_HEIGHT_M = 1.6F;
inline constexpr float DEFAULT_FIRST_PERSON_YAW_RAD = 0.0F;
inline constexpr float DEFAULT_FIRST_PERSON_PITCH_RAD = 0.0F;
inline constexpr float FIRST_PERSON_MIN_PITCH_RAD = DegreesToRadians(-85.0F);
inline constexpr float FIRST_PERSON_MAX_PITCH_RAD = DegreesToRadians(85.0F);

inline constexpr float DEFAULT_THIRD_PERSON_RADIUS_M = 5.2F;
inline constexpr float DEFAULT_THIRD_PERSON_PITCH_RAD = DegreesToRadians(74.0F);
inline constexpr float DEFAULT_THIRD_PERSON_YAW_RAD = DegreesToRadians(-90.0F);
inline constexpr float THIRD_PERSON_MIN_PITCH_RAD = DegreesToRadians(20.0F);
inline constexpr float THIRD_PERSON_MAX_PITCH_RAD = DegreesToRadians(88.0F);

inline constexpr float SPRING_ARM_MIN_LENGTH_M = 0.4F;
inline constexpr float SPRING_ARM_MAX_LENGTH_M = 15.0F;
inline constexpr float SPRING_ARM_LERP_SPEED = 10.0F;
inline constexpr float SPRING_ARM_COLLISION_MARGIN_M = 0.2F;
inline constexpr float MOUSE_ORBIT_SENSITIVITY_RAD_PER_PX = DegreesToRadians(0.15F);
} // namespace Kimgane::Engine::CameraSettings
