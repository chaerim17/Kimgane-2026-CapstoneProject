#pragma once
#include <cstdint>

namespace TerrainConfig
{
    // Todo : 서버용 ResolveAssetPath 구현 필요
    // Heightmap 파일
    inline constexpr wchar_t TERRAIN_RAW_PATH[] = L"terrain_100x100.raw";

    // Heightmap 크기
    inline constexpr uint32_t TERRAIN_WIDTH = 513;
    inline constexpr uint32_t TERRAIN_LENGTH = 513;

    // 월드 변환 정보
    inline constexpr float CELL_SPACING = 0.1953125f;
    inline constexpr float HEIGHT_SCALE = 20.0f;

}
