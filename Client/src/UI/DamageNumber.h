#pragma once

#include "../Rendering/RenderSettings.h"

#include <DirectXMath.h>

#include <algorithm>
#include <string>

namespace Kimgane::Client
{
// 피격 위치 위로 떠오르며 사라지는 데미지 숫자. GameScene이 고정 개수 풀로 재사용함.
class DamageNumber final
{
public:
    DamageNumber(float displayHeightOffsetM, float visibilityHeightOffsetM) noexcept
        : mDisplayHeightOffsetM(displayHeightOffsetM), mVisibilityHeightOffsetM(visibilityHeightOffsetM)
    {
    }

    // 새 숫자로 다시 띄움. horizontalOffsetPx는 여러 숫자가 겹치지 않게 좌우로 벌리는 값.
    void Show(int damage, const DirectX::XMFLOAT3& footPositionM, float horizontalOffsetPx)
    {
        mText = std::to_wstring(damage);
        mFootPositionM = footPositionM;
        mElapsedTimeSec = 0.0F;
        mHorizontalOffsetPx = horizontalOffsetPx;
        mActive = true;
    }

    void Update(float deltaTimeSec) noexcept
    {
        if (!mActive)
        {
            return;
        }
        mElapsedTimeSec += std::max(deltaTimeSec, 0.0F);
        if (mElapsedTimeSec >= RenderSettings::DAMAGE_NUMBER_LIFETIME_SEC)
        {
            mActive = false;
        }
    }

    [[nodiscard]] bool IsActive() const noexcept
    {
        return mActive;
    }

    [[nodiscard]] const std::wstring& GetText() const noexcept
    {
        return mText;
    }

    [[nodiscard]] const DirectX::XMFLOAT3& GetFootPositionM() const noexcept
    {
        return mFootPositionM;
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

    // 시간이 지날수록 위로 떠오름.
    [[nodiscard]] float GetDisplayHeightOffsetM() const noexcept
    {
        return mDisplayHeightOffsetM + mElapsedTimeSec * RenderSettings::DAMAGE_NUMBER_RISE_SPEED_MPS;
    }

    // 수명 끝 무렵 서서히 투명해짐.
    [[nodiscard]] float GetOpacity() const noexcept
    {
        return std::clamp((RenderSettings::DAMAGE_NUMBER_LIFETIME_SEC - mElapsedTimeSec) /
                              RenderSettings::DAMAGE_NUMBER_FADE_DURATION_SEC,
                          0.0F, 1.0F);
    }

private:
    std::wstring mText;
    DirectX::XMFLOAT3 mFootPositionM = {};
    float mElapsedTimeSec = 0.0F;
    float mHorizontalOffsetPx = 0.0F;
    float mDisplayHeightOffsetM = 0.0F;
    float mVisibilityHeightOffsetM = 0.0F;
    bool mActive = false;
};
} // namespace Kimgane::Client
