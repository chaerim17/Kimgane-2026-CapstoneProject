#pragma once

namespace Kimgane::Engine::RenderRootParameter
{
inline constexpr unsigned int kScene = 0;
inline constexpr unsigned int kObject = 1;

inline constexpr unsigned int kMatrix32BitCount = 16;
inline constexpr unsigned int kSceneConstants32BitCount = kMatrix32BitCount;
inline constexpr unsigned int kObjectWorld32BitCount = kMatrix32BitCount;
inline constexpr unsigned int kObjectWorld32BitOffset = 0;
inline constexpr unsigned int kObjectColor32BitOffset = 16;
inline constexpr unsigned int kObjectConstants32BitCount = 20;
} // namespace Kimgane::Engine::RenderRootParameter
