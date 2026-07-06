#pragma once

#include <DirectXMath.h>

#include <memory>
#include <string>

namespace Kimgane::Engine
{
class Material final
{
public:
    static std::shared_ptr<Material> CreateSolidColor(const DirectX::XMFLOAT4& baseColorLinear);

    Material() = default;

    [[nodiscard]] const std::string& GetName() const noexcept;
    [[nodiscard]] const DirectX::XMFLOAT4& GetBaseColorLinear() const noexcept;
    [[nodiscard]] float GetMetallic() const noexcept;
    [[nodiscard]] float GetRoughness() const noexcept;

    void SetName(std::string name);
    void SetBaseColorLinear(const DirectX::XMFLOAT4& baseColorLinear) noexcept;
    void SetSurface(float metallic, float roughness) noexcept;

private:
    std::string name_;
    DirectX::XMFLOAT4 baseColorLinear_ = {1.0F, 1.0F, 1.0F, 1.0F};
    float metallic_ = 0.0F;
    float roughness_ = 0.5F;
};
} // namespace Kimgane::Engine
