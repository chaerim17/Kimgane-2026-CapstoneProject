#pragma once

#include "../../../../Shared/World/ObjectTypes.h"
#include "../../../../Shared/Physics/CollisionTypes.h"

// 서버 월드의 공통 개체
class GameObject
{
public:
    using ObjectType = Kimgane::Shared::World::ObjectType;
    using Vec3 = Kimgane::Shared::Physics::Vec3;

    virtual ~GameObject() = default;
    [[nodiscard]] int GetId() const noexcept { return mId; }
    [[nodiscard]] ObjectType GetType() const noexcept { return mType; }
    [[nodiscard]] virtual const Vec3& GetPositionM() const noexcept = 0;
    [[nodiscard]] virtual float GetYawRad() const noexcept = 0;

protected:
    GameObject(int id, ObjectType type) : mId(id), mType(type) {}

private:
    const int mId;
    const ObjectType mType;
};
