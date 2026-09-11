#pragma once

#include "../../IO/AssetPathResolver.h"
#include "../../Physics/CollisionWorld.h"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace Kimgane::Shared::LunarMap
{
struct NamedCollider
{
    std::string name;
    Physics::CollisionShape shape;
};

// Strict, line-oriented loader: box uses half extents; ramp uses full size.
// Shared by both executables so collider coordinates and ramp direction cannot diverge.
inline std::vector<NamedCollider> LoadCollision(const std::filesystem::path& filePath)
{
    const auto resolvedPath = IO::ResolveAssetPath(filePath);
    std::ifstream stream(resolvedPath);
    if (!stream)
    {
        throw std::runtime_error("Lunar collision file not found: " + filePath.string());
    }
    std::vector<NamedCollider> result;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(stream, line))
    {
        ++lineNumber;
        line = line.substr(0, line.find('#'));
        std::istringstream input(line);
        std::string type;
        if (!(input >> type)) continue;
        NamedCollider entry;
        Physics::Vec3 center, size;
        if (!(input >> entry.name >> center.x >> center.y >> center.z >> size.x >> size.y >> size.z) ||
            !std::isfinite(center.x) || !std::isfinite(center.y) || !std::isfinite(center.z) ||
            !std::isfinite(size.x) || !std::isfinite(size.y) || !std::isfinite(size.z) ||
            size.x <= 0.0F || size.y <= 0.0F || size.z <= 0.0F)
        {
            throw std::runtime_error("Invalid lunar collider at line " + std::to_string(lineNumber));
        }
        if (type == "box")
        {
            entry.shape = Physics::Box{center, size};
        }
        else if (type == "ramp")
        {
            std::string direction;
            input >> direction;
            Physics::RampDirection value;
            if (direction == "PositiveX") value = Physics::RampDirection::PositiveX;
            else if (direction == "NegativeX") value = Physics::RampDirection::NegativeX;
            else if (direction == "PositiveZ") value = Physics::RampDirection::PositiveZ;
            else if (direction == "NegativeZ") value = Physics::RampDirection::NegativeZ;
            else throw std::runtime_error("Invalid lunar ramp direction at line " + std::to_string(lineNumber));
            entry.shape = Physics::Ramp{center, size, value};
        }
        else throw std::runtime_error("Unknown lunar collider type at line " + std::to_string(lineNumber));
        std::string extra;
        if (input >> extra) throw std::runtime_error("Extra lunar collider fields at line " + std::to_string(lineNumber));
        result.push_back(std::move(entry));
    }
    if (result.empty()) throw std::runtime_error("Lunar collision file is empty");
    return result;
}

inline void RegisterCollision(Physics::CollisionWorld& world, const std::vector<NamedCollider>& colliders)
{
    Physics::ObjectId id = 10000;
    for (const auto& collider : colliders)
    {
        world.AddOrUpdateBody({id++, collider.shape, Physics::CollisionLayer::STATIC_WORLD,
                              Physics::CollisionLayer::ALL, false});
    }
}
} // namespace Kimgane::Shared::LunarMap
