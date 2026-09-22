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
    mElapsedTimeSec = 0.0;
}

float GameClock::Tick() noexcept
{
    const auto currentTime = std::chrono::steady_clock::now();
    const std::chrono::duration<double> deltaTime = currentTime - mPreviousTime;
    mPreviousTime = currentTime;
    mElapsedTimeSec = std::max(0.0, deltaTime.count());
    mDeltaTimeSec = static_cast<float>(std::min(mElapsedTimeSec, static_cast<double>(MAX_DELTA_TIME_SEC)));
    return mDeltaTimeSec;
}

float GameClock::GetDeltaTimeSec() const noexcept
{
    return mDeltaTimeSec;
}

double GameClock::GetElapsedTimeSec() const noexcept
{
    return mElapsedTimeSec;
}
} // namespace Kimgane::Engine
