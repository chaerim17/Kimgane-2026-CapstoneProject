#pragma once

#include <DirectXMath.h>

namespace Kimgane::Engine::PlayerControllerSettings
{
inline constexpr float DEFAULT_MOVE_SPEED_MPS = 5.0F;
inline constexpr float DEFAULT_JUMP_VELOCITY_MPS = 8.0F;
inline constexpr bool DEFAULT_JUMP_ENABLED = true;
inline const DirectX::XMFLOAT3 DEFAULT_FALLBACK_FORWARD = {0.0F, 0.0F, 1.0F};
inline const DirectX::XMFLOAT3 DEFAULT_FALLBACK_RIGHT = {1.0F, 0.0F, 0.0F};
} // namespace Kimgane::Engine::PlayerControllerSettings
