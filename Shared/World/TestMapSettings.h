#pragma once

#include "../Physics/CollisionTypes.h"

#include <cstdint>

namespace Kimgane::Shared::World::TestMapSettings
{
inline constexpr wchar_t TERRAIN_RAW_PATH[] = L"Shared/Terrain/terrain_100x100.raw";
inline constexpr std::uint32_t TERRAIN_WIDTH = 513;
inline constexpr std::uint32_t TERRAIN_LENGTH = 513;
inline constexpr float TERRAIN_CELL_SPACING_M = 0.1953125F;
inline constexpr float TERRAIN_HEIGHT_SCALE_M = 20.0F;
inline constexpr Physics::Vec3 TERRAIN_POSITION_M = {};

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
