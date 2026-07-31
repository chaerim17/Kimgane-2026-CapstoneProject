#pragma once

#include "ObjGeometry.h"

#include <filesystem>

namespace Kimgane::Shared::Geometry
{
class ObjLoader final
{
public:
    static ObjGeometryData Load(const std::filesystem::path& filePath);
    ObjLoader() = delete;
};
} // namespace Kimgane::Shared::Geometry
