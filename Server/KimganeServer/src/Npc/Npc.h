#pragma once

#include "../Pch.h"
#include "../Config/ServerConfig.h"
#include <chrono>

class Npc
{    
public:
    int mId{};

    float mX{};
    float mY{};
    float mZ{};

    float mYaw{};

    float mMoveSpeed = 5.0f;
    
    int mMaxHp{NPC_MAX_HP};
    int mCurrentHp{mMaxHp};

    std::chrono::steady_clock::time_point mLastMove;

    void RandomMove();
};

