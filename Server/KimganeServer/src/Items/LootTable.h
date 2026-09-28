#pragma once

#include "../../../../Shared/Items/ItemTypes.h"

#include <array>
#include <cstdint>

namespace ItemBoxLoot
{
// 서버 전용 보상 설정
struct LootEntry
{
    Kimgane::Shared::Items::ItemId itemId;
    double weight; // 상대 가중치
    std::uint32_t minQuantity; // 지급수량 범위
    std::uint32_t maxQuantity;
};

// 현재 비율 1/3
inline constexpr std::array<LootEntry, 3> DEFAULT_TABLE = {
{
    {Kimgane::Shared::Items::ItemId::HpPotion, 1.0, 1, 1},
    {Kimgane::Shared::Items::ItemId::Chip, 1.0, 1, 1},
    {Kimgane::Shared::Items::ItemId::Armor, 1.0, 1, 1}
}};
} // namespace ItemBoxLoot
