#pragma once

#include <dxgiformat.h>

#include <DirectXMath.h>

#include <array>

namespace Kimgane::Engine::RenderSettings
{
inline constexpr unsigned int FRAME_COUNT = 2;
inline constexpr DXGI_FORMAT RENDER_TARGET_FORMAT = DXGI_FORMAT_B8G8R8A8_UNORM;
inline constexpr DXGI_FORMAT DEPTH_STENCIL_FORMAT = DXGI_FORMAT_D32_FLOAT;
inline constexpr std::array<float, 4> CLEAR_COLOR = {0.05F, 0.08F, 0.12F, 1.0F};

inline constexpr float CROSSHAIR_SIZE_PX = 10.0F;
inline constexpr float CROSSHAIR_THICKNESS_PX = 2.0F;
inline const DirectX::XMFLOAT4 CROSSHAIR_COLOR = {1.0F, 1.0F, 1.0F, 1.0F};
} // namespace Kimgane::Engine::RenderSettings
