#pragma once

#include <DirectXMath.h>

namespace Kimgane::Engine::PhysicsSettings
{
inline constexpr DirectX::XMFLOAT3 kGravityMps2 = {0.0F, -9.81F, 0.0F};
inline constexpr float kMinMassKg = 0.001F;
inline constexpr float kDefaultMassKg = 1.0F;
inline constexpr float kDefaultDragPerSec = 0.0F;
inline constexpr float kDefaultGroundFrictionPerSec = 6.0F;
inline constexpr float kDefaultRestitution = 0.0F;
inline constexpr float kDefaultWalkableSlopeRad = 0.78539816339F;
} // namespace Kimgane::Engine::PhysicsSettings
