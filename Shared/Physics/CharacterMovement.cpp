#include "CharacterMovement.h"

#include <algorithm>
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
    //-----------------------------------------------------------------------------------------------------
    const bool moved = input.moveUp || input.moveDown || input.moveRight || input.moveLeft;

    float moveX = 0.0F;
    float moveZ = 0.0F;

    if (moved)
    {
        moveX = std::sin(input.yawRad);
        moveZ = std::cos(input.yawRad);
    }
    //-----------------------------------------------------------------------------------------------------
    
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

    // 집 계단의 단차(약 0.44~0.53m)를 허용 벽은 불허용
    constexpr float MAX_STEP_HEIGHT_M = 0.55F;
    constexpr float MAX_GROUND_BOX_DISTANCE_M = 1.0F;
    const Vec3 startPositionM = state.positionM;
    const auto findGroundHeight = [&](const Vec3& positionM)
    {
        float heightM = terrainHeightM;
        for (const Box& box : groundBoxes)
        {
            const float topYM = box.centerM.y + box.halfExtentsM.y;
            const float riseM = topYM - startPositionM.y;
            if (riseM > (state.isJumping ? 0.0F : MAX_STEP_HEIGHT_M) ||
                riseM < -MAX_GROUND_BOX_DISTANCE_M)
                continue;

            // 발 중심이 들어가기 전에 캡슐 옆면이 계단에 닿지 않도록 반지름 검사
            const float dx = std::max(0.0F, std::fabs(positionM.x - box.centerM.x) - box.halfExtentsM.x);
            const float dz = std::max(0.0F, std::fabs(positionM.z - box.centerM.z) - box.halfExtentsM.z);
            const float supportRadiusM = Settings::PLAYER_CAPSULE_RADIUS_M;
            if (dx * dx + dz * dz <= supportRadiusM * supportRadiusM)
                heightM = std::max(heightM, topYM);
        }
        return heightM;
    };
    const auto isBlocked = [&](const Vec3& positionM)
    {
        const CollisionBody body{objectId,
            MakeCapsuleFromFootPosition(positionM, Settings::PLAYER_CAPSULE_RADIUS_M,
                                       Settings::PLAYER_CAPSULE_HEIGHT_M),
            CollisionLayer::PLAYER, CollisionLayer::ALL, false};
        return collisionWorld.HasBlockingContact(body, objectId);
    };
    const auto tryMove = [&](Vec3 candidateM)
    {
        if (!state.isJumping)
            candidateM.y = findGroundHeight(candidateM);
        if (candidateM.y > startPositionM.y)
        {
            // 계단 위에 설 공간뿐 아니라 현재 위치에서 몸을 올릴 공간도 필요해서 두 번 검사
            Vec3 raisedPositionM = state.positionM;
            raisedPositionM.y = candidateM.y;
            if (isBlocked(raisedPositionM))
                return false;
        }
        if (isBlocked(candidateM))
            return false;
        state.positionM = candidateM;
        return true;
    };

    if (!tryMove(nextPositionM))
    {
        // 대각선 전체를 취소하지 않고 막히지 않은 축의 이동은 유지
        if (nextPositionM.x != startPositionM.x)
            tryMove({nextPositionM.x, startPositionM.y, startPositionM.z});
        if (nextPositionM.z != startPositionM.z)
            tryMove({state.positionM.x, startPositionM.y, nextPositionM.z});
    }
    // 실패한 후보 위치의 바닥 높이를 적용하지 않음
    float groundHeightM = findGroundHeight(state.positionM);
    if (!state.isJumping)
    {
        Vec3 groundedPositionM = state.positionM;
        groundedPositionM.y = groundHeightM;
        if (!isBlocked(groundedPositionM))
            state.positionM = groundedPositionM;
        else
            groundHeightM = state.positionM.y;
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
