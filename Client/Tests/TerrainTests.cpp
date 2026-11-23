#include "Terrain/TerrainChunkGeometry.h"
#include "../../Shared/World/TestMapCollision.h"
#include "../../Shared/World/TestMapSettings.h"

#include <cmath>
#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

#include <Windows.h>

using namespace Kimgane::Client;
namespace Map = Kimgane::Shared::World::TestMapSettings;

void Require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

using Point = std::pair<float, float>;
using Segment = std::pair<Point, Point>;

// 위에서 내려다본 삼각형들이 청크를 빈틈·겹침 없이 정확히 한 번 덮는지,
// 테두리가 아닌 모서리마다 삼각형이 두 개씩 붙어 있는지 검사함 (T자 이음새·구멍 검출).
void CheckTopology(const TerrainGeometry& geometry, float sizeX, float sizeZ)
{
    std::map<Segment, unsigned int> edges;
    double area = 0;
    for (std::size_t i = 0; i < geometry.indices.size(); i += 3)
    {
        DirectX::XMFLOAT3 points[3];
        for (unsigned int k = 0; k < 3; ++k)
        {
            Require(geometry.indices[i + k] < geometry.vertices.size(), "인덱스가 정점 버퍼 범위를 벗어남");
            points[k] = geometry.vertices[geometry.indices[i + k]].positionM;
            Require(std::isfinite(points[k].y), "지형 높이가 유한한 값이 아님");
        }
        const auto& a = points[0];
        const auto& b = points[1];
        const auto& c = points[2];
        const float twiceArea = (b.z - a.z) * (c.x - a.x) - (b.x - a.x) * (c.z - a.z);
        Require(twiceArea > 0, "넓이가 0이거나 뒤집힌 삼각형이 있음");
        area += twiceArea * 0.5;
        for (unsigned int k = 0; k < 3; ++k)
        {
            Point p{points[k].x, points[k].z};
            Point q{points[(k + 1) % 3].x, points[(k + 1) % 3].z};
            if (q < p) std::swap(p, q);
            ++edges[{p, q}];
        }
    }
    Require(std::abs(area - sizeX * sizeZ) < 0.001, "청크가 예상 넓이를 덮지 않음");
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
        Require(count == (border ? 1U : 2U), "내부 모서리가 열려 있거나, T자 이음새 또는 겹친 삼각형이 있음");
    }
}

int main()
{
    // /utf-8 빌드라 문자열이 UTF-8로 들어감. 콘솔도 UTF-8로 맞춰야 한국어 메시지가 깨지지 않음.
    SetConsoleOutputCP(CP_UTF8);
    try
    {
        const auto shared = Kimgane::Shared::World::LoadTestMapTerrain();
        const TerrainHeightMap terrain(shared);
        Kimgane::Shared::Terrain::HeightMapSampler collision(shared, Map::TERRAIN_POSITION_M);
        Require(terrain.GetWidth() == 513 && terrain.GetLength() == 513, "RAW 높이맵 크기가 다름");
        Require(terrain.GetWorldWidthM() == 384, "RAW 칸 간격이 다름");
        Kimgane::Shared::Physics::TerrainSample sample;
        Require(collision.SampleHeightAtWorld({-5, 0, 0}, sample), "스폰 위치가 지형 밖에 있음");
        Require(std::abs(sample.heightM - 4.71F) < 0.001F, "스폰 지점에서 RAW 원점 또는 높이 배율이 맞지 않음");
        Require(collision.SampleHeightAtWorld({-56, 0, -76}, sample) && sample.heightM < 0,
                "주 분화구 바닥이 움푹 들어가 있어야 함");

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
                    Require(geometry.indices.size() < previousCount, "LOD가 올라가면 삼각형 수가 줄어야 함");
                    previousCount = geometry.indices.size();
                    std::set<std::pair<Point, float>> border;
                    for (const auto& v : geometry.vertices)
                    {
                        const float gx = (v.positionM.x + 192) / 0.75F;
                        const float gz = (v.positionM.z + 192) / 0.75F;
                        if (gx == x || gx == x + 64 || gz == z || gz == z + 64)
                            border.insert({{v.positionM.x, v.positionM.z}, v.positionM.y});
                        Require(std::abs(v.positionM.y - shared->SampleHeightM(v.uv.x, v.uv.y)) < 0.001F,
                                "렌더링 정점 높이가 충돌 높이와 다름");
                    }
                    if (lod == 0) { fineBorder = border; fullTriangles += geometry.indices.size() / 3; }
                    else Require(border == fineBorder, "LOD별 테두리 높이 또는 정점 위치가 다름");
                    if (lod == 3) coarseTriangles += geometry.indices.size() / 3;
                }
                cliffTriangles += BuildCliffChunk(terrain, x, z, 64, 64).indices.size() / 3;
            }
        Require(cliffTriangles > 0, "절벽 메쉬가 만들어지지 않음");
        CheckTopology(BuildTerrainChunk(terrain, 0, 0, 61, 57, 3), 61 * 0.75F, 57 * 0.75F);
        Require(SelectTerrainLod(62, 0) == 0 && SelectTerrainLod(62, 1) == 1, "LOD 경계에서 단계가 자꾸 바뀌지 않게 하는 여유 구간(히스테리시스)이 동작하지 않음");
        Require(SelectTerrainLod(300, 0) == 3 && SelectTerrainLod(0, 3) == 0, "거리별 LOD 선택이 잘못됨");
        DirectX::XMFLOAT4X4 vp;
        DirectX::XMStoreFloat4x4(&vp, DirectX::XMMatrixPerspectiveFovLH(1, 1, 0.1F, 100));
        Require(IsTerrainBoundsVisible({{0, 0, 10}, {1, 1, 1}}, vp), "보이는 청크가 컬링됨");
        Require(!IsTerrainBoundsVisible({{0, 0, -10}, {1, 1, 1}}, vp), "카메라 뒤 청크가 컬링되지 않음");
        Require(!IsTerrainBoundsVisible({{200, 0, 10}, {1, 1, 1}}, vp), "화면 밖 청크가 컬링되지 않음");
        std::cout << "지형 테스트 통과: 청크 64개 x LOD 4단계, 지형 삼각형 " << fullTriangles << " -> "
                  << coarseTriangles << "개, 절벽 삼각형 " << cliffTriangles << "개\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
