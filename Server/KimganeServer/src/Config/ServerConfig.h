#pragma once

constexpr int BUF_SIZE = 200;
constexpr int TIMER_INTERVAL_MS = 50;

//npc hp info
constexpr int NPC_MAX_HP = 100;
constexpr int SHOOTING_DAMAGE = 10;

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
inline constexpr int COUNT = 10;
inline constexpr int MAX_ATTEMPTS_PER_BOX = 200;
inline constexpr float EDGE_MARGIN_M = 0.5F;
inline constexpr float OBSTACLE_GAP_M = 0.5F;
inline constexpr float PLAYER_SPAWN_CLEARANCE_M = 3.0F;
inline constexpr float MAX_GROUND_HEIGHT_DIFFERENCE_M = 0.2F;
inline constexpr float MIN_GROUND_NORMAL_Y = 0.9396926F; // cos(20 degrees)
}
