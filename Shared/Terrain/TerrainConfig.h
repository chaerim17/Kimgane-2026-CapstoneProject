#pragma once
#include <cstdint>

namespace Shared
{
    inline constexpr uint32_t TERRAIN_WIDTH = 513;
    inline constexpr uint32_t TERRAIN_LENGTH = 513;

    inline constexpr float CELL_SPACING = 0.1953125f;
    inline constexpr float HEIGHT_SCALE = 20.0f;

    inline constexpr wchar_t TERRAIN_RAW_PATH[] = L"terrain_100x100.raw";
} // namespace Shared
