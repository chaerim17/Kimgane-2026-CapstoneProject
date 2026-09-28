#pragma once

#include "../../../../Shared/Items/ItemTypes.h"

#include <cstdint>
#include <map>

// 슬롯과 보유량 상한은 두지 않고 무한 합산하는 상태
class Inventory
{
public:
    using ItemId = Kimgane::Shared::Items::ItemId;
    using ItemReward = Kimgane::Shared::Items::ItemReward;
    using Quantity = std::uint64_t;

    // 잘못된 ID / 0개 지급이면 저장 상태를 변경X
    [[nodiscard]] bool TryAdd(const ItemReward& reward)
    {
        if (reward.quantity == 0)
            return false;

        switch (reward.itemId)
        {
        case ItemId::HpPotion:
        case ItemId::Chip:
        case ItemId::Armor:
            break;
        default:
            return false;
        }

        mItems[reward.itemId] += reward.quantity;
        return true;
    }

    [[nodiscard]] Quantity GetQuantity(ItemId itemId) const noexcept
    {
        const auto it = mItems.find(itemId);
        // 아이템 없으면 0반환
        if (it == mItems.end())
            return 0;

        return it->second;
    }

    [[nodiscard]] const std::map<ItemId, Quantity>& GetItems() const noexcept { return mItems; }

private:
    std::map<ItemId, Quantity> mItems;
};
