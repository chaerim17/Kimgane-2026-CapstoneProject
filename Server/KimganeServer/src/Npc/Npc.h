#pragma once

#include "../Pch.h"
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
    
    int mHp{100};
    int mAttack{10};

    std::chrono::steady_clock::time_point mLastMove;

    void RandomMove();
};

