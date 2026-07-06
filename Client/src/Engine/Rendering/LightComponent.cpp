#include "Pch.h"

#include "LightComponent.h"

namespace Kimgane::Engine
{
DirectionalLightComponent::DirectionalLightComponent(GameObject& owner) noexcept
    : Component(owner)
{
}

DirectionalLight& DirectionalLightComponent::GetLight() noexcept
{
    return mLight;
}

const DirectionalLight& DirectionalLightComponent::GetLight() const noexcept
{
    return mLight;
}

void DirectionalLightComponent::SetDirection(const DirectX::XMFLOAT3& direction) noexcept
{
    mLight.SetDirection(direction);
}

void DirectionalLightComponent::SetColorLinear(const DirectX::XMFLOAT3& colorLinear) noexcept
{
    mLight.SetColorLinear(colorLinear);
}

void DirectionalLightComponent::SetIntensity(float intensity) noexcept
{
    mLight.SetIntensity(intensity);
}

void DirectionalLightComponent::SetAmbientStrength(float ambientStrength) noexcept
{
    mLight.SetAmbientStrength(ambientStrength);
}
} // namespace Kimgane::Engine
