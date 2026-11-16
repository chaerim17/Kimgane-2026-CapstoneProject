#pragma once

#include <DirectXMath.h>

#include "../../Shared/Items/ItemTypes.h"

#include <array>

// 화면 하단 HUD(상태 바·이동속도·퀵슬롯)의 배치, 색, 이미지 경로. 단위는 화면 픽셀.
namespace Kimgane::Engine::PlayerHudSettings
{
// 화면 가장자리와 HUD 사이 여백
inline constexpr float SCREEN_MARGIN_PX = 24.0F;

// ---------------------------------------------------------------------------
// 왼쪽 아래 상태 영역: 아래에서부터 HP줄 → 변이줄 → 이동속도줄
// [아이콘] [바] 형태로 한 줄씩 쌓음
// ---------------------------------------------------------------------------
inline constexpr float STATUS_ICON_SIZE_PX = 32.0F;
inline constexpr float STATUS_ROW_HEIGHT_PX = 32.0F;
inline constexpr float STATUS_ROW_GAP_PX = 6.0F;
inline constexpr float STATUS_ICON_TO_BAR_GAP_PX = 8.0F;
inline constexpr float STATUS_BAR_WIDTH_PX = 220.0F;
inline constexpr float STATUS_BAR_TEXT_PADDING_PX = 8.0F;
inline constexpr float STATUS_FONT_SIZE_DIP = 15.0F;
inline const DirectX::XMFLOAT4 STATUS_TEXT_COLOR = {1.0F, 1.0F, 1.0F, 1.0F};
inline const DirectX::XMFLOAT4 STATUS_BAR_BACKGROUND_COLOR = {0.03F, 0.04F, 0.07F, 0.70F};
inline const DirectX::XMFLOAT4 STATUS_BAR_BORDER_COLOR = {0.75F, 0.80F, 0.88F, 0.55F};

inline constexpr wchar_t STATUS_HEALTH_ICON_PATH[] = L"Assets/Textures/UI/HUD/status-health.png";
inline constexpr wchar_t STATUS_MUTATION_ICON_PATH[] = L"Assets/Textures/UI/HUD/status-mutation.png";
inline constexpr wchar_t STATUS_SPEED_ICON_PATH[] = L"Assets/Textures/UI/HUD/status-speed.png";

// HP바
inline constexpr float HP_BAR_HEIGHT_PX = 20.0F;
inline const DirectX::XMFLOAT4 HP_BAR_FILL_COLOR = {0.82F, 0.12F, 0.12F, 0.95F};

// 변이바 (아이콘 색에 맞춘 보라)
inline constexpr float MUTATION_BAR_HEIGHT_PX = 14.0F;
inline const DirectX::XMFLOAT4 MUTATION_BAR_FILL_COLOR = {0.62F, 0.22F, 0.88F, 0.95F};

// 이동속도 글자 (아이콘 색에 맞춘 청록)
inline const DirectX::XMFLOAT4 SPEED_TEXT_COLOR = {0.45F, 0.95F, 1.0F, 1.0F};

// 서버가 아직 플레이어 HP를 보내지 않아서 쓰는 임시 값. 서버 연동 시 제거.
inline constexpr int PLACEHOLDER_MAX_HP = 100;
inline constexpr int PLACEHOLDER_CURRENT_HP = 100;

// ---------------------------------------------------------------------------
// 가운데 아래 퀵슬롯
// ---------------------------------------------------------------------------
inline constexpr int QUICK_SLOT_COUNT = 5;
inline constexpr float QUICK_SLOT_SIZE_PX = 64.0F;
inline constexpr float QUICK_SLOT_GAP_PX = 6.0F;
// 슬롯 틀 이미지의 안쪽 유리 영역에 아이콘이 들어가도록 사방을 이 비율만큼 비움
inline constexpr float QUICK_SLOT_ICON_INSET_RATIO = 0.16F;
// 슬롯 번호·수량 글자를 틀 테두리 안쪽에 두기 위한 여백
inline constexpr float QUICK_SLOT_TEXT_INSET_PX = 9.0F;
inline constexpr float QUICK_SLOT_KEY_FONT_SIZE_DIP = 12.0F;
inline constexpr float QUICK_SLOT_COUNT_FONT_SIZE_DIP = 14.0F;
inline const DirectX::XMFLOAT4 QUICK_SLOT_TEXT_COLOR = {1.0F, 1.0F, 1.0F, 1.0F};
// 보유 수량이 0이거나 아직 획득 불가능한 아이템은 흐리게 표시
inline constexpr float QUICK_SLOT_UNAVAILABLE_OPACITY = 0.35F;

inline constexpr wchar_t QUICK_SLOT_FRAME_PATH[] = L"Assets/Textures/UI/HUD/quick-slot-normal.png";
inline constexpr wchar_t QUICK_SLOT_FRAME_SELECTED_PATH[] = L"Assets/Textures/UI/HUD/quick-slot-selected.png";

// ---------------------------------------------------------------------------
// 퀵슬롯 위 상호작용 안내 ("[F] 정화", "[F] 상자 열기")
// ---------------------------------------------------------------------------
inline constexpr float INTERACTION_PROMPT_WIDTH_PX = 170.0F;
inline constexpr float INTERACTION_PROMPT_HEIGHT_PX = 34.0F;
inline constexpr float INTERACTION_PROMPT_GAP_ABOVE_QUICK_SLOT_PX = 18.0F;
inline constexpr float INTERACTION_PROMPT_FONT_SIZE_DIP = 16.0F;
inline const DirectX::XMFLOAT4 INTERACTION_PROMPT_BACKGROUND_COLOR = {0.03F, 0.04F, 0.07F, 0.75F};
inline const DirectX::XMFLOAT4 INTERACTION_PROMPT_BORDER_COLOR = {0.45F, 0.95F, 1.0F, 0.85F};
inline const DirectX::XMFLOAT4 INTERACTION_PROMPT_TEXT_COLOR = {1.0F, 1.0F, 1.0F, 1.0F};
inline constexpr wchar_t INTERACTION_PROMPT_PURIFY_TEXT[] = L"[F] 정화";
inline constexpr wchar_t INTERACTION_PROMPT_OPEN_ITEM_BOX_TEXT[] = L"[F] 상자 열기";

// 슬롯 하나에 무엇을 둘지. itemId가 None이면 서버 수량이 없는 칸(아이콘만 표시 또는 빈 칸).
struct QuickSlotDefinition
{
    Kimgane::Shared::Items::ItemId itemId = Kimgane::Shared::Items::ItemId::None;
    const wchar_t* iconPath = nullptr;
};

// 1번 칸: 정화주사기. Shared ItemId가 아직 없어서 아이콘만 둠(ItemId가 추가되면 itemId만 채우면 수량이 연결됨).
// 2~5번 칸: 비워 둠.
inline constexpr std::array<QuickSlotDefinition, QUICK_SLOT_COUNT> QUICK_SLOT_DEFINITIONS = {{
    {Kimgane::Shared::Items::ItemId::None, L"Assets/Textures/UI/Inventory/purification-syringe.png"},
    {Kimgane::Shared::Items::ItemId::None, nullptr},
    {Kimgane::Shared::Items::ItemId::None, nullptr},
    {Kimgane::Shared::Items::ItemId::None, nullptr},
    {Kimgane::Shared::Items::ItemId::None, nullptr},
}};

// 선택된 칸이 없음을 나타내는 값
inline constexpr int NO_SELECTED_QUICK_SLOT = -1;
} // namespace Kimgane::Engine::PlayerHudSettings
