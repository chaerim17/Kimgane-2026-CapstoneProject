#pragma once

#include "../Core/GameObject.h"
#include "RenderSettings.h"

#include <algorithm>
#include <string>

namespace Kimgane::Engine
{
// NPC와 별도 객체에 붙여서 NPC가 제거된 뒤에도 마지막 데미지 숫자를 표시합니다.
class DamageNumberComponent final : public Component
{
public:
    DamageNumberComponent(GameObject& owner, float displayHeightOffsetM, float visibilityHeightOffsetM) noexcept
        : Component(owner), mDisplayHeightOffsetM(displayHeightOffsetM),
          mVisibilityHeightOffsetM(visibilityHeightOffsetM)
    {
    }

    void Reset(int damage, float horizontalOffsetPx)
    {
        mText = std::to_wstring(damage);
        mElapsedTimeSec = 0.0F;
        mHorizontalOffsetPx = horizontalOffsetPx;
        GetOwner().SetActive(true);
    }

    void Update(float deltaTimeSec) override
    {
        mElapsedTimeSec += std::max(deltaTimeSec, 0.0F);
        if (mElapsedTimeSec >= RenderSettings::DAMAGE_NUMBER_LIFETIME_SEC)
        {
            GetOwner().SetActive(false);
        }
    }

    [[nodiscard]] const std::wstring& GetText() const noexcept
    {
        return mText;
    }

    [[nodiscard]] float GetElapsedTimeSec() const noexcept
    {
        return mElapsedTimeSec;
    }

    [[nodiscard]] float GetHorizontalOffsetPx() const noexcept
    {
        return mHorizontalOffsetPx;
    }

    [[nodiscard]] float GetVisibilityHeightOffsetM() const noexcept
    {
        return mVisibilityHeightOffsetM;
    }

    [[nodiscard]] float GetDisplayHeightOffsetM() const noexcept
    {
        return mDisplayHeightOffsetM + mElapsedTimeSec * RenderSettings::DAMAGE_NUMBER_RISE_SPEED_MPS;
    }

    [[nodiscard]] float GetOpacity() const noexcept
    {
        return std::clamp((RenderSettings::DAMAGE_NUMBER_LIFETIME_SEC - mElapsedTimeSec) /
                              RenderSettings::DAMAGE_NUMBER_FADE_DURATION_SEC,
                          0.0F, 1.0F);
    }

private:
    std::wstring mText;
    float mElapsedTimeSec = 0.0F;
    float mHorizontalOffsetPx = 0.0F;
    float mDisplayHeightOffsetM;
    float mVisibilityHeightOffsetM;
};
} // namespace Kimgane::Engine
