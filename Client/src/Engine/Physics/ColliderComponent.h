#pragma once

#include "../Core/Component.h"

#include <DirectXCollision.h>
#include <DirectXMath.h>

namespace Kimgane::Engine
{
enum class ColliderType
{
    Box,
    Capsule,
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
    ColliderType mType;
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

    DirectX::BoundingOrientedBox mLocalBox = {};
    DirectX::BoundingOrientedBox mWorldBox = {};
    DirectX::BoundingBox mWorldAabb = {};
};

class CapsuleColliderComponent final : public ColliderComponent
{
public:
    CapsuleColliderComponent(GameObject& owner,
                             const DirectX::XMFLOAT3& centerM,
                             float radiusM,
                             float heightM);

    void Update(float deltaTimeSec) override;

    [[nodiscard]] const DirectX::XMFLOAT3& GetWorldSegmentStartM() const noexcept;
    [[nodiscard]] const DirectX::XMFLOAT3& GetWorldSegmentEndM() const noexcept;
    [[nodiscard]] const DirectX::XMFLOAT3& GetWorldCenterM() const noexcept;
    [[nodiscard]] float GetWorldRadiusM() const noexcept;
    [[nodiscard]] float GetWorldBottomM() const noexcept;
    [[nodiscard]] const DirectX::BoundingBox& GetWorldAabb() const noexcept override;
    [[nodiscard]] bool Raycast(const DirectX::XMFLOAT3& originM,
                               const DirectX::XMFLOAT3& direction,
                               float& outDistanceM) const noexcept override;

private:
    [[nodiscard]] static float Max(float lhs, float rhs) noexcept;

    DirectX::XMFLOAT3 mLocalCenterM = {0.0F, 0.0F, 0.0F};
    float mLocalRadiusM = 0.0F;
    float mLocalHeightM = 0.0F;
    DirectX::XMFLOAT3 mWorldSegmentStartM = {0.0F, 0.0F, 0.0F};
    DirectX::XMFLOAT3 mWorldSegmentEndM = {0.0F, 0.0F, 0.0F};
    DirectX::XMFLOAT3 mWorldCenterM = {0.0F, 0.0F, 0.0F};
    float mWorldRadiusM = 0.0F;
    DirectX::BoundingBox mWorldAabb = {};
};
} // namespace Kimgane::Engine
