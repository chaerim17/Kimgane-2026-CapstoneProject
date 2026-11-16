#pragma once

#include "GameObject.h"
#include "../Config/ServerConfig.h"
#include "../Items/Inventory.h"
#include "../../../../Shared/Physics/CharacterMovement.h"
#include "../../../../Shared/World/TestMapSettings.h"
#include "../../../../Shared/Protocol.h"
#include <algorithm>
#include <cmath>

class Player final : public GameObject
{
public:
    explicit Player(int id) : GameObject(id, ObjectType::Player),
        mMovementState(Kimgane::Shared::Physics::MakePlayerMovementState(
            Kimgane::Shared::World::TestMapSettings::PLAYER_SPAWN_POSITION_M)) {}

    // 위치 원본 보관
    [[nodiscard]] const Vec3& GetPositionM() const noexcept override { return mMovementState.positionM; }
    [[nodiscard]] float GetYawRad() const noexcept override { return mYaw; }

    [[nodiscard]] Inventory& GetInventory() noexcept { return mInventory; }
    [[nodiscard]] const Inventory& GetInventory() const noexcept { return mInventory; }

    [[nodiscard]] float GetMutation() const noexcept { return mMutation; }

    [[nodiscard]] float GetMoveSpeedMultiplier() const noexcept
    {
        namespace Mutation = PlayerMutationSettings;
        constexpr float mutationPerStage = (Mutation::MAX_VALUE - Mutation::MIN_VALUE) /
                                           Mutation::MOVE_SPEED_PARTITION;
        const float speedStage = std::floor((mMutation - Mutation::MIN_VALUE) / mutationPerStage);
        const float stageRatio = speedStage / Mutation::MOVE_SPEED_PARTITION;
        return std::lerp(Mutation::MIN_MOVE_SPEED_MULTIPLIER, Mutation::MAX_MOVE_SPEED_MULTIPLIER, stageRatio);
    }

    void UpdateMutation(float deltaTimeSec, bool isPurifying) noexcept
    {
        const float rate = isPurifying ? -PlayerMutationSettings::PURIFICATION_PER_SECOND
                                       : PlayerMutationSettings::GAIN_PER_SECOND;
        mMutation = std::clamp(mMutation + rate * deltaTimeSec,
            PlayerMutationSettings::MIN_VALUE, PlayerMutationSettings::MAX_VALUE);
    }

    char mUserName[MAX_NAME_LEN] = {};
    float mYaw = 0.0F;
    float mMoveYaw = 0.0F;
    bool mMoveUp = false;
    bool mMoveDown = false;
    bool mMoveLeft = false;
    bool mMoveRight = false;
    bool mJumpRequested = false;
    bool mIsPurifying = false;
    Kimgane::Shared::Physics::RigidbodyState mMovementState;

private:
    float mMutation = PlayerMutationSettings::INITIAL_VALUE;
    Inventory mInventory;
};
