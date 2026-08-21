#pragma once

#include "../Core/Component.h"

#include <DirectXMath.h>

namespace Kimgane::Engine
{
class NetworkSmoothingComponent final : public Component
{
public:
    explicit NetworkSmoothingComponent(GameObject& owner) noexcept;

    void Update(float deltaTimeSec) override;

    void SetTargetPositionM(const DirectX::XMFLOAT3& positionM) noexcept;
    void SnapToPositionM(const DirectX::XMFLOAT3& positionM) noexcept;
    void SetMoveSpeedMps(float moveSpeedMps) noexcept;

    [[nodiscard]] const DirectX::XMFLOAT3& GetTargetPositionM() const noexcept;
    [[nodiscard]] float GetMoveSpeedMps() const noexcept;

private:
    DirectX::XMFLOAT3 mTargetPositionM = {0.0F, 0.0F, 0.0F};
    float mMoveSpeedMps = 0.0F;
    bool mHasTargetPosition = false;
};
} // namespace Kimgane::Engine
