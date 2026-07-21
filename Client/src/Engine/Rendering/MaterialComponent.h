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
        : Component(owner), mMaterial(Material::CreateSolidColor(baseColorLinear))
    {
    }

    explicit MaterialComponent(GameObject& owner, std::shared_ptr<Material> material) noexcept
        : Component(owner), mMaterial(std::move(material))
    {
        if (!mMaterial)
        {
            mMaterial = Material::CreateSolidColor({1.0F, 1.0F, 1.0F, 1.0F});
        }
    }

    [[nodiscard]] const Material& GetMaterial() const noexcept
    {
        return *mMaterial;
    }

    [[nodiscard]] Material& GetMaterial() noexcept
    {
        return *mMaterial;
    }

    [[nodiscard]] const DirectX::XMFLOAT4& GetBaseColor() const noexcept
    {
        return mMaterial->GetBaseColorLinear();
    }

    void SetMaterial(std::shared_ptr<Material> material) noexcept
    {
        mMaterial = std::move(material);
        if (!mMaterial)
        {
            mMaterial = Material::CreateSolidColor({1.0F, 1.0F, 1.0F, 1.0F});
        }
    }

    void SetBaseColor(const DirectX::XMFLOAT4& baseColorLinear) noexcept
    {
        mMaterial->SetBaseColorLinear(baseColorLinear);
    }

private:
    std::shared_ptr<Material> mMaterial;
};
} // namespace Kimgane::Engine
