#pragma once

// Client와 Server가 같은 충돌 크기/경사 기준을 쓰기 위한 기본값입니다.
// 서버에서도 include해야 하므로 엔진/DirectX 의존성을 두지 않습니다.
namespace Kimgane::Shared::Physics::Settings
{
// Client와 Server가 같은 중력/강체 계산 기본값을 쓰기 위한 값입니다.
inline constexpr float GRAVITY_Y_MPS2 = -9.81F;
inline constexpr float MIN_MASS_KG = 0.001F;
inline constexpr float DEFAULT_MASS_KG = 1.0F;
inline constexpr float DEFAULT_DRAG_PER_SEC = 0.0F;
inline constexpr float DEFAULT_GROUND_FRICTION_PER_SEC = 6.0F;
inline constexpr float DEFAULT_RESTITUTION = 0.0F;
inline constexpr float DEFAULT_GRAVITY_SCALE = 1.0F;
inline constexpr float RESTING_HORIZONTAL_SPEED_SQ = 0.0001F;

// 0 크기 collider는 거리/normal 계산을 불안정하게 만들어 최소값으로 보정합니다.
inline constexpr float MIN_COLLIDER_SIZE_M = 0.001F;

// 너무 짧은 방향 벡터는 normal로 쓰기 어려워 world up으로 대체합니다.
inline constexpr float MIN_DIRECTION_LENGTH_SQ = 0.000001F;

// 45도 경사 기준입니다. 더 가파른 지형은 접촉하더라도 walkable=false로 표시합니다.
inline constexpr float DEFAULT_WALKABLE_SLOPE_RAD = 0.78539816339F;

// 현재 클라 테스트 플레이어와 맞춘 기본 capsule 크기입니다.
inline constexpr float PLAYER_CAPSULE_RADIUS_M = 0.45F;
inline constexpr float PLAYER_CAPSULE_HEIGHT_M = 1.8F;
inline constexpr float NPC_CAPSULE_RADIUS_M = 0.45F;
inline constexpr float NPC_CAPSULE_HEIGHT_M = 1.8F;
} // namespace Kimgane::Shared::Physics::Settings
