#pragma once

#include "../Physics/CollisionTypes.h"

namespace Kimgane::Shared::World::CrateDefinition
{
    // 모델 규격 확정 전 임시 기본값으로 1m 정육면체를 사용해 충돌 박스를 정의
    inline constexpr Physics::Vec3 SIZE_M = {1.0F, 1.0F, 1.0F};
    inline constexpr Physics::Vec3 HALF_EXTENTS_M = {SIZE_M.x * 0.5F, SIZE_M.y * 0.5F, SIZE_M.z * 0.5F};

    // 상자 위치는 바닥 중심 / 충돌 박스의 월드 중심은 위치 + 아래 오프셋
    inline constexpr Physics::Vec3 COLLIDER_CENTER_OFFSET_M = {0.0F, HALF_EXTENTS_M.y, 0.0F};
    // 현재 Physics::Box에 따라 상자 회전은 0으로 고정
    inline constexpr float YAW_RAD = 0.0F;
} // namespace Kimgane::Shared::World::CrateDefinition
