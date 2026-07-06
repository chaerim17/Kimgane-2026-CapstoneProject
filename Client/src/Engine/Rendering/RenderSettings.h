#pragma once

#include <dxgiformat.h>

#include <array>

namespace Kimgane::Engine::RenderSettings
{
inline constexpr unsigned int kFrameCount = 2;
inline constexpr DXGI_FORMAT kRenderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
inline constexpr DXGI_FORMAT kDepthStencilFormat = DXGI_FORMAT_D32_FLOAT;
inline constexpr std::array<float, 4> kClearColor = {0.05F, 0.08F, 0.12F, 1.0F};
} // namespace Kimgane::Engine::RenderSettings
