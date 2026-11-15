#pragma once

#include "../Physics/CollisionTypes.h"

#include <cstdint>

namespace Kimgane::Shared::World::TestMapSettings
{
// 헤더 없는 리틀엔디언 R16_UNORM 높이맵, 행은 +Z 방향으로 증가. 자세한 값은 Assets/Terrain/lunar_craters.json 참고.
inline constexpr wchar_t TERRAIN_RAW_PATH[] = L"Assets/Terrain/lunar_craters_r16.raw";
inline constexpr std::uint32_t TERRAIN_WIDTH = 513;
inline constexpr std::uint32_t TERRAIN_LENGTH = 513;
inline constexpr float TERRAIN_CELL_SPACING_M = 0.75F;
inline constexpr float TERRAIN_HEIGHT_SCALE_M = 48.0F;
inline constexpr Physics::Vec3 TERRAIN_POSITION_M = {0.0F, -16.0F, 0.0F};

// 현재 맵 배치는 이동(평행이동)만 지원하고, 충돌 박스는 월드 축에 정렬된 AABB.
inline constexpr wchar_t HOUSE_MODEL_PATH[] = L"Shared/Geometry/TestHouse";
inline constexpr wchar_t HOUSE_COLLISION_PATH[] = L"Shared/Geometry/L02_LunarKit_collision.txt";
inline constexpr Physics::Vec3 HOUSE_POSITION_M = {0.0F, 4.71F, 0.0F};
inline constexpr Physics::Vec3 PLAYER_SPAWN_POSITION_M = {-5.0F, 0.0F, 0.0F};

// NR01 정화기. POSITION은 발판 바닥 원점이고, 충돌 박스·경사로·상호작용 기준점은 이 원점 기준 로컬 좌표.
// (DirectX 왼손 좌표계, +Y 위, +Z가 경사로 입구 방향, 단위 미터)
inline constexpr wchar_t PURIFIER_COLLISION_PATH[] = L"Shared/Geometry/NR01_Purifier_collision.txt";
inline constexpr Physics::Vec3 PURIFIER_POSITION_M = {-16.0F, 4.40F, 4.0F};
// 경사로를 감싸는 AABB(전체 크기). 로컬 z=3.57(낮음)에서 z=1.13(높음)으로 올라가므로 NegativeZ.
inline constexpr Physics::Vec3 PURIFIER_RAMP_CENTER_M = {0.0F, 0.3325F, 2.35F};
inline constexpr Physics::Vec3 PURIFIER_RAMP_SIZE_M = {2.16F, 0.615F, 2.44F};
inline constexpr Physics::RampDirection PURIFIER_RAMP_DIRECTION = Physics::RampDirection::NegativeZ;
// F키 정화: 플레이어 발 위치가 기준점에서 이 반경(XZ) 안, 높이 오차(Y) 안에 있을 때만 허용.
inline constexpr Physics::Vec3 PURIFIER_INTERACT_FEET_CENTER_M = {0.0F, 0.642F, 0.035F};
inline constexpr float PURIFIER_INTERACT_RADIUS_M = 0.73F;
inline constexpr float PURIFIER_INTERACT_Y_TOLERANCE_M = 0.14F;

// 맵 콜라이더 개별 ID. 플레이어/NPC 네트워크 ID는 별도 구간을 씀.
inline constexpr Physics::ObjectId TERRAIN_COLLIDER_ID = 10000;
inline constexpr Physics::ObjectId FIRST_HOUSE_COLLIDER_ID = 10001;
// 정화기 박스들 다음에 경사로 순서로 부여. 집 콜라이더 마지막 ID보다 커야 함.
inline constexpr Physics::ObjectId FIRST_PURIFIER_COLLIDER_ID = 20000;
inline constexpr Physics::ObjectId LOCAL_TEST_CUBE_COLLIDER_ID = -2;
inline constexpr Physics::ObjectId LOCAL_PLAYER_COLLIDER_ID = -3;
} // namespace Kimgane::Shared::World::TestMapSettings
