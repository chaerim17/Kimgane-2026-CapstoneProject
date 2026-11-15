#include "Pch.h"

#include "PlayerHud.h"

#include "../Rendering/Dx12Renderer.h"
#include "../Rendering/TextComponent.h"

#include <algorithm>
#include <cmath>
#include <cwchar>
#include <string>

namespace Kimgane::Engine
{
namespace
{
using Kimgane::Shared::Items::ItemId;

constexpr int ALIGN_LEFT = static_cast<int>(TextHorizontalAlignment::Left);
constexpr int ALIGN_RIGHT = static_cast<int>(TextHorizontalAlignment::Right);
constexpr int ALIGN_TOP = static_cast<int>(TextVerticalAlignment::Top);
constexpr int ALIGN_MIDDLE = static_cast<int>(TextVerticalAlignment::Center);
constexpr int ALIGN_BOTTOM = static_cast<int>(TextVerticalAlignment::Bottom);

// 줄 맨 왼쪽의 상태 아이콘 영역
DirectX::XMFLOAT4 BuildStatusIconRect(float rowBottomPx) noexcept
{
    using namespace PlayerHudSettings;
    const float centerYPx = rowBottomPx - STATUS_ROW_HEIGHT_PX * 0.5F;
    const float halfPx = STATUS_ICON_SIZE_PX * 0.5F;
    return {SCREEN_MARGIN_PX, centerYPx - halfPx, SCREEN_MARGIN_PX + STATUS_ICON_SIZE_PX, centerYPx + halfPx};
}

// 아이콘 오른쪽, 줄 가운데에 높이 barHeightPx인 바 영역
DirectX::XMFLOAT4 BuildStatusBarRect(float rowBottomPx, float barHeightPx) noexcept
{
    using namespace PlayerHudSettings;
    const float leftPx = SCREEN_MARGIN_PX + STATUS_ICON_SIZE_PX + STATUS_ICON_TO_BAR_GAP_PX;
    const float centerYPx = rowBottomPx - STATUS_ROW_HEIGHT_PX * 0.5F;
    const float halfPx = barHeightPx * 0.5F;
    return {leftPx, centerYPx - halfPx, leftPx + STATUS_BAR_WIDTH_PX, centerYPx + halfPx};
}

// 배경 → 비율만큼 채움 → 테두리 순서로 그리는 공통 바
void QueueFilledBar(Dx12Renderer& renderer,
                    const DirectX::XMFLOAT4& barRect,
                    float ratio,
                    const DirectX::XMFLOAT4& fillColor)
{
    using namespace PlayerHudSettings;
    renderer.QueueUiFillRect(barRect, STATUS_BAR_BACKGROUND_COLOR);

    const float clampedRatio = std::clamp(ratio, 0.0F, 1.0F);
    if (clampedRatio > 0.0F)
    {
        const float fillRightPx = barRect.x + (barRect.z - barRect.x) * clampedRatio;
        renderer.QueueUiFillRect({barRect.x, barRect.y, fillRightPx, barRect.w}, fillColor);
    }

    renderer.QueueUiStrokeRect(barRect, STATUS_BAR_BORDER_COLOR, 1.0F);
}
} // namespace

void PlayerHud::Queue(Dx12Renderer& renderer, const PlayerHudState& state) const
{
    using namespace PlayerHudSettings;

    // 아래에서부터 한 줄씩 위로 쌓음
    const float firstRowBottomPx = static_cast<float>(renderer.GetHeightPx()) - SCREEN_MARGIN_PX;
    const float rowStepPx = STATUS_ROW_HEIGHT_PX + STATUS_ROW_GAP_PX;
    QueueHpRow(renderer, state, firstRowBottomPx);
    QueueMutationRow(renderer, state, firstRowBottomPx - rowStepPx);
    QueueSpeedRow(renderer, state, firstRowBottomPx - rowStepPx * 2.0F);

    QueueQuickSlots(renderer, state);
}

void PlayerHud::QueueHpRow(Dx12Renderer& renderer, const PlayerHudState& state, float rowBottomPx) const
{
    using namespace PlayerHudSettings;

    renderer.QueueUiImage(STATUS_HEALTH_ICON_PATH, BuildStatusIconRect(rowBottomPx));

    const DirectX::XMFLOAT4 barRect = BuildStatusBarRect(rowBottomPx, HP_BAR_HEIGHT_PX);
    const float hpRatio =
        state.maxHp > 0 ? static_cast<float>(state.currentHp) / static_cast<float>(state.maxHp) : 0.0F;
    QueueFilledBar(renderer, barRect, hpRatio, HP_BAR_FILL_COLOR);

    // 바 안쪽 오른쪽에 "현재 / 최대"
    renderer.QueueUiText(std::to_wstring(std::max(state.currentHp, 0)) + L" / " + std::to_wstring(state.maxHp),
                         {barRect.x, barRect.y, barRect.z - STATUS_BAR_TEXT_PADDING_PX, barRect.w}, STATUS_TEXT_COLOR,
                         STATUS_FONT_SIZE_DIP, ALIGN_RIGHT, ALIGN_MIDDLE);
}

void PlayerHud::QueueMutationRow(Dx12Renderer& renderer, const PlayerHudState& state, float rowBottomPx) const
{
    using namespace PlayerHudSettings;

    renderer.QueueUiImage(STATUS_MUTATION_ICON_PATH, BuildStatusIconRect(rowBottomPx));

    const DirectX::XMFLOAT4 barRect = BuildStatusBarRect(rowBottomPx, MUTATION_BAR_HEIGHT_PX);
    QueueFilledBar(renderer, barRect, state.mutationRatio, MUTATION_BAR_FILL_COLOR);

    // 바 오른쪽 바깥에 퍼센트
    const int percent = static_cast<int>(std::lround(std::clamp(state.mutationRatio, 0.0F, 1.0F) * 100.0F));
    const float textLeftPx = barRect.z + STATUS_BAR_TEXT_PADDING_PX;
    renderer.QueueUiText(std::to_wstring(percent) + L"%",
                         {textLeftPx, rowBottomPx - STATUS_ROW_HEIGHT_PX, textLeftPx + 60.0F, rowBottomPx},
                         STATUS_TEXT_COLOR, STATUS_FONT_SIZE_DIP, ALIGN_LEFT, ALIGN_MIDDLE);
}

void PlayerHud::QueueSpeedRow(Dx12Renderer& renderer, const PlayerHudState& state, float rowBottomPx) const
{
    using namespace PlayerHudSettings;

    renderer.QueueUiImage(STATUS_SPEED_ICON_PATH, BuildStatusIconRect(rowBottomPx));

    wchar_t speedText[32] = {};
    std::swprintf(speedText, std::size(speedText), L"%.1f m/s", state.moveSpeedMps);
    const float textLeftPx = SCREEN_MARGIN_PX + STATUS_ICON_SIZE_PX + STATUS_ICON_TO_BAR_GAP_PX;
    renderer.QueueUiText(speedText,
                         {textLeftPx, rowBottomPx - STATUS_ROW_HEIGHT_PX, textLeftPx + STATUS_BAR_WIDTH_PX, rowBottomPx},
                         SPEED_TEXT_COLOR, STATUS_FONT_SIZE_DIP, ALIGN_LEFT, ALIGN_MIDDLE);
}

void PlayerHud::QueueQuickSlots(Dx12Renderer& renderer, const PlayerHudState& state) const
{
    using namespace PlayerHudSettings;

    const float screenWidthPx = static_cast<float>(renderer.GetWidthPx());
    const float screenHeightPx = static_cast<float>(renderer.GetHeightPx());
    const float totalWidthPx =
        QUICK_SLOT_SIZE_PX * QUICK_SLOT_COUNT + QUICK_SLOT_GAP_PX * static_cast<float>(QUICK_SLOT_COUNT - 1);
    const float startLeftPx = (screenWidthPx - totalWidthPx) * 0.5F;
    const float bottomPx = screenHeightPx - SCREEN_MARGIN_PX;
    const float topPx = bottomPx - QUICK_SLOT_SIZE_PX;
    const float iconInsetPx = QUICK_SLOT_SIZE_PX * QUICK_SLOT_ICON_INSET_RATIO;
    const float textInsetPx = QUICK_SLOT_TEXT_INSET_PX;

    for (int slotIndex = 0; slotIndex < QUICK_SLOT_COUNT; ++slotIndex)
    {
        const QuickSlotState& slot = state.quickSlots[static_cast<size_t>(slotIndex)];
        const float leftPx = startLeftPx + (QUICK_SLOT_SIZE_PX + QUICK_SLOT_GAP_PX) * static_cast<float>(slotIndex);
        const float rightPx = leftPx + QUICK_SLOT_SIZE_PX;
        const bool isSelected = slotIndex == state.selectedQuickSlotIndex;

        // 1) 슬롯 틀 (선택된 칸은 청록 테두리 버전)
        renderer.QueueUiImage(isSelected ? QUICK_SLOT_FRAME_SELECTED_PATH : QUICK_SLOT_FRAME_PATH,
                              {leftPx, topPx, rightPx, bottomPx});

        // 2) 아이템 아이콘. 서버 수량이 없는 칸(정화주사기 등)이나 0개면 흐리게
        const bool hasQuantity = slot.itemId != ItemId::None;
        if (slot.iconPath != nullptr)
        {
            const float opacity = hasQuantity && slot.quantity > 0 ? 1.0F : QUICK_SLOT_UNAVAILABLE_OPACITY;
            renderer.QueueUiImage(slot.iconPath,
                                  {leftPx + iconInsetPx, topPx + iconInsetPx, rightPx - iconInsetPx, bottomPx - iconInsetPx},
                                  opacity);
        }

        // 3) 왼쪽 위 슬롯 번호(1~5)
        renderer.QueueUiText(std::to_wstring(slotIndex + 1),
                             {leftPx + textInsetPx, topPx + textInsetPx * 0.6F, rightPx, bottomPx}, QUICK_SLOT_TEXT_COLOR,
                             QUICK_SLOT_KEY_FONT_SIZE_DIP, ALIGN_LEFT, ALIGN_TOP);

        // 4) 오른쪽 아래 보유 수량
        if (hasQuantity)
        {
            renderer.QueueUiText(std::to_wstring(slot.quantity),
                                 {leftPx, topPx, rightPx - textInsetPx, bottomPx - textInsetPx * 0.6F},
                                 QUICK_SLOT_TEXT_COLOR, QUICK_SLOT_COUNT_FONT_SIZE_DIP, ALIGN_RIGHT, ALIGN_BOTTOM);
        }
    }
}
} // namespace Kimgane::Engine
