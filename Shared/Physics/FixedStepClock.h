#pragma once

#include <algorithm>
#include <cmath>

namespace Kimgane::Shared::Physics
{
inline constexpr int FIXED_PHYSICS_HZ = 60;
inline constexpr double FIXED_STEP_SEC = 1.0 / FIXED_PHYSICS_HZ;
inline constexpr float FIXED_STEP_DELTA_SEC = static_cast<float>(FIXED_STEP_SEC);

// Wall-clock accumulation is separate from simulation time. Long stalls drop whole
// excess steps after the catch-up limit; only the fractional remainder is retained.
class FixedStepClock final
{
public:
    static constexpr int MAX_STEPS_PER_UPDATE = 8;

    int Advance(double elapsedSec) noexcept
    {
        if (!std::isfinite(elapsedSec) || elapsedSec <= 0.0)
        {
            return 0;
        }
        mAccumulatorSec += std::min(elapsedSec, MAX_ELAPSED_SEC);
        const int availableSteps = static_cast<int>((mAccumulatorSec + ROUNDING_TOLERANCE_SEC) / FIXED_STEP_SEC);
        mAccumulatorSec = std::max(0.0, mAccumulatorSec - availableSteps * FIXED_STEP_SEC);
        return std::min(availableSteps, MAX_STEPS_PER_UPDATE);
    }

    void Reset() noexcept { mAccumulatorSec = 0.0; }

    [[nodiscard]] float GetInterpolationAlpha() const noexcept
    {
        return static_cast<float>(mAccumulatorSec / FIXED_STEP_SEC);
    }

    [[nodiscard]] double GetTimeUntilNextStepSec() const noexcept
    {
        return FIXED_STEP_SEC - mAccumulatorSec;
    }

private:
    static constexpr double MAX_ELAPSED_SEC = 0.25;
    static constexpr double ROUNDING_TOLERANCE_SEC = 1.0e-9;
    double mAccumulatorSec = 0.0;
};
} // namespace Kimgane::Shared::Physics
