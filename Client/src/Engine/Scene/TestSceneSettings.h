#pragma once

#include <DirectXMath.h>

namespace Kimgane::Engine::TestSceneSettings
{
inline constexpr float CUBE_SIZE_M = 1.5F;
inline const DirectX::XMFLOAT4 CUBE_BASE_COLOR_LINEAR = {0.18F, 0.62F, 0.92F, 1.0F};
inline const DirectX::XMFLOAT3 CUBE_START_POSITION_M = {0.0F, 0.0F, 0.0F};
inline const DirectX::XMFLOAT3 CAMERA_LOOK_AT_POSITION_M = {0.0F, 0.0F, 0.0F};
} // namespace Kimgane::Engine::TestSceneSettings
