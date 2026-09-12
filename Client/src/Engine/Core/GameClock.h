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
    // 고정 물리 누적용 실제 경과 시간입니다. 프레임 갱신용 delta의 상한을 적용하지 않습니다.
    [[nodiscard]] double GetElapsedTimeSec() const noexcept;

private:
    std::chrono::steady_clock::time_point mPreviousTime;
    float mDeltaTimeSec = 0.0F;
    double mElapsedTimeSec = 0.0;
};
} // namespace Kimgane::Engine
