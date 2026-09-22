#pragma once

#include <DirectXMath.h>
#include "../../Shared/Physics/PhysicsSettings.h"

namespace Kimgane::Engine::PlayerControllerSettings
{
inline constexpr float DEFAULT_MOVE_SPEED_MPS = Kimgane::Shared::Physics::Settings::PLAYER_MOVE_SPEED_MPS;
inline constexpr float DEFAULT_JUMP_VELOCITY_MPS = Kimgane::Shared::Physics::Settings::PLAYER_JUMP_VELOCITY_MPS;
inline constexpr bool DEFAULT_JUMP_ENABLED = true;
inline const DirectX::XMFLOAT3 DEFAULT_FALLBACK_FORWARD = {0.0F, 0.0F, 1.0F};
inline const DirectX::XMFLOAT3 DEFAULT_FALLBACK_RIGHT = {1.0F, 0.0F, 0.0F};
} // namespace Kimgane::Engine::PlayerControllerSettings
