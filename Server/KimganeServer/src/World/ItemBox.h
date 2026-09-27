#pragma once

#include "GameObject.h"
#include "../../../../Shared/World/CrateDefinition.h"

// 배치 후 고정 위치는 바닥 중심 기준
class ItemBox final : public GameObject
{
public:
    ItemBox(int id, const Vec3& positionM) : GameObject(id, ObjectType::ItemBox), mPositionM(positionM) {}

    [[nodiscard]] const Vec3& GetPositionM() const noexcept override { return mPositionM; }
    [[nodiscard]] float GetYawRad() const noexcept override
    {
        return Kimgane::Shared::World::CrateDefinition::YAW_RAD;
    }
    [[nodiscard]] Kimgane::Shared::Physics::Box GetCollisionBox() const noexcept
    {
        namespace Definition = Kimgane::Shared::World::CrateDefinition;
        return {Kimgane::Shared::Physics::Add(mPositionM, Definition::COLLIDER_CENTER_OFFSET_M),
                Definition::HALF_EXTENTS_M};
    }

private:
    Vec3 mPositionM;
};
