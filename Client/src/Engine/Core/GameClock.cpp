#include "Pch.h"

#include "GameClock.h"

#include <algorithm>

namespace Kimgane::Engine
{
namespace
{
constexpr float kMaxDeltaTimeSec = 0.1F;
}

GameClock::GameClock() noexcept
{
    Reset();
}

void GameClock::Reset() noexcept
{
    previousTime_ = std::chrono::steady_clock::now();
    deltaTimeSec_ = 0.0F;
}

float GameClock::Tick() noexcept
{
    const auto currentTime = std::chrono::steady_clock::now();
    const std::chrono::duration<float> deltaTime = currentTime - previousTime_;
    previousTime_ = currentTime;
    deltaTimeSec_ = std::clamp(deltaTime.count(), 0.0F, kMaxDeltaTimeSec);
    return deltaTimeSec_;
}

float GameClock::GetDeltaTimeSec() const noexcept
{
    return deltaTimeSec_;
}
} // namespace Kimgane::Engine
