#pragma once

#include "../Core/Component.h"

#include <DirectXMath.h>

#include <algorithm>
#include <string>
#include <utility>

namespace Kimgane::Engine
{
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

class TextComponent final : public Component
{
public:
    explicit TextComponent(GameObject& owner, std::wstring text = {}, float fontSizeDip = 28.0F)
        : Component(owner), mText(std::move(text)), mFontSizeDip(std::max(fontSizeDip, 1.0F))
    {
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

    void SetFontSizeDip(float fontSizeDip) noexcept
    {
        mFontSizeDip = std::max(fontSizeDip, 1.0F);
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
    std::wstring mText;
    DirectX::XMFLOAT4 mColorLinear = {1.0F, 1.0F, 1.0F, 1.0F};
    DirectX::XMFLOAT2 mInsetRatio = {0.06F, 0.12F};
    float mFontSizeDip = 28.0F;
    TextHorizontalAlignment mHorizontalAlignment = TextHorizontalAlignment::Center;
    TextVerticalAlignment mVerticalAlignment = TextVerticalAlignment::Center;
};
} // namespace Kimgane::Engine
