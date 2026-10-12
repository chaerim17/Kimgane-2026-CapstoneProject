#pragma once

#include "../Physics/CollisionTypes.h"
#include <array>

namespace Kimgane::Shared::Npc
{
enum class HitPart { Body, Head };

struct HitBox
{
    HitPart part;
    Physics::Box localBox;
};

// 발 기준 로컬 좌표(m). 1.8m 사람형 모델 기준 초기값.
inline constexpr std::array<HitBox, 2> HUMANOID_HIT_BOXES{{
    {HitPart::Body, {{0.0F, 0.725F, 0.0F}, {0.35F, 0.725F, 0.22F}}},
    {HitPart::Head, {{0.0F, 1.625F, 0.0F}, {0.18F, 0.175F, 0.18F}}}
}};
}
