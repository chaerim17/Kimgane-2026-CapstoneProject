#pragma once

#include "../Physics/CollisionTypes.h"
#include "../IO/AssetPathResolver.h"

#include <array>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>

namespace Kimgane::Shared::World
{
struct EnvironmentPlacement
{
    std::string id;
    std::string meshPrefix;
    Physics::Vec3 positionM;
    float yawDeg = 0;
};

// Same thresholds as catalog.json. A 2m hysteresis prevents boundary flickering.
[[nodiscard]] inline unsigned int SelectEnvironmentLod(float distanceM, unsigned int currentLod) noexcept
{
    constexpr std::array<float, 3> DISTANCES_M = {22.0F, 55.0F, 110.0F};
    currentLod = std::min(currentLod, 3U);
    while (currentLod < 3U && distanceM > DISTANCES_M[currentLod] + 2.0F) ++currentLod;
    while (currentLod > 0U && distanceM < DISTANCES_M[currentLod - 1U] - 2.0F) --currentLod;
    return currentLod;
}

[[nodiscard]] inline std::vector<EnvironmentPlacement> LoadEnvironmentLayout(const std::filesystem::path& path)
{
    const auto resolved = IO::ResolveAssetPath(path);
    std::ifstream input(resolved);
    std::string magic;
    int version = 0;
    if (!(input >> magic >> version) || magic != "lunar_layout" || version != 2)
        throw std::runtime_error("Invalid lunar layout header: " + path.string());
    std::vector<EnvironmentPlacement> result;
    std::set<std::string> ids;
    std::string line;
    std::getline(input, line);
    while (std::getline(input, line))
    {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream fields(line);
        EnvironmentPlacement item;
        std::string keyword, trailing;
        if (!(fields >> keyword >> item.id >> item.meshPrefix >> item.positionM.x >> item.positionM.y
                     >> item.positionM.z >> item.yawDeg) || keyword != "instance" || (fields >> trailing)
            || !std::isfinite(item.positionM.x) || !std::isfinite(item.positionM.y)
            || !std::isfinite(item.positionM.z) || !std::isfinite(item.yawDeg)
            || std::abs(item.yawDeg / 90.0F - std::round(item.yawDeg / 90.0F)) > 0.00001F
            || !ids.insert(item.id).second)
            throw std::runtime_error("Invalid lunar layout instance: " + line);
        result.push_back(std::move(item));
    }
    if (result.empty()) throw std::runtime_error("Lunar layout contains no instances");
    return result;
}
} // namespace Kimgane::Shared::World
