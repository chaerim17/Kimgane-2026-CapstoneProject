#pragma once

#include "../Core/Component.h"

#include <DirectXMath.h>

namespace Kimgane::Engine
{
class MaterialComponent final : public Component
{
public:
    explicit MaterialComponent(GameObject& owner,
                               const DirectX::XMFLOAT4& baseColor = {1.0F, 1.0F, 1.0F, 1.0F}) noexcept
        : Component(owner), baseColor_(baseColor)
    {
    }

    [[nodiscard]] const DirectX::XMFLOAT4& GetBaseColor() const noexcept
    {
        return baseColor_;
    }

    void SetBaseColor(const DirectX::XMFLOAT4& baseColor) noexcept
    {
        baseColor_ = baseColor;
    }

private:
    DirectX::XMFLOAT4 baseColor_;
};
} // namespace Kimgane::Engine
