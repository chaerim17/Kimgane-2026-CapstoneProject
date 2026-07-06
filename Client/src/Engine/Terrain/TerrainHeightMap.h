#pragma once

#include <DirectXCollision.h>
#include <DirectXMath.h>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

namespace Kimgane::Engine
{
class TerrainHeightMap final
{
public:
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
    [[nodiscard]] float HeightAt(std::uint32_t x, std::uint32_t z) const noexcept;
    [[nodiscard]] std::size_t IndexOf(std::uint32_t x, std::uint32_t z) const noexcept;

    std::uint32_t width_ = 2;
    std::uint32_t length_ = 2;
    float cellSpacingM_ = 1.0F;
    std::vector<float> heightsM_;
};
} // namespace Kimgane::Engine
