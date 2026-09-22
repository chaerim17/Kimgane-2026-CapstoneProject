#include "TerrainHeightMap.h"

#include <stdexcept>
#include <utility>

namespace Kimgane::Engine
{
using HeightMapData = Kimgane::Shared::Terrain::HeightMapData;

TerrainHeightMap::TerrainHeightMap(std::shared_ptr<const HeightMapData> data) : mData(std::move(data))
{
    if (!mData)
    {
        throw std::invalid_argument("TerrainHeightMap requires shared height data");
    }
}

TerrainHeightMap::TerrainHeightMap(std::uint32_t width, std::uint32_t length, float cellSpacingM,
                                 std::vector<float> heightsM)
    : TerrainHeightMap(std::make_shared<HeightMapData>(width, length, cellSpacingM, std::move(heightsM)))
{
}

const std::shared_ptr<const HeightMapData>& TerrainHeightMap::GetSharedData() const noexcept
{
    return mData;
}

std::shared_ptr<TerrainHeightMap> TerrainHeightMap::CreateFlat(std::uint32_t width, std::uint32_t length, float cellSpacingM, float heightM)
{
    return std::make_shared<TerrainHeightMap>(HeightMapData::CreateFlat(width, length, cellSpacingM, heightM));
}

std::shared_ptr<TerrainHeightMap> TerrainHeightMap::CreateWaveField(std::uint32_t width, std::uint32_t length, float cellSpacingM, float amplitudeM, float frequency)
{
    return std::make_shared<TerrainHeightMap>(HeightMapData::CreateWaveField(width, length, cellSpacingM, amplitudeM, frequency));
}

std::shared_ptr<TerrainHeightMap> TerrainHeightMap::LoadRaw8(const std::filesystem::path& filePath, std::uint32_t width, std::uint32_t length, float cellSpacingM, float heightScaleM)
{
    return std::make_shared<TerrainHeightMap>(HeightMapData::LoadRaw8(filePath, width, length, cellSpacingM, heightScaleM));
}

std::shared_ptr<TerrainHeightMap> TerrainHeightMap::LoadRaw16(const std::filesystem::path& filePath, std::uint32_t width, std::uint32_t length, float cellSpacingM, float heightScaleM)
{
    return std::make_shared<TerrainHeightMap>(HeightMapData::LoadRaw16(filePath, width, length, cellSpacingM, heightScaleM));
}

std::shared_ptr<TerrainHeightMap> TerrainHeightMap::LoadRawAuto(const std::filesystem::path& filePath, float cellSpacingM, float heightScaleM)
{
    return std::make_shared<TerrainHeightMap>(HeightMapData::LoadRawAuto(filePath, cellSpacingM, heightScaleM));
}

std::uint32_t TerrainHeightMap::GetWidth() const noexcept
{
    return mData->GetWidth();
}

std::uint32_t TerrainHeightMap::GetLength() const noexcept
{
    return mData->GetLength();
}

float TerrainHeightMap::GetCellSpacingM() const noexcept
{
    return mData->GetCellSpacingM();
}

float TerrainHeightMap::GetWorldWidthM() const noexcept
{
    return mData->GetWorldWidthM();
}

float TerrainHeightMap::GetWorldLengthM() const noexcept
{
    return mData->GetWorldLengthM();
}

const std::vector<float>& TerrainHeightMap::GetHeightsM() const noexcept
{
    return mData->GetHeightsM();
}

bool TerrainHeightMap::ContainsSamplePositionM(float sampleXM, float sampleZM) const noexcept
{
    return mData->ContainsSamplePositionM(sampleXM, sampleZM);
}

float TerrainHeightMap::SampleHeightM(float sampleXM, float sampleZM) const noexcept
{
    return mData->SampleHeightM(sampleXM, sampleZM);
}

DirectX::XMFLOAT3 TerrainHeightMap::SampleNormal(float sampleXM, float sampleZM) const noexcept
{
    const auto normal = mData->SampleNormal(sampleXM, sampleZM);
    return {normal.x, normal.y, normal.z};
}

DirectX::BoundingBox TerrainHeightMap::GetCenteredLocalAabb() const noexcept
{
    const auto box = mData->GetCenteredLocalBox();
    return {{box.centerM.x, box.centerM.y, box.centerM.z},
            {box.halfExtentsM.x, box.halfExtentsM.y, box.halfExtentsM.z}};
}
} // namespace Kimgane::Engine
