#pragma once

#include "../Core/Component.h"
#include "Light.h"

namespace Kimgane::Engine
{
class DirectionalLightComponent final : public Component
{
public:
    explicit DirectionalLightComponent(GameObject& owner) noexcept;

    [[nodiscard]] DirectionalLight& GetLight() noexcept;
    [[nodiscard]] const DirectionalLight& GetLight() const noexcept;

    void SetDirection(const DirectX::XMFLOAT3& direction) noexcept;
    void SetColorLinear(const DirectX::XMFLOAT3& colorLinear) noexcept;
    void SetIntensity(float intensity) noexcept;
    void SetAmbientStrength(float ambientStrength) noexcept;

private:
    DirectionalLight mLight;
};
} // namespace Kimgane::Engine
