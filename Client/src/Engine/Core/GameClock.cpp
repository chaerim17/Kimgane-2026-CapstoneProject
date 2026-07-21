#include "Pch.h"

#include "GameClock.h"

#include <algorithm>

namespace Kimgane::Engine
{
namespace
{
constexpr float MAX_DELTA_TIME_SEC = 0.1F;
}

GameClock::GameClock() noexcept
{
    Reset();
}

void GameClock::Reset() noexcept
{
    mPreviousTime = std::chrono::steady_clock::now();
    mDeltaTimeSec = 0.0F;
}

float GameClock::Tick() noexcept
{
    const auto currentTime = std::chrono::steady_clock::now();
    const std::chrono::duration<float> deltaTime = currentTime - mPreviousTime;
    mPreviousTime = currentTime;
    mDeltaTimeSec = std::clamp(deltaTime.count(), 0.0F, MAX_DELTA_TIME_SEC);
    return mDeltaTimeSec;
}

float GameClock::GetDeltaTimeSec() const noexcept
{
    return mDeltaTimeSec;
}
} // namespace Kimgane::Engine
