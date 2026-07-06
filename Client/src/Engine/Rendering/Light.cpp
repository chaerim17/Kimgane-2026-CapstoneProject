#include "Pch.h"

#include "Light.h"

#include <algorithm>

namespace Kimgane::Engine
{
namespace
{
DirectX::XMFLOAT3 NormalizeOrFallback(const DirectX::XMFLOAT3& value,
                                      const DirectX::XMFLOAT3& fallback) noexcept
{
    const DirectX::XMVECTOR vector = DirectX::XMLoadFloat3(&value);
    if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(vector)) <= 0.000001F)
    {
        return fallback;
    }

    DirectX::XMFLOAT3 result = {};
    DirectX::XMStoreFloat3(&result, DirectX::XMVector3Normalize(vector));
    return result;
}
} // namespace

DirectionalLight::DirectionalLight() noexcept
{
    SetDirection(direction_);
}

DirectionalLightShaderData DirectionalLight::BuildShaderData() const noexcept
{
    return {{direction_.x, direction_.y, direction_.z, intensity_},
            {colorLinear_.x, colorLinear_.y, colorLinear_.z, ambientStrength_}};
}

const DirectX::XMFLOAT3& DirectionalLight::GetDirection() const noexcept
{
    return direction_;
}

const DirectX::XMFLOAT3& DirectionalLight::GetColorLinear() const noexcept
{
    return colorLinear_;
}

float DirectionalLight::GetIntensity() const noexcept
{
    return intensity_;
}

float DirectionalLight::GetAmbientStrength() const noexcept
{
    return ambientStrength_;
}

void DirectionalLight::SetDirection(const DirectX::XMFLOAT3& direction) noexcept
{
    direction_ = NormalizeOrFallback(direction, {0.0F, -1.0F, 0.0F});
}

void DirectionalLight::SetColorLinear(const DirectX::XMFLOAT3& colorLinear) noexcept
{
    colorLinear_ = {std::max(colorLinear.x, 0.0F),
                    std::max(colorLinear.y, 0.0F),
                    std::max(colorLinear.z, 0.0F)};
}

void DirectionalLight::SetIntensity(float intensity) noexcept
{
    intensity_ = std::max(intensity, 0.0F);
}

void DirectionalLight::SetAmbientStrength(float ambientStrength) noexcept
{
    ambientStrength_ = std::clamp(ambientStrength, 0.0F, 1.0F);
}
} // namespace Kimgane::Engine
