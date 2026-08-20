#include "CollisionBoxLoader.h"

#include "../IO/AssetPathResolver.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace Kimgane::Shared::Geometry
{
namespace
{
Kimgane::Shared::Physics::Vec3 ReadVec3(std::istringstream& lineStream)
{
    Kimgane::Shared::Physics::Vec3 value = {};
    lineStream >> value.x >> value.y >> value.z;
    return value;
}
} // namespace

std::vector<NamedCollisionBox> CollisionBoxLoader::Load(const std::filesystem::path& filePath)
{
    const std::filesystem::path resolvedPath = Kimgane::Shared::IO::ResolveAssetPath(filePath);
    if (resolvedPath.empty())
    {
        throw std::runtime_error("Collision file not found: " + filePath.string());
    }

    std::ifstream stream(resolvedPath);
    if (!stream)
    {
        throw std::runtime_error("Failed to open collision file: " + resolvedPath.string());
    }

    std::vector<NamedCollisionBox> boxes;
    std::string line;
    while (std::getline(stream, line))
    {
        std::istringstream lineStream(line);
        std::string keyword;
        lineStream >> keyword;

        if (keyword != "box")
        {
            continue;
        }

        NamedCollisionBox namedBox;
        lineStream >> namedBox.name;
        namedBox.box.centerM = ReadVec3(lineStream);
        namedBox.box.halfExtentsM = ReadVec3(lineStream);
        // rx, ry, rz는 읽지 않고 스트림에 남겨둔 채 버림

        boxes.push_back(namedBox);
    }

    return boxes;
}
} // namespace Kimgane::Shared::Geometry
