#pragma once

#include "../../Shared/Terrain/HeightMapData.h"

#include <DirectXCollision.h>
#include <DirectXMath.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

namespace Kimgane::Client
{
class TerrainHeightMap final
{
public:
    explicit TerrainHeightMap(std::shared_ptr<const Kimgane::Shared::Terrain::HeightMapData> data);
    [[nodiscard]] const std::shared_ptr<const Kimgane::Shared::Terrain::HeightMapData>& GetSharedData() const noexcept;

    TerrainHeightMap(std::uint32_t width,
                     std::uint32_t length,
                     float cellSpacingM,
                     std::vector<float> heightsM);

    static std::shared_ptr<TerrainHeightMap> CreateFlat(std::uint32_t width,
                                                        std::uint32_t length,
                                                        float cellSpacingM,
                                                        float heightM = 0.0F);
    static std::shared_ptr<TerrainHeightMap> CreateWaveField(std::uint32_t width,
                                                             std::uint32_t length,
                                                             float cellSpacingM,
                                                             float amplitudeM,
                                                             float frequency);
    static std::shared_ptr<TerrainHeightMap> LoadRaw8(const std::filesystem::path& filePath,
                                                      std::uint32_t width,
                                                      std::uint32_t length,
                                                      float cellSpacingM,
                                                      float heightScaleM);
    static std::shared_ptr<TerrainHeightMap> LoadRaw16(const std::filesystem::path& filePath,
                                                       std::uint32_t width,
                                                       std::uint32_t length,
                                                       float cellSpacingM,
                                                       float heightScaleM);
    static std::shared_ptr<TerrainHeightMap> LoadRawAuto(const std::filesystem::path& filePath,
                                                         float cellSpacingM,
                                                         float heightScaleM);

    [[nodiscard]] std::uint32_t GetWidth() const noexcept;
    [[nodiscard]] std::uint32_t GetLength() const noexcept;
    [[nodiscard]] float GetCellSpacingM() const noexcept;
    [[nodiscard]] float GetWorldWidthM() const noexcept;
    [[nodiscard]] float GetWorldLengthM() const noexcept;
    [[nodiscard]] const std::vector<float>& GetHeightsM() const noexcept;

    [[nodiscard]] bool ContainsSamplePositionM(float sampleXM, float sampleZM) const noexcept;
    [[nodiscard]] float SampleHeightM(float sampleXM, float sampleZM) const noexcept;
    [[nodiscard]] DirectX::XMFLOAT3 SampleNormal(float sampleXM, float sampleZM) const noexcept;
    [[nodiscard]] DirectX::BoundingBox GetCenteredLocalAabb() const noexcept;

private:
    std::shared_ptr<const Kimgane::Shared::Terrain::HeightMapData> mData;
};
} // namespace Kimgane::Client
