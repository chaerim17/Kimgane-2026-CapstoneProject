#pragma once

#include <cstdint>
#include <vector>

namespace Kimgane::Shared::Geometry
{

struct Vec3
{
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
};

struct ObjGeometryData
{
    std::vector<Vec3> positionsM;
    std::vector<Vec3> normals;
    std::vector<std::uint32_t> indices;
};

} // namespace Kimgane::Shared::Geometry
