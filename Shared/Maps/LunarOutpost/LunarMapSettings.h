#pragma once
#include <cstdint>

namespace Kimgane::Shared::LunarMap
{
// Shared switch: rebuild both client and server after changing this value.
inline constexpr bool ENABLED = true;
inline constexpr wchar_t HEIGHTMAP_PATH[] = L"Shared/Maps/LunarOutpost/lunar_height.raw";
inline constexpr wchar_t COLLISION_PATH[] = L"Shared/Maps/LunarOutpost/lunar_collision.txt";
inline constexpr std::uint32_t SAMPLE_WIDTH = 281;
inline constexpr std::uint32_t SAMPLE_LENGTH = 281;
inline constexpr float CELL_SPACING_M = 1.0F;
inline constexpr float HEIGHT_SCALE_M = 64.0F;
inline constexpr float BLENDER_HEIGHT_OFFSET_M = 16.0F;
inline constexpr float SPAWN_X_M = 0.0F;
inline constexpr float SPAWN_Y_M = 17.650660F;
inline constexpr float SPAWN_Z_M = 88.0F;
inline constexpr float FAR_CLIP_M = 500.0F;
} // namespace Kimgane::Shared::LunarMap
