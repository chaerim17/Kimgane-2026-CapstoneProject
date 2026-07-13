#pragma once

#include <DirectXMath.h>

namespace Kimgane::Engine::PhysicsSettings
{
inline constexpr DirectX::XMFLOAT3 GRAVITY_MPS2 = {0.0F, -9.81F, 0.0F};
inline constexpr float MIN_MASS_KG = 0.001F;
inline constexpr float DEFAULT_MASS_KG = 1.0F;
inline constexpr float DEFAULT_DRAG_PER_SEC = 0.0F;
inline constexpr float DEFAULT_GROUND_FRICTION_PER_SEC = 6.0F;
inline constexpr float DEFAULT_RESTITUTION = 0.0F;
inline constexpr float DEFAULT_WALKABLE_SLOPE_RAD = 0.78539816339F;
} // namespace Kimgane::Engine::PhysicsSettings
