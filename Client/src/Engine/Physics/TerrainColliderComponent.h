#pragma once

#include "ColliderComponent.h"
#include "../../Shared/Terrain/TerrainHeightMap.h"

#include <memory>

namespace Kimgane::Engine
{
class TerrainColliderComponent final : public ColliderComponent
{
public:
    TerrainColliderComponent(GameObject& owner, std::shared_ptr<const TerrainHeightMap> heightMap);

    void Update(float deltaTimeSec) override;

    [[nodiscard]] const std::shared_ptr<const TerrainHeightMap>& GetHeightMap() const noexcept;
    [[nodiscard]] const DirectX::BoundingBox& GetWorldAabb() const noexcept override;
    [[nodiscard]] bool GetHeightAtWorld(const DirectX::XMFLOAT3& worldPositionM,
                                        float& outHeightM,
                                        DirectX::XMFLOAT3& outNormal) const noexcept;
    [[nodiscard]] bool Raycast(const DirectX::XMFLOAT3& originM,
                               const DirectX::XMFLOAT3& direction,
                               float& outDistanceM) const noexcept override;

private:
    [[nodiscard]] DirectX::XMFLOAT3 TransformPointToLocal(const DirectX::XMFLOAT3& worldPositionM) const noexcept;
    [[nodiscard]] DirectX::XMFLOAT3 TransformPointToWorld(const DirectX::XMFLOAT3& localPositionM) const noexcept;
    [[nodiscard]] DirectX::XMFLOAT3 TransformNormalToWorld(const DirectX::XMFLOAT3& localNormal) const noexcept;

    std::shared_ptr<const TerrainHeightMap> mHeightMap;
    DirectX::XMFLOAT4X4 mWorldMatrix = {};
    DirectX::XMFLOAT4X4 mInverseWorldMatrix = {};
    DirectX::BoundingBox mWorldAabb = {};
};
} // namespace Kimgane::Engine
