#include "HeightMapData.h"

#include "../IO/AssetPathResolver.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace Kimgane::Shared::Terrain
{
namespace
{
constexpr float MIN_CELL_SPACING_M = 0.001F;
constexpr float MIN_DIRECTION_LENGTH_SQ = 0.000001F;
constexpr float TWO_PI = 6.28318530718F;

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
    const std::filesystem::path resolvedPath = Kimgane::Shared::IO::ResolveAssetPath(filePath);
    if (resolvedPath.empty())
    {
        throw std::runtime_error("Failed to open terrain RAW file: " + filePath.string());
    }

    std::ifstream file(resolvedPath, std::ios::binary | std::ios::ate);
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

HeightMapData::HeightMapData(std::uint32_t width,
                                   std::uint32_t length,
                                   float cellSpacingM,
                                   std::vector<float> heightsM)
    : mWidth(ClampSampleCount(width)),
      mLength(ClampSampleCount(length)),
      mCellSpacingM(std::max(cellSpacingM, MIN_CELL_SPACING_M)),
      mHeightsM(std::move(heightsM))
{
    const std::size_t expectedCount = static_cast<std::size_t>(mWidth) * mLength;
    if (mHeightsM.size() != expectedCount)
    {
        mHeightsM.assign(expectedCount, 0.0F);
    }
}

std::shared_ptr<HeightMapData> HeightMapData::CreateFlat(std::uint32_t width,
                                                               std::uint32_t length,
                                                               float cellSpacingM,
                                                               float heightM)
{
    width = ClampSampleCount(width);
    length = ClampSampleCount(length);
    std::vector<float> heightsM(static_cast<std::size_t>(width) * length, heightM);
    return std::make_shared<HeightMapData>(width, length, cellSpacingM, std::move(heightsM));
}

std::shared_ptr<HeightMapData> HeightMapData::CreateWaveField(std::uint32_t width,
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
            const float broadWave = std::sin(nx * TWO_PI * frequency) *
                                    std::cos(nz * TWO_PI * frequency * 0.75F);
            const float diagonalWave = std::sin((nx + nz) * TWO_PI * frequency * 0.4F);
            heightsM[static_cast<std::size_t>(z) * width + x] =
                (broadWave * 0.65F + diagonalWave * 0.35F) * amplitudeM;
        }
    }

    return std::make_shared<HeightMapData>(width, length, cellSpacingM, std::move(heightsM));
}

std::shared_ptr<HeightMapData> HeightMapData::LoadRaw8(const std::filesystem::path& filePath,
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

    return std::make_shared<HeightMapData>(width, length, cellSpacingM, std::move(heightsM));
}

std::shared_ptr<HeightMapData> HeightMapData::LoadRaw16(const std::filesystem::path& filePath,
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

    return std::make_shared<HeightMapData>(width, length, cellSpacingM, std::move(heightsM));
}

std::shared_ptr<HeightMapData> HeightMapData::LoadRawAuto(const std::filesystem::path& filePath,
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

std::uint32_t HeightMapData::GetWidth() const noexcept
{
    return mWidth;
}

std::uint32_t HeightMapData::GetLength() const noexcept
{
    return mLength;
}

float HeightMapData::GetCellSpacingM() const noexcept
{
    return mCellSpacingM;
}

float HeightMapData::GetWorldWidthM() const noexcept
{
    return static_cast<float>(mWidth - 1U) * mCellSpacingM;
}

float HeightMapData::GetWorldLengthM() const noexcept
{
    return static_cast<float>(mLength - 1U) * mCellSpacingM;
}

const std::vector<float>& HeightMapData::GetHeightsM() const noexcept
{
    return mHeightsM;
}

bool HeightMapData::ContainsSamplePositionM(float sampleXM, float sampleZM) const noexcept
{
    return sampleXM >= 0.0F && sampleZM >= 0.0F && sampleXM <= GetWorldWidthM() && sampleZM <= GetWorldLengthM();
}

float HeightMapData::SampleHeightM(float sampleXM, float sampleZM) const noexcept
{
    if (mHeightsM.empty())
    {
        return 0.0F;
    }

    sampleXM = std::clamp(sampleXM, 0.0F, GetWorldWidthM());
    sampleZM = std::clamp(sampleZM, 0.0F, GetWorldLengthM());

    const float gridX = sampleXM / mCellSpacingM;
    const float gridZ = sampleZM / mCellSpacingM;
    const auto x0 = std::min(static_cast<std::uint32_t>(std::floor(gridX)), mWidth - 1U);
    const auto z0 = std::min(static_cast<std::uint32_t>(std::floor(gridZ)), mLength - 1U);
    const std::uint32_t x1 = std::min(x0 + 1U, mWidth - 1U);
    const std::uint32_t z1 = std::min(z0 + 1U, mLength - 1U);

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

Physics::Vec3 HeightMapData::SampleNormal(float sampleXM, float sampleZM) const noexcept
{
    const Physics::Vec3 normal = {
        SampleHeightM(sampleXM - mCellSpacingM, sampleZM) - SampleHeightM(sampleXM + mCellSpacingM, sampleZM),
        2.0F * mCellSpacingM,
        SampleHeightM(sampleXM, sampleZM - mCellSpacingM) - SampleHeightM(sampleXM, sampleZM + mCellSpacingM)};
    const float lengthSq = Physics::LengthSquared(normal);
    return lengthSq > MIN_DIRECTION_LENGTH_SQ
        ? Physics::Scale(normal, 1.0F / std::sqrt(lengthSq)) : Physics::Vec3{0.0F, 1.0F, 0.0F};
}

Physics::Box HeightMapData::GetCenteredLocalBox() const noexcept
{
    const auto [minIt, maxIt] = std::minmax_element(mHeightsM.begin(), mHeightsM.end());
    const float minHeightM = mHeightsM.empty() ? 0.0F : *minIt;
    const float maxHeightM = mHeightsM.empty() ? 0.0F : *maxIt;
    return {{0.0F, (minHeightM + maxHeightM) * 0.5F, 0.0F},
            {GetWorldWidthM() * 0.5F, std::max((maxHeightM - minHeightM) * 0.5F, 0.001F),
             GetWorldLengthM() * 0.5F}};
}

float HeightMapData::HeightAt(std::uint32_t x, std::uint32_t z) const noexcept
{
    return mHeightsM[IndexOf(std::min(x, mWidth - 1U), std::min(z, mLength - 1U))];
}

std::size_t HeightMapData::IndexOf(std::uint32_t x, std::uint32_t z) const noexcept
{
    return static_cast<std::size_t>(z) * mWidth + x;
}
} // namespace Kimgane::Shared::Terrain
