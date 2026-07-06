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
    [[nodiscard]] const DirectX::XMFLOAT4& GetEmissionLinear() const noexcept;
    [[nodiscard]] float GetMetallic() const noexcept;
    [[nodiscard]] float GetRoughness() const noexcept;

    void SetName(std::string name);
    void SetBaseColorLinear(const DirectX::XMFLOAT4& baseColorLinear) noexcept;
    void SetEmissionLinear(const DirectX::XMFLOAT3& colorLinear, float intensity) noexcept;
    void SetSurface(float metallic, float roughness) noexcept;

private:
    std::string mName;
    DirectX::XMFLOAT4 mBaseColorLinear = {1.0F, 1.0F, 1.0F, 1.0F};
    DirectX::XMFLOAT4 mEmissionLinear = {0.0F, 0.0F, 0.0F, 0.0F};
    float mMetallic = 0.0F;
    float mRoughness = 0.5F;
};
} // namespace Kimgane::Engine
