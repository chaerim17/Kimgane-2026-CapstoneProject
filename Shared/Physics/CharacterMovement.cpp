#include "CharacterMovement.h"

#include <cfloat>
#include <cmath>

namespace Kimgane::Shared::Physics
{
float StepCharacterHorizontalMovement(CharacterMovementState& state,
                                      const CharacterMovementInput& input,
                                      ObjectId objectId,
                                      const CollisionWorld& collisionWorld,
                                      float terrainHeightM,
                                      std::span<const Box> groundBoxes,
                                      float moveSpeedMps,
                                      float deltaTimeSec)
{
    // 입력 yaw를 기준으로 방향 벡터 계산
    const float forwardX = std::sin(input.yawRad);
    const float forwardZ = std::cos(input.yawRad);
    const float rightX = std::cos(input.yawRad);
    const float rightZ = -std::sin(input.yawRad);
    float moveX = 0.0F;
    float moveZ = 0.0F;

    if (input.moveUp)
    {
        moveX += forwardX;
        moveZ += forwardZ;
    }
    if (input.moveDown)
    {
        moveX -= forwardX;
        moveZ -= forwardZ;
    }
    if (input.moveRight)
    {
        moveX += rightX;
        moveZ += rightZ;
    }
    if (input.moveLeft)
    {
        moveX -= rightX;
        moveZ -= rightZ;
    }

    // 대각선 이동 보정
    const float length = std::sqrt(moveX * moveX + moveZ * moveZ);
    if (length > 0.0F)
    {
        moveX /= length;
        moveZ /= length;
    }

    Vec3 nextPositionM = state.positionM;
    nextPositionM.x += moveX * moveSpeedMps * deltaTimeSec;
    nextPositionM.z += moveZ * moveSpeedMps * deltaTimeSec;

    // 충돌 체크를 위해 현재 위치에서 이동 후 위치로 캡슐 생성
    const CollisionBody playerBody{
        objectId,
        MakeCapsuleFromFootPosition(nextPositionM, Settings::PLAYER_CAPSULE_RADIUS_M,
                                   Settings::PLAYER_CAPSULE_HEIGHT_M),
        CollisionLayer::PLAYER, CollisionLayer::ALL, false};
    const bool blocked = collisionWorld.HasBlockingContact(playerBody, objectId);

    float groundHeightM = terrainHeightM;
    float bestDistanceM = FLT_MAX;
    constexpr float MAX_GROUND_BOX_DISTANCE_M = 1.0F;
    for (const Box& box : groundBoxes)
    {
        const bool insideXZ = nextPositionM.x >= box.centerM.x - box.halfExtentsM.x &&
                              nextPositionM.x <= box.centerM.x + box.halfExtentsM.x &&
                              nextPositionM.z >= box.centerM.z - box.halfExtentsM.z &&
                              nextPositionM.z <= box.centerM.z + box.halfExtentsM.z;
        const float topYM = box.centerM.y + box.halfExtentsM.y;
        const float distanceM = state.positionM.y - topYM;
        if (insideXZ && state.positionM.y >= topYM && distanceM <= MAX_GROUND_BOX_DISTANCE_M &&
            distanceM < bestDistanceM)
        {
            bestDistanceM = distanceM;
            groundHeightM = topYM;
        }
    }

    if (!blocked)
    {
        state.positionM.x = nextPositionM.x;
        state.positionM.z = nextPositionM.z;
    }
    if (!state.isJumping)
    {
        state.positionM.y = groundHeightM;
    }
    return groundHeightM;
}

//
void StepCharacterVerticalMovement(CharacterMovementState& state,
                                   float groundHeightM,
                                   float gravityMps2,
                                   float deltaTimeSec) noexcept
{
    if (state.isJumping)
    {
        state.positionM.y += state.velocityYMps * deltaTimeSec;
        state.velocityYMps -= gravityMps2 * deltaTimeSec;
        if (state.velocityYMps < 0.0F && state.positionM.y <= groundHeightM)
        {
            state.positionM.y = groundHeightM;
            state.velocityYMps = 0.0F;
            state.isJumping = false;
        }
    }
    if (!state.isJumping)
    {
        state.positionM.y = groundHeightM;
    }
}
} // namespace Kimgane::Shared::Physics
