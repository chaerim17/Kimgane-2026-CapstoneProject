#pragma once

#include <cstdint>

namespace Kimgane::Shared::Items
{
// 클라이언트/서버가 공유하는 아이템 식별용
enum class ItemId : std::uint16_t
{
    None = 0,
    HpPotion = 1,
    Chip = 2,
    Armor = 3
};

struct ItemReward
{
    ItemId itemId = ItemId::None;
    std::uint32_t quantity = 0;
};
} // namespace Kimgane::Shared::Items
