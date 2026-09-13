#pragma once

#include "../Core/Component.h"

#include <algorithm>

namespace Kimgane::Engine
{
// 소유 GameObject 위에 체력바를 그리라는 표시만 담는 순수 데이터 컴포넌트.
// displayHeightOffsetM은 바를 "그리는" 위치(머리 위)이고, visibilityHeightOffsetM은
// "가려짐을 판단하는" 기준점(몸통 높이)이다. 둘을 분리한 이유는, 바 위치만 기준으로 가려짐을
// 판단하면 언덕이 몸은 가려도 둥실 뜬 바 위치까지는 못 가려서 몸은 안 보이는데 바만 보이는 문제가 생기기 때문.
// 실제 HP 갱신은 GameScene이 매 프레임 NetworkManager에서 읽어 SetHp()로 넣어주고,
// 화면에 그리는 건 Dx12Renderer가 이 컴포넌트를 찾아서 처리함.
class HealthBarComponent final : public Component
{
public:
    HealthBarComponent(GameObject& owner, float displayHeightOffsetM, float visibilityHeightOffsetM) noexcept
        : Component(owner), mDisplayHeightOffsetM(displayHeightOffsetM),
          mVisibilityHeightOffsetM(visibilityHeightOffsetM)
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
} // namespace Kimgane::Engine
