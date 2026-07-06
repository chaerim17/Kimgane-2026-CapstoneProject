#include "Pch.h"

#include "Material.h"

#include <algorithm>
#include <utility>

namespace Kimgane::Engine
{
std::shared_ptr<Material> Material::CreateSolidColor(const DirectX::XMFLOAT4& baseColorLinear)
{
    auto material = std::make_shared<Material>();
    material->SetBaseColorLinear(baseColorLinear);
    return material;
}

const std::string& Material::GetName() const noexcept
{
    return mName;
}

const DirectX::XMFLOAT4& Material::GetBaseColorLinear() const noexcept
{
    return mBaseColorLinear;
}

const DirectX::XMFLOAT4& Material::GetEmissionLinear() const noexcept
{
    return mEmissionLinear;
}

float Material::GetMetallic() const noexcept
{
    return mMetallic;
}

float Material::GetRoughness() const noexcept
{
    return mRoughness;
}

void Material::SetName(std::string name)
{
    mName = std::move(name);
}

void Material::SetBaseColorLinear(const DirectX::XMFLOAT4& baseColorLinear) noexcept
{
    mBaseColorLinear = baseColorLinear;
}

void Material::SetEmissionLinear(const DirectX::XMFLOAT3& colorLinear, float intensity) noexcept
{
    mEmissionLinear = {std::max(colorLinear.x, 0.0F),
                       std::max(colorLinear.y, 0.0F),
                       std::max(colorLinear.z, 0.0F),
                       std::max(intensity, 0.0F)};
}

void Material::SetSurface(float metallic, float roughness) noexcept
{
    mMetallic = std::clamp(metallic, 0.0F, 1.0F);
    mRoughness = std::clamp(roughness, 0.02F, 1.0F);
}
} // namespace Kimgane::Engine
