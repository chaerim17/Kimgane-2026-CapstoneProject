#pragma once

#include <DirectXMath.h>

namespace Kimgane::Engine
{
struct DirectionalLightShaderData
{
    DirectX::XMFLOAT4 directionIntensity = {0.35F, -1.0F, 0.25F, 1.0F};
    DirectX::XMFLOAT4 colorAmbient = {1.0F, 0.96F, 0.86F, 0.18F};
};

class DirectionalLight final
{
public:
    DirectionalLight() noexcept;

    [[nodiscard]] DirectionalLightShaderData BuildShaderData() const noexcept;
    [[nodiscard]] const DirectX::XMFLOAT3& GetDirection() const noexcept;
    [[nodiscard]] const DirectX::XMFLOAT3& GetColorLinear() const noexcept;
    [[nodiscard]] float GetIntensity() const noexcept;
    [[nodiscard]] float GetAmbientStrength() const noexcept;

    void SetDirection(const DirectX::XMFLOAT3& direction) noexcept;
    void SetColorLinear(const DirectX::XMFLOAT3& colorLinear) noexcept;
    void SetIntensity(float intensity) noexcept;
    void SetAmbientStrength(float ambientStrength) noexcept;

private:
    DirectX::XMFLOAT3 direction_ = {0.35F, -1.0F, 0.25F};
    DirectX::XMFLOAT3 colorLinear_ = {1.0F, 0.96F, 0.86F};
    float intensity_ = 1.0F;
    float ambientStrength_ = 0.18F;
};
} // namespace Kimgane::Engine
