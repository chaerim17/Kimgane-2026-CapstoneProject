#pragma once

#include "../World/TestMapSettings.h"

namespace TerrainConfig
{
namespace Map = Kimgane::Shared::World::TestMapSettings;
inline constexpr auto& TERRAIN_RAW_PATH = Map::TERRAIN_RAW_PATH;
inline constexpr auto TERRAIN_WIDTH = Map::TERRAIN_WIDTH;
inline constexpr auto TERRAIN_LENGTH = Map::TERRAIN_LENGTH;
inline constexpr float CELL_SPACING = Map::TERRAIN_CELL_SPACING_M;
inline constexpr float HEIGHT_SCALE = Map::TERRAIN_HEIGHT_SCALE_M;
}
