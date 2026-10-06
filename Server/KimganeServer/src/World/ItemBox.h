#pragma once

#include "GameObject.h"

// 배치 후 고정 위치는 바닥 중심 기준
class ItemBox final : public GameObject
{
public:
    ItemBox(int id, const Vec3& positionM, const Kimgane::Shared::Physics::Box& localCollisionBox)
        : GameObject(id, ObjectType::ItemBox), mPositionM(positionM),
          mCollisionBox{Kimgane::Shared::Physics::Add(positionM, localCollisionBox.centerM),
                        localCollisionBox.halfExtentsM} {}

    [[nodiscard]] const Vec3& GetPositionM() const noexcept override { return mPositionM; }
    [[nodiscard]] float GetYawRad() const noexcept override
    {
        // 축 정렬 충돌 박스에 맞춰 회전은 0으로 고정.
        return 0.0F;
    }
    [[nodiscard]] Kimgane::Shared::Physics::Box GetCollisionBox() const noexcept
    {
        return mCollisionBox;
    }

private:
    Vec3 mPositionM;
    Kimgane::Shared::Physics::Box mCollisionBox;
};
