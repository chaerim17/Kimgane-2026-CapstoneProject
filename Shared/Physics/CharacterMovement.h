// 현재 서버의 이동 로직을 분리해낸 헤더
#pragma once

#include "CollisionWorld.h"

#include <span>

namespace Kimgane::Shared::Physics
{
// IsJumping은 점프 처리 활성 여부
struct CharacterMovementState
{
    Vec3 positionM = {};
    float velocityYMps = 0.0F;
    bool isJumping = false;
};

struct CharacterMovementInput
{
    float yawRad = 0.0F;
    bool moveUp = false;
    bool moveDown = false;
    bool moveRight = false;
    bool moveLeft = false;
};

// 이동/충돌을 계산해 state의 위치를 갱신
// 반환값은 점프/착지 계산에 사용할 바닥 높이
[[nodiscard]] float StepCharacterHorizontalMovement(
    CharacterMovementState& state,
    const CharacterMovementInput& input,
    ObjectId objectId,
    const CollisionWorld& collisionWorld,
    float terrainHeightM,   // 현재 위치의 지형 높이
    std::span<const Box> groundBoxes,   // 바닥 높이 계산에 사용할 박스 목록
    float moveSpeedMps,
    float deltaTimeSec);

// 점프중인 캐릭터의 위치 갱신 + 착지 처리
// gravityMps2는 아래 방향 가속도
void StepCharacterVerticalMovement(CharacterMovementState& state,
                                   float groundHeightM,
                                   float gravityMps2,
                                   float deltaTimeSec) noexcept;
} // namespace Kimgane::Shared::Physics
