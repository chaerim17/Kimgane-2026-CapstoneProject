#pragma once

#include "../Pch.h"
#include "../Config/ServerConfig.h"
#include <chrono>
#include "../World/GameObject.h"

class Npc final : public GameObject
{
public:
    explicit Npc(int id) : GameObject(id, ObjectType::Npc) {}
    [[nodiscard]] const Vec3& GetPositionM() const noexcept override { return mPositionM; }
    [[nodiscard]] float GetYawRad() const noexcept override { return mYaw; }

    Vec3 mPositionM = {};
    float mYaw = 0.0F;

    float mMoveSpeed = 5.0f;
    
    int mMaxHp{NPC_MAX_HP};
    int mCurrentHp{mMaxHp};

    std::chrono::steady_clock::time_point mLastMove;

    void RandomMove();
};

