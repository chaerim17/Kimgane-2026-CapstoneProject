#pragma once

#include "PlayerHudSettings.h"

#include <array>
#include <cstdint>

namespace Kimgane::Engine
{
class Dx12Renderer;

struct QuickSlotState
{
    Kimgane::Shared::Items::ItemId itemId = Kimgane::Shared::Items::ItemId::None;
    const wchar_t* iconPath = nullptr;
    std::uint64_t quantity = 0;
};

// HUD가 그릴 값 묶음. GameScene이 매 프레임 채워서 넘김.
struct PlayerHudState
{
    int currentHp = 0;
    int maxHp = 0;
    float mutationRatio = 0.0F; // 0.0(정상) ~ 1.0(완전 변이)
    float moveSpeedMps = 0.0F;
    int selectedQuickSlotIndex = PlayerHudSettings::NO_SELECTED_QUICK_SLOT; // 0~4, 선택 없음이면 -1
    std::array<QuickSlotState, PlayerHudSettings::QUICK_SLOT_COUNT> quickSlots = {};
    // F키로 할 수 있는 상호작용 안내. nullptr이면 표시하지 않음.
    const wchar_t* interactionPrompt = nullptr;
};

// 화면 하단 HUD. 상태를 받아 렌더러의 2D UI 요청(QueueUi*)으로 바꾸기만 하고 값은 들고 있지 않음.
class PlayerHud final
{
public:
    void Queue(Dx12Renderer& renderer, const PlayerHudState& state) const;

private:
    // rowBottomPx: 그 줄의 아래쪽 y. 아이콘과 바를 줄 가운데에 맞춰 그림.
    void QueueHpRow(Dx12Renderer& renderer, const PlayerHudState& state, float rowBottomPx) const;
    void QueueMutationRow(Dx12Renderer& renderer, const PlayerHudState& state, float rowBottomPx) const;
    void QueueSpeedRow(Dx12Renderer& renderer, const PlayerHudState& state, float rowBottomPx) const;
    void QueueQuickSlots(Dx12Renderer& renderer, const PlayerHudState& state) const;
    void QueueInteractionPrompt(Dx12Renderer& renderer, const PlayerHudState& state) const;
};
} // namespace Kimgane::Engine
