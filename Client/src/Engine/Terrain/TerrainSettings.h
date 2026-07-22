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
inline constexpr wchar_t RAW_HEIGHTMAP_PATH[] = L"Assets/Terrain/terrain_100x100.raw";
inline constexpr std::uint32_t RAW_SAMPLE_WIDTH = 513;
inline constexpr std::uint32_t RAW_SAMPLE_LENGTH = 513;
inline constexpr float RAW_CELL_SPACING_M = 0.1953125F;
inline constexpr float RAW_HEIGHT_SCALE_M = 20.0F; // 원하는 최대 고저차
} // namespace Kimgane::Engine::TerrainSettings
