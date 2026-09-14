#pragma once

#include <cstdint>
#include "../World/TestMapSettings.h"

namespace Kimgane::Engine::TerrainSettings
{
inline constexpr std::uint32_t DEFAULT_SAMPLE_WIDTH = 65;
inline constexpr std::uint32_t DEFAULT_SAMPLE_LENGTH = 65;
inline constexpr float DEFAULT_CELL_SPACING_M = 0.3125F;
inline constexpr float DEFAULT_HEIGHT_SCALE_M = 1.2F;
inline constexpr float DEFAULT_WAVE_AMPLITUDE_M = 0.55F;
inline constexpr float DEFAULT_WAVE_FREQUENCY = 2.0F;
namespace Map = Kimgane::Shared::World::TestMapSettings;
inline constexpr auto& RAW_HEIGHTMAP_PATH = Map::TERRAIN_RAW_PATH;
inline constexpr auto RAW_SAMPLE_WIDTH = Map::TERRAIN_WIDTH;
inline constexpr auto RAW_SAMPLE_LENGTH = Map::TERRAIN_LENGTH;
inline constexpr float RAW_CELL_SPACING_M = Map::TERRAIN_CELL_SPACING_M;
inline constexpr float RAW_HEIGHT_SCALE_M = Map::TERRAIN_HEIGHT_SCALE_M;
} // namespace Kimgane::Engine::TerrainSettings
