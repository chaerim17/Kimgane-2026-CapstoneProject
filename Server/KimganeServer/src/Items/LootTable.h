#pragma once

#include "../../../../Shared/Items/ItemTypes.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <random>
#include <span>
#include <stdexcept>
#include <vector>

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

// 랜덤으로 보상 아이템 선택하고 수량 결정
[[nodiscard]] inline Kimgane::Shared::Items::ItemReward DrawReward(
    std::mt19937& random, std::span<const LootEntry> table = DEFAULT_TABLE)
{
    if (table.empty())
        throw std::invalid_argument("Loot table must not be empty");

    std::vector<double> weights;
    weights.reserve(table.size());
    double totalWeight = 0.0;
    for (const auto& entry : table)
    {
        if (!std::isfinite(entry.weight) || entry.weight < 0.0)
            throw std::invalid_argument("Loot weight must be finite and non-negative");
        if (entry.itemId == Kimgane::Shared::Items::ItemId::None ||
            entry.minQuantity == 0 || entry.minQuantity > entry.maxQuantity)
            throw std::invalid_argument("Loot entry requires an item and a valid positive quantity range");

        totalWeight += entry.weight;
        weights.push_back(entry.weight);
    }
    if (!std::isfinite(totalWeight) || totalWeight <= 0.0)
        throw std::invalid_argument("Total loot weight must be finite and positive");

    std::discrete_distribution<std::size_t> selectItem(weights.begin(), weights.end());
    const auto& entry = table[selectItem(random)];
    std::uniform_int_distribution<std::uint32_t> selectQuantity(entry.minQuantity, entry.maxQuantity);
    return {entry.itemId, selectQuantity(random)};
}
} // namespace ItemBoxLoot
