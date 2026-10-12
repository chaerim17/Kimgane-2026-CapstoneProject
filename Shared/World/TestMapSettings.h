#pragma once

#include "../Physics/CollisionTypes.h"

#include <cstdint>

namespace Kimgane::Shared::World::TestMapSettings
{
// Headerless little-endian R16_UNORM, rows increasing along +Z. See Assets/Terrain/lunar_craters.json.
inline constexpr wchar_t TERRAIN_RAW_PATH[] = L"Assets/Terrain/lunar_craters_r16.raw";
inline constexpr std::uint32_t TERRAIN_WIDTH = 513;
inline constexpr std::uint32_t TERRAIN_LENGTH = 513;
inline constexpr float TERRAIN_CELL_SPACING_M = 0.75F;
inline constexpr float TERRAIN_HEIGHT_SCALE_M = 48.0F;
inline constexpr Physics::Vec3 TERRAIN_POSITION_M = {0.0F, -16.0F, 0.0F};

// Current map placement supports translation; collision boxes are world-aligned.
inline constexpr wchar_t HOUSE_MODEL_PATH[] = L"Shared/Geometry/TestHouse";
inline constexpr wchar_t HOUSE_COLLISION_PATH[] = L"Shared/Geometry/TestHouse_collision.txt";
inline constexpr Physics::Vec3 HOUSE_POSITION_M = {0.0F, 4.71F, 0.0F};
inline constexpr Physics::Vec3 PLAYER_SPAWN_POSITION_M = {-5.0F, 0.0F, 0.0F};

// Individual collider IDs; player/NPC network IDs use a separate range.
inline constexpr Physics::ObjectId TERRAIN_COLLIDER_ID = 10000;
inline constexpr Physics::ObjectId FIRST_HOUSE_COLLIDER_ID = 10001;
inline constexpr Physics::ObjectId LOCAL_TEST_CUBE_COLLIDER_ID = -2;
inline constexpr Physics::ObjectId LOCAL_PLAYER_COLLIDER_ID = -3;
} // namespace Kimgane::Shared::World::TestMapSettings
