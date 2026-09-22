#pragma once

#include "CharacterMovement.h"

namespace Kimgane::Shared::Physics
{
[[nodiscard]] inline CollisionBody MakePlayerCollisionBody(ObjectId colliderId, const Vec3& footPositionM)
{
    return {colliderId, MakeCapsuleFromFootPosition(footPositionM,
        Settings::PLAYER_CAPSULE_RADIUS_M, Settings::PLAYER_CAPSULE_HEIGHT_M),
        CollisionLayer::PLAYER, CollisionLayer::ALL, false};
}

namespace Detail
{
// One query implementation for both hosts: registered terrain and obstacles,
// identical filtering, contact order and candidate capsule.
class WorldCharacterContactQuery final : public CharacterContactQuery
{
public:
    WorldCharacterContactQuery(const CollisionWorld& world, ObjectId colliderId)
        : mWorld(world), mColliderId(colliderId) {}

    void QueryContacts(const Vec3& positionM, std::vector<ContactInfo>& contacts) override
    {
        contacts = mWorld.QueryCharacterContacts(MakePlayerCollisionBody(mColliderId, positionM));
    }

private:
    const CollisionWorld& mWorld;
    ObjectId mColliderId;
};
} // namespace Detail

inline void ResolveCharacterContactsInWorld(RigidbodyState& state,
                                            ObjectId colliderId,
                                            const CollisionWorld& world)
{
    Detail::WorldCharacterContactQuery query(world, colliderId);
    ResolveCharacterContacts(state, query);
}

inline void StepCharacterMovementInWorld(RigidbodyState& state,
                                         const CharacterMotionInput& input,
                                         float deltaTimeSec,
                                         ObjectId colliderId,
                                         const CollisionWorld& world)
{
    Detail::WorldCharacterContactQuery query(world, colliderId);
    StepCharacterMovement(state, input, deltaTimeSec, query);
}
} // namespace Kimgane::Shared::Physics
