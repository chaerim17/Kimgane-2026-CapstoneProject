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
    SetDirection(mDirection);
}

DirectionalLightShaderData DirectionalLight::BuildShaderData() const noexcept
{
    return {{mDirection.x, mDirection.y, mDirection.z, mIntensity},
            {mColorLinear.x, mColorLinear.y, mColorLinear.z, mAmbientStrength}};
}

const DirectX::XMFLOAT3& DirectionalLight::GetDirection() const noexcept
{
    return mDirection;
}

const DirectX::XMFLOAT3& DirectionalLight::GetColorLinear() const noexcept
{
    return mColorLinear;
}

float DirectionalLight::GetIntensity() const noexcept
{
    return mIntensity;
}

float DirectionalLight::GetAmbientStrength() const noexcept
{
    return mAmbientStrength;
}

void DirectionalLight::SetDirection(const DirectX::XMFLOAT3& direction) noexcept
{
    mDirection = NormalizeOrFallback(direction, {0.0F, -1.0F, 0.0F});
}

void DirectionalLight::SetColorLinear(const DirectX::XMFLOAT3& colorLinear) noexcept
{
    mColorLinear = {std::max(colorLinear.x, 0.0F),
                    std::max(colorLinear.y, 0.0F),
                    std::max(colorLinear.z, 0.0F)};
}

void DirectionalLight::SetIntensity(float intensity) noexcept
{
    mIntensity = std::max(intensity, 0.0F);
}

void DirectionalLight::SetAmbientStrength(float ambientStrength) noexcept
{
    mAmbientStrength = std::clamp(ambientStrength, 0.0F, 1.0F);
}
} // namespace Kimgane::Engine
