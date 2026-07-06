#pragma once

#include <cstdint>

namespace Kimgane::Engine::TerrainSettings
{
inline constexpr std::uint32_t kDefaultSampleWidth = 65;
inline constexpr std::uint32_t kDefaultSampleLength = 65;
inline constexpr float kDefaultCellSpacingM = 0.3125F;
inline constexpr float kDefaultHeightScaleM = 1.2F;
inline constexpr float kDefaultWaveAmplitudeM = 0.55F;
inline constexpr float kDefaultWaveFrequency = 2.0F;
} // namespace Kimgane::Engine::TerrainSettings
