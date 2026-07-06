#pragma once

#include "../Core/Component.h"
#include "Material.h"

#include <DirectXMath.h>

#include <memory>
#include <utility>

namespace Kimgane::Engine
{
class MaterialComponent final : public Component
{
public:
    explicit MaterialComponent(GameObject& owner,
                               const DirectX::XMFLOAT4& baseColorLinear = {1.0F, 1.0F, 1.0F, 1.0F})
        : Component(owner), material_(Material::CreateSolidColor(baseColorLinear))
    {
    }

    explicit MaterialComponent(GameObject& owner, std::shared_ptr<Material> material) noexcept
        : Component(owner), material_(std::move(material))
    {
        if (!material_)
        {
            material_ = Material::CreateSolidColor({1.0F, 1.0F, 1.0F, 1.0F});
        }
    }

    [[nodiscard]] const Material& GetMaterial() const noexcept
    {
        return *material_;
    }

    [[nodiscard]] Material& GetMaterial() noexcept
    {
        return *material_;
    }

    [[nodiscard]] const DirectX::XMFLOAT4& GetBaseColor() const noexcept
    {
        return material_->GetBaseColorLinear();
    }

    void SetMaterial(std::shared_ptr<Material> material) noexcept
    {
        material_ = std::move(material);
        if (!material_)
        {
            material_ = Material::CreateSolidColor({1.0F, 1.0F, 1.0F, 1.0F});
        }
    }

    void SetBaseColor(const DirectX::XMFLOAT4& baseColorLinear) noexcept
    {
        material_->SetBaseColorLinear(baseColorLinear);
    }

private:
    std::shared_ptr<Material> material_;
};
} // namespace Kimgane::Engine
