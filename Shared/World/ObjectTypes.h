#pragma once

#include <cstdint>

namespace Kimgane::Shared::World
{
// ID는 개체 식별에 사용, 오브젝트 종류는 밑의 타임으로 정의
enum class ObjectType : std::uint8_t
{
    Player = 0,
    Npc = 1,
    ItemBox = 2
};
} // namespace Kimgane::Shared::World
