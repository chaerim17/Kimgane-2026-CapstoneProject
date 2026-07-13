#pragma once

#include <DirectXMath.h>

namespace Kimgane::Engine::TestSceneSettings
{
inline constexpr float CUBE_SIZE_M = 1.5F;
inline const DirectX::XMFLOAT4 CUBE_BASE_COLOR_LINEAR = {0.18F, 0.62F, 0.92F, 1.0F};
inline const DirectX::XMFLOAT3 CUBE_START_POSITION_M = {0.0F, 0.0F, 0.0F};
inline const DirectX::XMFLOAT3 CAMERA_LOOK_AT_POSITION_M = {0.0F, 0.0F, 0.0F};
inline constexpr float PLAYER_CAPSULE_RADIUS_M = 0.45F;
inline constexpr float PLAYER_CAPSULE_HEIGHT_M = 1.8F;
inline const DirectX::XMFLOAT3 PLAYER_START_POSITION_M = {-2.0F, PLAYER_CAPSULE_HEIGHT_M * 0.5F, -2.0F};
inline const DirectX::XMFLOAT3 PLAYER_CAMERA_TARGET_OFFSET_M = {0.0F, 0.65F, 0.0F};
inline const DirectX::XMFLOAT4 PLAYER_BASE_COLOR_LINEAR = {0.20F, 0.82F, 0.42F, 1.0F};
inline const DirectX::XMFLOAT4 NETWORK_PLAYER_BASE_COLOR_LINEAR = {0.96F, 0.58F, 0.18F, 1.0F};
} // namespace Kimgane::Engine::TestSceneSettings
