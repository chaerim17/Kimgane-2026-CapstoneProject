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
inline constexpr unsigned int MAX_TEXTURE_DESCRIPTORS = 32;

inline constexpr float CROSSHAIR_SIZE_PX = 10.0F;
inline constexpr float CROSSHAIR_THICKNESS_PX = 2.0F;
inline const DirectX::XMFLOAT4 CROSSHAIR_COLOR = {1.0F, 1.0F, 1.0F, 1.0F};

inline constexpr float HEALTH_BAR_WIDTH_PX = 60.0F;
inline constexpr float HEALTH_BAR_HEIGHT_PX = 8.0F;
inline const DirectX::XMFLOAT4 HEALTH_BAR_BACKGROUND_COLOR = {0.12F, 0.02F, 0.02F, 0.85F};
inline const DirectX::XMFLOAT4 HEALTH_BAR_FILL_COLOR = {0.05F, 0.85F, 0.15F, 1.0F};

inline constexpr unsigned int MAX_DAMAGE_NUMBER_COUNT = 64;
inline constexpr float DAMAGE_NUMBER_LIFETIME_SEC = 1.1F;
inline constexpr float DAMAGE_NUMBER_FADE_DURATION_SEC = 0.45F;
inline constexpr float DAMAGE_NUMBER_RISE_SPEED_MPS = 1.0F;
inline constexpr float DAMAGE_NUMBER_HEIGHT_GAP_M = 0.3F;
inline constexpr float DAMAGE_NUMBER_HORIZONTAL_SPACING_PX = 14.0F;
inline constexpr unsigned int DAMAGE_NUMBER_LANE_COUNT = 3;
inline constexpr float DAMAGE_NUMBER_WIDTH_PX = 180.0F;
inline constexpr float DAMAGE_NUMBER_HEIGHT_PX = 52.0F;
inline constexpr float DAMAGE_NUMBER_FONT_SIZE_DIP = 30.0F;
inline constexpr float DAMAGE_NUMBER_SHADOW_OFFSET_PX = 2.0F;
inline const DirectX::XMFLOAT4 DAMAGE_NUMBER_COLOR = {1.0F, 0.85F, 0.25F, 1.0F};
} // namespace Kimgane::Engine::RenderSettings
