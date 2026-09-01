#pragma once

#include "../../Shared/Physics/PhysicsSettings.h"

#include <DirectXMath.h>

namespace Kimgane::Engine::PhysicsSettings
{
namespace SharedSettings = Kimgane::Shared::Physics::Settings;

inline constexpr DirectX::XMFLOAT3 GRAVITY_MPS2 = {0.0F, SharedSettings::GRAVITY_Y_MPS2, 0.0F};
inline constexpr float MIN_MASS_KG = SharedSettings::MIN_MASS_KG;
inline constexpr float DEFAULT_MASS_KG = SharedSettings::DEFAULT_MASS_KG;
inline constexpr float DEFAULT_DRAG_PER_SEC = SharedSettings::DEFAULT_DRAG_PER_SEC;
inline constexpr float DEFAULT_GROUND_FRICTION_PER_SEC = SharedSettings::DEFAULT_GROUND_FRICTION_PER_SEC;
inline constexpr float DEFAULT_RESTITUTION = SharedSettings::DEFAULT_RESTITUTION;
inline constexpr float DEFAULT_WALKABLE_SLOPE_RAD = SharedSettings::DEFAULT_WALKABLE_SLOPE_RAD;
} // namespace Kimgane::Engine::PhysicsSettings
