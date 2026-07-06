#pragma once

#include <dxgiformat.h>

#include <array>

namespace Kimgane::Engine::RenderSettings
{
inline constexpr unsigned int FRAME_COUNT = 2;
inline constexpr DXGI_FORMAT RENDER_TARGET_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
inline constexpr DXGI_FORMAT DEPTH_STENCIL_FORMAT = DXGI_FORMAT_D32_FLOAT;
inline constexpr std::array<float, 4> CLEAR_COLOR = {0.05F, 0.08F, 0.12F, 1.0F};
} // namespace Kimgane::Engine::RenderSettings
