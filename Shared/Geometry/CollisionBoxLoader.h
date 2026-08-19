#pragma once

#include "../Physics/CollisionTypes.h"

#include <filesystem>
#include <string>
#include <vector>

namespace Kimgane::Shared::Geometry
{
struct NamedCollisionBox
{
    std::string name;
    Kimgane::Shared::Physics::Box box;
};

class CollisionBoxLoader final
{
public:
    static std::vector<NamedCollisionBox> Load(const std::filesystem::path& filePath);
    CollisionBoxLoader() = delete;
};
} // namespace Kimgane::Shared::Geometry
