#include "Pch.h"

#include "TerrainHeightMap.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>

namespace Kimgane::Engine
{
namespace
{
constexpr float kMinCellSpacingM = 0.001F;
constexpr float kMinDirectionLengthSq = 0.000001F;

std::uint32_t ClampSampleCount(std::uint32_t value) noexcept
{
    return std::max(value, 2U);
}

std::uint32_t InferSquareDimension(std::uint64_t sampleCount) noexcept
{
    const double root = std::sqrt(static_cast<double>(sampleCount));
    const auto dimension = static_cast<std::uint64_t>(root + 0.5);
    if (dimension >= 2U && dimension * dimension == sampleCount && dimension <= UINT32_MAX)
    {
        return static_cast<std::uint32_t>(dimension);
    }

    return 0U;
}

std::vector<unsigned char> ReadBinaryFile(const std::filesystem::path& filePath)
{
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file)
    {
        throw std::runtime_error("Failed to open terrain RAW file: " + filePath.string());
    }

    const std::streamoff fileSize = file.tellg();
    if (fileSize <= 0)
    {
        throw std::runtime_error("Terrain RAW file is empty: " + filePath.string());
    }

    std::vector<unsigned char> bytes(static_cast<std::size_t>(fileSize));
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!file)
    {
        throw std::runtime_error("Failed to read terrain RAW file: " + filePath.string());
    }

    return bytes;
}
} // namespace

TerrainHeightMap::TerrainHeightMap(std::uint32_t width,
                                   std::uint32_t length,
                                   float cellSpacingM,
                                   std::vector<float> heightsM)
    : width_(ClampSampleCount(width)),
      length_(ClampSampleCount(length)),
      cellSpacingM_(std::max(cellSpacingM, kMinCellSpacingM)),
      heightsM_(std::move(heightsM))
{
    const std::size_t expectedCount = static_cast<std::size_t>(width_) * length_;
    if (heightsM_.size() != expectedCount)
    {
        heightsM_.assign(expectedCount, 0.0F);
    }
}

std::shared_ptr<TerrainHeightMap> TerrainHeightMap::CreateFlat(std::uint32_t width,
                                                               std::uint32_t length,
                                                               float cellSpacingM,
                                                               float heightM)
{
    width = ClampSampleCount(width);
    length = ClampSampleCount(length);
    std::vector<float> heightsM(static_cast<std::size_t>(width) * length, heightM);
    return std::make_shared<TerrainHeightMap>(width, length, cellSpacingM, std::move(heightsM));
}

std::shared_ptr<TerrainHeightMap> TerrainHeightMap::CreateWaveField(std::uint32_t width,
                                                                    std::uint32_t length,
                                                                    float cellSpacingM,
                                                                    float amplitudeM,
                                                                    float frequency)
{
    width = ClampSampleCount(width);
    length = ClampSampleCount(length);
    amplitudeM = std::max(amplitudeM, 0.0F);
    frequency = std::max(frequency, 0.1F);

    std::vector<float> heightsM(static_cast<std::size_t>(width) * length);
    for (std::uint32_t z = 0; z < length; ++z)
    {
        const float nz = static_cast<float>(z) / static_cast<float>(length - 1U);
        for (std::uint32_t x = 0; x < width; ++x)
        {
            const float nx = static_cast<float>(x) / static_cast<float>(width - 1U);
            const float broadWave = std::sinf(nx * DirectX::XM_2PI * frequency) *
                                    std::cosf(nz * DirectX::XM_2PI * frequency * 0.75F);
            const float diagonalWave = std::sinf((nx + nz) * DirectX::XM_2PI * frequency * 0.4F);
            heightsM[static_cast<std::size_t>(z) * width + x] =
                (broadWave * 0.65F + diagonalWave * 0.35F) * amplitudeM;
        }
    }

    return std::make_shared<TerrainHeightMap>(width, length, cellSpacingM, std::move(heightsM));
}

std::shared_ptr<TerrainHeightMap> TerrainHeightMap::LoadRaw8(const std::filesystem::path& filePath,
                                                             std::uint32_t width,
                                                             std::uint32_t length,
                                                             float cellSpacingM,
                                                             float heightScaleM)
{
    width = ClampSampleCount(width);
    length = ClampSampleCount(length);
    const std::vector<unsigned char> bytes = ReadBinaryFile(filePath);
    const std::size_t expectedCount = static_cast<std::size_t>(width) * length;
    if (bytes.size() != expectedCount)
    {
        throw std::runtime_error("Terrain RAW8 byte count does not match width * length: " + filePath.string());
    }

    heightScaleM = std::max(heightScaleM, 0.0F);
    std::vector<float> heightsM(bytes.size());
    for (std::size_t index = 0; index < bytes.size(); ++index)
    {
        heightsM[index] = (static_cast<float>(bytes[index]) / 255.0F) * heightScaleM;
    }

    return std::make_shared<TerrainHeightMap>(width, length, cellSpacingM, std::move(heightsM));
}

std::shared_ptr<TerrainHeightMap> TerrainHeightMap::LoadRaw16(const std::filesystem::path& filePath,
                                                              std::uint32_t width,
                                                              std::uint32_t length,
                                                              float cellSpacingM,
                                                              float heightScaleM)
{
    width = ClampSampleCount(width);
    length = ClampSampleCount(length);
    const std::vector<unsigned char> bytes = ReadBinaryFile(filePath);
    const std::size_t expectedSampleCount = static_cast<std::size_t>(width) * length;
    if (bytes.size() != expectedSampleCount * 2U)
    {
        throw std::runtime_error("Terrain RAW16 byte count does not match width * length * 2: " + filePath.string());
    }

    heightScaleM = std::max(heightScaleM, 0.0F);
    std::vector<float> heightsM(expectedSampleCount);
    for (std::size_t index = 0; index < expectedSampleCount; ++index)
    {
        const auto low = static_cast<unsigned int>(bytes[index * 2U + 0U]);
        const auto high = static_cast<unsigned int>(bytes[index * 2U + 1U]);
        const unsigned int value = low | (high << 8U);
        heightsM[index] = (static_cast<float>(value) / 65535.0F) * heightScaleM;
    }

    return std::make_shared<TerrainHeightMap>(width, length, cellSpacingM, std::move(heightsM));
}

std::shared_ptr<TerrainHeightMap> TerrainHeightMap::LoadRawAuto(const std::filesystem::path& filePath,
                                                                float cellSpacingM,
                                                                float heightScaleM)
{
    const std::vector<unsigned char> bytes = ReadBinaryFile(filePath);
    const std::uint32_t dimension8 = InferSquareDimension(static_cast<std::uint64_t>(bytes.size()));
    const std::uint32_t dimension16 =
        (bytes.size() % 2U == 0U) ? InferSquareDimension(static_cast<std::uint64_t>(bytes.size() / 2U)) : 0U;

    if (dimension8 != 0U)
    {
        return LoadRaw8(filePath, dimension8, dimension8, cellSpacingM, heightScaleM);
    }

    if (dimension16 != 0U)
    {
        return LoadRaw16(filePath, dimension16, dimension16, cellSpacingM, heightScaleM);
    }

    throw std::runtime_error("Terrain RAW file must be square RAW8 or little-endian RAW16: " + filePath.string());
}

std::uint32_t TerrainHeightMap::GetWidth() const noexcept
{
    return width_;
}

std::uint32_t TerrainHeightMap::GetLength() const noexcept
{
    return length_;
}

float TerrainHeightMap::GetCellSpacingM() const noexcept
{
    return cellSpacingM_;
}

float TerrainHeightMap::GetWorldWidthM() const noexcept
{
    return static_cast<float>(width_ - 1U) * cellSpacingM_;
}

float TerrainHeightMap::GetWorldLengthM() const noexcept
{
    return static_cast<float>(length_ - 1U) * cellSpacingM_;
}

const std::vector<float>& TerrainHeightMap::GetHeightsM() const noexcept
{
    return heightsM_;
}

bool TerrainHeightMap::ContainsSamplePositionM(float sampleXM, float sampleZM) const noexcept
{
    return sampleXM >= 0.0F && sampleZM >= 0.0F && sampleXM <= GetWorldWidthM() && sampleZM <= GetWorldLengthM();
}

float TerrainHeightMap::SampleHeightM(float sampleXM, float sampleZM) const noexcept
{
    if (heightsM_.empty())
    {
        return 0.0F;
    }

    sampleXM = std::clamp(sampleXM, 0.0F, GetWorldWidthM());
    sampleZM = std::clamp(sampleZM, 0.0F, GetWorldLengthM());

    const float gridX = sampleXM / cellSpacingM_;
    const float gridZ = sampleZM / cellSpacingM_;
    const auto x0 = std::min(static_cast<std::uint32_t>(std::floor(gridX)), width_ - 1U);
    const auto z0 = std::min(static_cast<std::uint32_t>(std::floor(gridZ)), length_ - 1U);
    const std::uint32_t x1 = std::min(x0 + 1U, width_ - 1U);
    const std::uint32_t z1 = std::min(z0 + 1U, length_ - 1U);

    const float tx = gridX - static_cast<float>(x0);
    const float tz = gridZ - static_cast<float>(z0);
    const float h00 = HeightAt(x0, z0);
    const float h10 = HeightAt(x1, z0);
    const float h01 = HeightAt(x0, z1);
    const float h11 = HeightAt(x1, z1);
    const float h0 = h00 + (h10 - h00) * tx;
    const float h1 = h01 + (h11 - h01) * tx;
    return h0 + (h1 - h0) * tz;
}

DirectX::XMFLOAT3 TerrainHeightMap::SampleNormal(float sampleXM, float sampleZM) const noexcept
{
    const float leftHeightM = SampleHeightM(sampleXM - cellSpacingM_, sampleZM);
    const float rightHeightM = SampleHeightM(sampleXM + cellSpacingM_, sampleZM);
    const float backHeightM = SampleHeightM(sampleXM, sampleZM - cellSpacingM_);
    const float forwardHeightM = SampleHeightM(sampleXM, sampleZM + cellSpacingM_);
    const DirectX::XMFLOAT3 normal = {leftHeightM - rightHeightM,
                                      2.0F * cellSpacingM_,
                                      backHeightM - forwardHeightM};
    const DirectX::XMVECTOR normalVector = DirectX::XMLoadFloat3(&normal);
    if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(normalVector)) <= kMinDirectionLengthSq)
    {
        return {0.0F, 1.0F, 0.0F};
    }

    DirectX::XMFLOAT3 result = {};
    DirectX::XMStoreFloat3(&result, DirectX::XMVector3Normalize(normalVector));
    return result;
}

DirectX::BoundingBox TerrainHeightMap::GetCenteredLocalAabb() const noexcept
{
    float minHeightM = 0.0F;
    float maxHeightM = 0.0F;
    if (!heightsM_.empty())
    {
        const auto [minIt, maxIt] = std::minmax_element(heightsM_.begin(), heightsM_.end());
        minHeightM = *minIt;
        maxHeightM = *maxIt;
    }

    DirectX::BoundingBox bounds = {};
    bounds.Center = {0.0F, (minHeightM + maxHeightM) * 0.5F, 0.0F};
    bounds.Extents = {GetWorldWidthM() * 0.5F,
                      std::max((maxHeightM - minHeightM) * 0.5F, 0.001F),
                      GetWorldLengthM() * 0.5F};
    return bounds;
}

float TerrainHeightMap::HeightAt(std::uint32_t x, std::uint32_t z) const noexcept
{
    return heightsM_[IndexOf(std::min(x, width_ - 1U), std::min(z, length_ - 1U))];
}

std::size_t TerrainHeightMap::IndexOf(std::uint32_t x, std::uint32_t z) const noexcept
{
    return static_cast<std::size_t>(z) * width_ + x;
}
} // namespace Kimgane::Engine
