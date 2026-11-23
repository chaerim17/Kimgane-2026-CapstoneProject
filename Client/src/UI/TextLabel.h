#pragma once

#include <DirectXMath.h>

#include <algorithm>
#include <string>
#include <utility>

namespace Kimgane::Client
{
class GameObject;

enum class TextHorizontalAlignment
{
    Left,
    Center,
    Right
};

enum class TextVerticalAlignment
{
    Top,
    Center,
    Bottom
};

// 메뉴 버튼 같은 UI 패널 위에 그리는 글자. 패널의 화면 사각형 안쪽(inset 비율만큼 여백)에 맞춰 그려짐.
// Scene이 소유하고, 패널이 비활성이면 그리지 않음.
class TextLabel final
{
public:
    TextLabel(const GameObject& panel, std::wstring text, float fontSizeDip)
        : mPanel(&panel), mText(std::move(text)), mFontSizeDip(std::max(fontSizeDip, 1.0F))
    {
    }

    [[nodiscard]] const GameObject& GetPanel() const noexcept
    {
        return *mPanel;
    }

    [[nodiscard]] const std::wstring& GetText() const noexcept
    {
        return mText;
    }

    void SetText(std::wstring text)
    {
        mText = std::move(text);
    }

    [[nodiscard]] const DirectX::XMFLOAT4& GetColorLinear() const noexcept
    {
        return mColorLinear;
    }

    void SetColorLinear(const DirectX::XMFLOAT4& colorLinear) noexcept
    {
        mColorLinear = colorLinear;
    }

    [[nodiscard]] float GetFontSizeDip() const noexcept
    {
        return mFontSizeDip;
    }

    [[nodiscard]] const DirectX::XMFLOAT2& GetInsetRatio() const noexcept
    {
        return mInsetRatio;
    }

    void SetInsetRatio(float horizontalInsetRatio, float verticalInsetRatio) noexcept
    {
        mInsetRatio.x = std::clamp(horizontalInsetRatio, 0.0F, 0.45F);
        mInsetRatio.y = std::clamp(verticalInsetRatio, 0.0F, 0.45F);
    }

    [[nodiscard]] TextHorizontalAlignment GetHorizontalAlignment() const noexcept
    {
        return mHorizontalAlignment;
    }

    [[nodiscard]] TextVerticalAlignment GetVerticalAlignment() const noexcept
    {
        return mVerticalAlignment;
    }

    void SetAlignment(TextHorizontalAlignment horizontalAlignment, TextVerticalAlignment verticalAlignment) noexcept
    {
        mHorizontalAlignment = horizontalAlignment;
        mVerticalAlignment = verticalAlignment;
    }

private:
    const GameObject* mPanel = nullptr;
    std::wstring mText;
    DirectX::XMFLOAT4 mColorLinear = {1.0F, 1.0F, 1.0F, 1.0F};
    DirectX::XMFLOAT2 mInsetRatio = {0.06F, 0.12F};
    float mFontSizeDip = 28.0F;
    TextHorizontalAlignment mHorizontalAlignment = TextHorizontalAlignment::Center;
    TextVerticalAlignment mVerticalAlignment = TextVerticalAlignment::Center;
};
} // namespace Kimgane::Client
