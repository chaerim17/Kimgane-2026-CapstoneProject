#pragma once

#include <DirectXMath.h>

#include <algorithm>

namespace Kimgane::Client
{
// 캐릭터 머리 위에 뜨는 체력바 데이터. 그리기(투영·가림 판정)는 렌더러가 함.
class WorldHealthBar final
{
public:
    // displayHeightOffsetM: 발 위치에서 바를 그릴 높이, visibilityHeightOffsetM: 가려짐을 판단할 몸통 높이
    WorldHealthBar(float displayHeightOffsetM, float visibilityHeightOffsetM) noexcept
        : mDisplayHeightOffsetM(displayHeightOffsetM), mVisibilityHeightOffsetM(visibilityHeightOffsetM)
    {
    }

    void SetHp(int currentHp, int maxHp) noexcept
    {
        mCurrentHp = currentHp;
        mMaxHp = maxHp;
    }

    [[nodiscard]] float GetDisplayHeightOffsetM() const noexcept
    {
        return mDisplayHeightOffsetM;
    }

    [[nodiscard]] float GetVisibilityHeightOffsetM() const noexcept
    {
        return mVisibilityHeightOffsetM;
    }

    [[nodiscard]] float GetHpRatio() const noexcept
    {
        if (mMaxHp <= 0)
        {
            return 0.0F;
        }
        return std::clamp(static_cast<float>(mCurrentHp) / static_cast<float>(mMaxHp), 0.0F, 1.0F);
    }

private:
    float mDisplayHeightOffsetM = 0.0F;
    float mVisibilityHeightOffsetM = 0.0F;
    int mCurrentHp = 0;
    int mMaxHp = 0;
};

// 렌더러에 넘기는 체력바 한 개: 주인 발 위치 + 체력바 데이터.
struct WorldHealthBarView
{
    DirectX::XMFLOAT3 footPositionM = {};
    const WorldHealthBar* healthBar = nullptr;
};
} // namespace Kimgane::Client
