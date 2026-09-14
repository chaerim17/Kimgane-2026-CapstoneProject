#pragma once

#include "HeightMapData.h"

#include <memory>
#include <utility>

namespace Kimgane::Shared::Terrain
{
// Keeps the data alive while CollisionWorld uses its non-owning TerrainSurface.
class HeightMapSampler final : public Physics::TerrainSampler
{
public:
    explicit HeightMapSampler(std::shared_ptr<const HeightMapData> data, Physics::Vec3 positionM = {})
        : mData(std::move(data)), mPositionM(positionM) {}

    bool SampleHeightAtWorld(const Physics::Vec3& worldPositionM,
                             Physics::TerrainSample& sample) const noexcept override
    {
        if (!mData)
        {
            sample = {};
            return false;
        }
        const float x = worldPositionM.x - mPositionM.x + mData->GetWorldWidthM() * 0.5F;
        const float z = worldPositionM.z - mPositionM.z + mData->GetWorldLengthM() * 0.5F;
        if (!mData->ContainsSamplePositionM(x, z))
        {
            sample = {};
            return false;
        }
        sample.heightM = mData->SampleHeightM(x, z) + mPositionM.y;
        sample.normal = mData->SampleNormal(x, z);
        return true;
    }

private:
    std::shared_ptr<const HeightMapData> mData;
    Physics::Vec3 mPositionM;
};
} // namespace Kimgane::Shared::Terrain
