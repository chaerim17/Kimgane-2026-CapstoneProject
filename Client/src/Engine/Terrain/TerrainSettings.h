#pragma once

#include <cstdint>

namespace Kimgane::Engine::TerrainSettings
{
inline constexpr std::uint32_t DEFAULT_SAMPLE_WIDTH = 65;
inline constexpr std::uint32_t DEFAULT_SAMPLE_LENGTH = 65;
inline constexpr float DEFAULT_CELL_SPACING_M = 0.3125F;
inline constexpr float DEFAULT_HEIGHT_SCALE_M = 1.2F;
inline constexpr float DEFAULT_WAVE_AMPLITUDE_M = 0.55F;
inline constexpr float DEFAULT_WAVE_FREQUENCY = 2.0F;
} // namespace Kimgane::Engine::TerrainSettings
