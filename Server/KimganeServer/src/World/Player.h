#pragma once

#include "GameObject.h"
#include "../../../../Shared/Physics/CharacterMovement.h"
#include "../../../../Shared/World/TestMapSettings.h"
#include "../../../../Shared/Protocol.h"

class Player final : public GameObject
{
public:
    explicit Player(int id) : GameObject(id, ObjectType::Player),
        mMovementState(Kimgane::Shared::Physics::MakePlayerMovementState(
            Kimgane::Shared::World::TestMapSettings::PLAYER_SPAWN_POSITION_M)) {}

    // 위치 원본 보관
    [[nodiscard]] const Vec3& GetPositionM() const noexcept override { return mMovementState.positionM; }
    [[nodiscard]] float GetYawRad() const noexcept override { return mYaw; }

    char mUserName[MAX_NAME_LEN] = {};
    float mYaw = 0.0F;
    float mMoveYaw = 0.0F;
    bool mMoveUp = false;
    bool mMoveDown = false;
    bool mMoveLeft = false;
    bool mMoveRight = false;
    bool mJumpRequested = false;
    Kimgane::Shared::Physics::RigidbodyState mMovementState;
};
