#pragma once

constexpr int MATCH_CAPACITY = 100;
constexpr int PLAYERS_PER_MATCH = 100;

constexpr int NPC_COUNT = 100;
constexpr int MOVE_INTERVAL_MS = 1000; // NPC 이동 간격
constexpr int TIMER_INTERVAL_MS = 50;

namespace PlayerMutationSettings
{
inline constexpr float MIN_VALUE = 0.0F;
inline constexpr float MAX_VALUE = 100.0F;
inline constexpr float INITIAL_VALUE = MIN_VALUE;
inline constexpr float GAIN_PER_SECOND = 1.0F;
inline constexpr float MIN_MOVE_SPEED_MULTIPLIER = 1.0F;
inline constexpr float MAX_MOVE_SPEED_MULTIPLIER = 3.0F;
inline constexpr int MOVE_SPEED_PARTITION = 5; // 1.0 ~ 3.0 사이를 5개 구간으로 나누어 선택 (변이 범위, 속도)
//디버깅용
// inline constexpr int DEBUG_LOG_INTERVAL_MS = 1000;
}

//npc hp info
constexpr int NPC_MAX_HP = 100;
constexpr int SHOOTING_DAMAGE = 10;
constexpr int NPC_HEADSHOT_DAMAGE_MULTIPLIER = 2;

// 플레이어 발 위치와 상자 바닥 중심 사이의 거리 3M로 정의하고 사용
constexpr float ITEM_BOX_OPEN_DISTANCE_M = 3.0F;

// 아이템 박스 모델과 충돌 경로 세팅
namespace ItemBoxAssetSettings
{
inline constexpr wchar_t BODY_MODEL_PATH[] = L"Assets/ItemBox/obj/S07_Body_LOD0";
inline constexpr wchar_t LID_MODEL_PATH[] = L"Assets/ItemBox/obj/S07_Lid_LOD0";
inline constexpr wchar_t COLLISION_PATH[] = L"Shared/Geometry/S07_Supply_Crate_collision.txt";
}

namespace ItemBoxSpawnSettings
{
inline constexpr int COUNT = 300;
inline constexpr int MAX_ATTEMPTS_PER_BOX = 200;
inline constexpr float EDGE_MARGIN_M = 0.5F;
inline constexpr float OBSTACLE_GAP_M = 0.5F;
inline constexpr float PLAYER_SPAWN_CLEARANCE_M = 3.0F;
inline constexpr float MAX_GROUND_HEIGHT_DIFFERENCE_M = 0.2F;
inline constexpr float MIN_GROUND_NORMAL_Y = 0.9396926F; // cos(20 degrees)
}
