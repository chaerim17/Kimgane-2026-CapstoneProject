#pragma once

#include <chrono>

namespace Kimgane::Engine
{
class GameClock final
{
public:
    GameClock() noexcept;

    void Reset() noexcept;
    [[nodiscard]] float Tick() noexcept;
    [[nodiscard]] float GetDeltaTimeSec() const noexcept;

private:
    std::chrono::steady_clock::time_point mPreviousTime;
    float mDeltaTimeSec = 0.0F;
};
} // namespace Kimgane::Engine
