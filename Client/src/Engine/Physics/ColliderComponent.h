#pragma once

#include "../Core/Component.h"

#include <DirectXCollision.h>
#include <DirectXMath.h>

namespace Kimgane::Engine
{
enum class ColliderType
{
    Box,
    Terrain
};

class ColliderComponent : public Component
{
public:
    ColliderComponent(GameObject& owner, ColliderType type) noexcept;
    ~ColliderComponent() override = default;

    [[nodiscard]] ColliderType GetType() const noexcept;
    [[nodiscard]] virtual const DirectX::BoundingBox& GetWorldAabb() const noexcept = 0;
    [[nodiscard]] virtual bool Raycast(const DirectX::XMFLOAT3& originM,
                                       const DirectX::XMFLOAT3& direction,
                                       float& outDistanceM) const noexcept = 0;

private:
    ColliderType type_;
};

class BoxColliderComponent final : public ColliderComponent
{
public:
    BoxColliderComponent(GameObject& owner, const DirectX::XMFLOAT3& centerM, const DirectX::XMFLOAT3& sizeM);

    void Update(float deltaTimeSec) override;

    [[nodiscard]] const DirectX::BoundingOrientedBox& GetWorldBox() const noexcept;
    [[nodiscard]] const DirectX::BoundingBox& GetWorldAabb() const noexcept override;
    [[nodiscard]] bool Raycast(const DirectX::XMFLOAT3& originM, const DirectX::XMFLOAT3& direction,
                               float& outDistanceM) const noexcept override;

private:
    [[nodiscard]] static float Max(float lhs, float rhs) noexcept;

    DirectX::BoundingOrientedBox localBox_ = {};
    DirectX::BoundingOrientedBox worldBox_ = {};
    DirectX::BoundingBox worldAabb_ = {};
};
} // namespace Kimgane::Engine
