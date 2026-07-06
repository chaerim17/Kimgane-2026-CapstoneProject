#pragma once

#include <DirectXMath.h>

namespace Kimgane::Engine::TestSceneSettings
{
inline constexpr float kCubeSizeM = 1.5F;
inline const DirectX::XMFLOAT4 kCubeBaseColorLinear = {0.18F, 0.62F, 0.92F, 1.0F};
inline const DirectX::XMFLOAT3 kCubeStartPositionM = {0.0F, 0.0F, 0.0F};
inline const DirectX::XMFLOAT3 kCameraLookAtPositionM = {0.0F, 0.0F, 0.0F};
} // namespace Kimgane::Engine::TestSceneSettings
