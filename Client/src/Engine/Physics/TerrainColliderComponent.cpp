#include "Pch.h"

#include "TerrainColliderComponent.h"

#include "../Core/GameObject.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

namespace Kimgane::Engine
{
namespace
{
constexpr float MIN_RAY_DIRECTION_LENGTH_SQ = 0.000001F;
constexpr float MAX_TERRAIN_RAY_DISTANCE_M = 1000.0F;
constexpr float MIN_TERRAIN_RAY_STEP_M = 0.1F;

DirectX::XMFLOAT3 NormalizeOrUp(const DirectX::XMFLOAT3& value) noexcept
{
    const DirectX::XMVECTOR vector = DirectX::XMLoadFloat3(&value);
    if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(vector)) <= MIN_RAY_DIRECTION_LENGTH_SQ)
    {
        return {0.0F, 1.0F, 0.0F};
    }

    DirectX::XMFLOAT3 result = {};
    DirectX::XMStoreFloat3(&result, DirectX::XMVector3Normalize(vector));
    return result;
}
} // namespace

TerrainColliderComponent::TerrainColliderComponent(GameObject& owner, std::shared_ptr<const TerrainHeightMap> heightMap)
    : ColliderComponent(owner, ColliderType::Terrain), mHeightMap(std::move(heightMap))
{
    Update(0.0F);
}

void TerrainColliderComponent::Update(float deltaTimeSec)
{
    (void)deltaTimeSec;

    const DirectX::XMMATRIX worldMatrix = GetOwner().GetTransform().GetWorldMatrix();
    DirectX::XMStoreFloat4x4(&mWorldMatrix, worldMatrix);
    DirectX::XMStoreFloat4x4(&mInverseWorldMatrix, DirectX::XMMatrixInverse(nullptr, worldMatrix));

    if (!mHeightMap)
    {
        mWorldAabb = {};
        return;
    }

    const DirectX::BoundingBox localAabb = mHeightMap->GetCenteredLocalAabb();
    localAabb.Transform(mWorldAabb, worldMatrix);
}

const std::shared_ptr<const TerrainHeightMap>& TerrainColliderComponent::GetHeightMap() const noexcept
{
    return mHeightMap;
}

const DirectX::BoundingBox& TerrainColliderComponent::GetWorldAabb() const noexcept
{
    return mWorldAabb;
}

bool TerrainColliderComponent::GetHeightAtWorld(const DirectX::XMFLOAT3& worldPositionM,
                                                float& outHeightM,
                                                DirectX::XMFLOAT3& outNormal) const noexcept
{
    if (!mHeightMap)
    {
        outHeightM = 0.0F;
        outNormal = {0.0F, 1.0F, 0.0F};
        return false;
    }

    const DirectX::XMFLOAT3 localPositionM = TransformPointToLocal(worldPositionM);
    const float sampleXM = localPositionM.x + mHeightMap->GetWorldWidthM() * 0.5F;
    const float sampleZM = localPositionM.z + mHeightMap->GetWorldLengthM() * 0.5F;
    if (!mHeightMap->ContainsSamplePositionM(sampleXM, sampleZM))
    {
        outHeightM = 0.0F;
        outNormal = {0.0F, 1.0F, 0.0F};
        return false;
    }

    const float localHeightM = mHeightMap->SampleHeightM(sampleXM, sampleZM);
    const DirectX::XMFLOAT3 worldHeightPositionM =
        TransformPointToWorld({localPositionM.x, localHeightM, localPositionM.z});
    outHeightM = worldHeightPositionM.y;
    outNormal = TransformNormalToWorld(mHeightMap->SampleNormal(sampleXM, sampleZM));
    return true;
}

bool TerrainColliderComponent::Raycast(const DirectX::XMFLOAT3& originM,
                                       const DirectX::XMFLOAT3& direction,
                                       float& outDistanceM) const noexcept
{
    if (!mHeightMap)
    {
        outDistanceM = FLT_MAX;
        return false;
    }

    DirectX::XMVECTOR rayDirection = DirectX::XMLoadFloat3(&direction);
    if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(rayDirection)) <= MIN_RAY_DIRECTION_LENGTH_SQ)
    {
        outDistanceM = FLT_MAX;
        return false;
    }

    rayDirection = DirectX::XMVector3Normalize(rayDirection);
    const float stepM = std::max(mHeightMap->GetCellSpacingM() * 0.5F, MIN_TERRAIN_RAY_STEP_M);
    float previousDistanceM = 0.0F;
    float previousHeightM = 0.0F;
    DirectX::XMFLOAT3 previousNormal = {0.0F, 1.0F, 0.0F};
    bool hasPreviousSample = false;

    for (float distanceM = 0.0F; distanceM <= MAX_TERRAIN_RAY_DISTANCE_M; distanceM += stepM)
    {
        const DirectX::XMVECTOR sampleVector =
            DirectX::XMVectorAdd(DirectX::XMLoadFloat3(&originM), DirectX::XMVectorScale(rayDirection, distanceM));
        DirectX::XMFLOAT3 sampleM = {};
        DirectX::XMStoreFloat3(&sampleM, sampleVector);

        float terrainHeightM = 0.0F;
        DirectX::XMFLOAT3 terrainNormal = {};
        if (!GetHeightAtWorld(sampleM, terrainHeightM, terrainNormal))
        {
            continue;
        }

        if (sampleM.y <= terrainHeightM)
        {
            outDistanceM = hasPreviousSample ? (previousDistanceM + distanceM) * 0.5F : distanceM;
            return true;
        }

        previousDistanceM = distanceM;
        previousHeightM = terrainHeightM;
        previousNormal = terrainNormal;
        hasPreviousSample = true;
    }

    (void)previousHeightM;
    (void)previousNormal;
    outDistanceM = FLT_MAX;
    return false;
}

DirectX::XMFLOAT3 TerrainColliderComponent::TransformPointToLocal(const DirectX::XMFLOAT3& worldPositionM) const noexcept
{
    DirectX::XMFLOAT3 localPositionM = {};
    DirectX::XMStoreFloat3(&localPositionM,
                           DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&worldPositionM),
                                                            DirectX::XMLoadFloat4x4(&mInverseWorldMatrix)));
    return localPositionM;
}

DirectX::XMFLOAT3 TerrainColliderComponent::TransformPointToWorld(const DirectX::XMFLOAT3& localPositionM) const noexcept
{
    DirectX::XMFLOAT3 worldPositionM = {};
    DirectX::XMStoreFloat3(&worldPositionM,
                           DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&localPositionM),
                                                            DirectX::XMLoadFloat4x4(&mWorldMatrix)));
    return worldPositionM;
}

DirectX::XMFLOAT3 TerrainColliderComponent::TransformNormalToWorld(const DirectX::XMFLOAT3& localNormal) const noexcept
{
    DirectX::XMFLOAT3 worldNormal = {};
    DirectX::XMStoreFloat3(&worldNormal,
                           DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&localNormal),
                                                             DirectX::XMLoadFloat4x4(&mWorldMatrix)));
    return NormalizeOrUp(worldNormal);
}
} // namespace Kimgane::Engine
