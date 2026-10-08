#include "Engine/Terrain/TerrainChunkGeometry.h"
#include "../../Shared/World/TestMapCollision.h"
#include "../../Shared/World/TestMapSettings.h"

#include <cmath>
#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

using namespace Kimgane::Engine;
namespace Map = Kimgane::Shared::World::TestMapSettings;

void Require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

using Point = std::pair<float, float>;
using Segment = std::pair<Point, Point>;

// Checks that the projected triangle mesh covers the chunk exactly once and
// every non-boundary edge has two incident faces (detects T-junctions / holes).
void CheckTopology(const TerrainGeometry& geometry, float sizeX, float sizeZ)
{
    std::map<Segment, unsigned int> edges;
    double area = 0;
    for (std::size_t i = 0; i < geometry.indices.size(); i += 3)
    {
        DirectX::XMFLOAT3 points[3];
        for (unsigned int k = 0; k < 3; ++k)
        {
            Require(geometry.indices[i + k] < geometry.vertices.size(), "Index outside vertex buffer");
            points[k] = geometry.vertices[geometry.indices[i + k]].positionM;
            Require(std::isfinite(points[k].y), "Non-finite terrain height");
        }
        const auto& a = points[0];
        const auto& b = points[1];
        const auto& c = points[2];
        const float twiceArea = (b.z - a.z) * (c.x - a.x) - (b.x - a.x) * (c.z - a.z);
        Require(twiceArea > 0, "Degenerate or reversed triangle");
        area += twiceArea * 0.5;
        for (unsigned int k = 0; k < 3; ++k)
        {
            Point p{points[k].x, points[k].z};
            Point q{points[(k + 1) % 3].x, points[(k + 1) % 3].z};
            if (q < p) std::swap(p, q);
            ++edges[{p, q}];
        }
    }
    Require(std::abs(area - sizeX * sizeZ) < 0.001, "Chunk does not cover the expected area");
    float minX = 1e9F, minZ = 1e9F, maxX = -1e9F, maxZ = -1e9F;
    for (const auto& v : geometry.vertices)
    {
        minX = std::min(minX, v.positionM.x); maxX = std::max(maxX, v.positionM.x);
        minZ = std::min(minZ, v.positionM.z); maxZ = std::max(maxZ, v.positionM.z);
    }
    for (const auto& [edge, count] : edges)
    {
        const auto& [p, q] = edge;
        const bool border = (p.first == q.first && (p.first == minX || p.first == maxX))
            || (p.second == q.second && (p.second == minZ || p.second == maxZ));
        Require(count == (border ? 1U : 2U), "Open interior edge / T-junction / overlapping triangles");
    }
}

int main()
{
    try
    {
        const auto shared = Kimgane::Shared::World::LoadTestMapTerrain();
        const TerrainHeightMap terrain(shared);
        Kimgane::Shared::Terrain::HeightMapSampler collision(shared, Map::TERRAIN_POSITION_M);
        Require(terrain.GetWidth() == 513 && terrain.GetLength() == 513, "RAW dimensions");
        Require(terrain.GetWorldWidthM() == 384, "RAW cell spacing");
        Kimgane::Shared::Physics::TerrainSample sample;
        Require(collision.SampleHeightAtWorld({-5, 0, 0}, sample), "Spawn is outside terrain");
        Require(std::abs(sample.heightM - 4.71F) < 0.001F, "RAW origin / height scale mismatch at spawn");
        Require(collision.SampleHeightAtWorld({58, 0, 43}, sample) && sample.heightM < 0,
                "Primary crater must have a depressed floor");

        std::size_t fullTriangles = 0, coarseTriangles = 0, cliffTriangles = 0;
        for (unsigned int z = 0; z < 512; z += 64)
            for (unsigned int x = 0; x < 512; x += 64)
            {
                std::set<std::pair<Point, float>> fineBorder;
                std::size_t previousCount = 1000000;
                for (unsigned int lod = 0; lod < 4; ++lod)
                {
                    const auto geometry = BuildTerrainChunk(terrain, x, z, 64, 64, lod);
                    CheckTopology(geometry, 48, 48);
                    Require(geometry.indices.size() < previousCount, "LOD must reduce triangle count");
                    previousCount = geometry.indices.size();
                    std::set<std::pair<Point, float>> border;
                    for (const auto& v : geometry.vertices)
                    {
                        const float gx = (v.positionM.x + 192) / 0.75F;
                        const float gz = (v.positionM.z + 192) / 0.75F;
                        if (gx == x || gx == x + 64 || gz == z || gz == z + 64)
                            border.insert({{v.positionM.x, v.positionM.z}, v.positionM.y});
                        Require(std::abs(v.positionM.y - shared->SampleHeightM(v.uv.x, v.uv.y)) < 0.001F,
                                "Render vertex differs from collision height");
                    }
                    if (lod == 0) { fineBorder = border; fullTriangles += geometry.indices.size() / 3; }
                    else Require(border == fineBorder, "LOD boundary heights or vertex positions differ");
                    if (lod == 3) coarseTriangles += geometry.indices.size() / 3;
                }
                cliffTriangles += BuildCliffChunk(terrain, x, z, 64, 64).indices.size() / 3;
            }
        Require(cliffTriangles > 0, "No cliff meshes generated");
        CheckTopology(BuildTerrainChunk(terrain, 0, 0, 61, 57, 3), 61 * 0.75F, 57 * 0.75F);
        Require(SelectTerrainLod(62, 0) == 0 && SelectTerrainLod(62, 1) == 1, "LOD hysteresis");
        Require(SelectTerrainLod(300, 0) == 3 && SelectTerrainLod(0, 3) == 0, "LOD distance selection");
        DirectX::XMFLOAT4X4 vp;
        DirectX::XMStoreFloat4x4(&vp, DirectX::XMMatrixPerspectiveFovLH(1, 1, 0.1F, 100));
        Require(IsTerrainBoundsVisible({{0, 0, 10}, {1, 1, 1}}, vp), "Visible chunk culled");
        Require(!IsTerrainBoundsVisible({{0, 0, -10}, {1, 1, 1}}, vp), "Chunk behind camera not culled");
        Require(!IsTerrainBoundsVisible({{200, 0, 10}, {1, 1, 1}}, vp), "Off-screen chunk not culled");
        std::cout << "Terrain tests passed: 64 chunks x 4 LODs, " << fullTriangles << " -> "
                  << coarseTriangles << " terrain triangles, " << cliffTriangles << " cliff triangles\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
