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
    return name_;
}

const DirectX::XMFLOAT4& Material::GetBaseColorLinear() const noexcept
{
    return baseColorLinear_;
}

float Material::GetMetallic() const noexcept
{
    return metallic_;
}

float Material::GetRoughness() const noexcept
{
    return roughness_;
}

void Material::SetName(std::string name)
{
    name_ = std::move(name);
}

void Material::SetBaseColorLinear(const DirectX::XMFLOAT4& baseColorLinear) noexcept
{
    baseColorLinear_ = baseColorLinear;
}

void Material::SetSurface(float metallic, float roughness) noexcept
{
    metallic_ = std::clamp(metallic, 0.0F, 1.0F);
    roughness_ = std::clamp(roughness, 0.02F, 1.0F);
}
} // namespace Kimgane::Engine
