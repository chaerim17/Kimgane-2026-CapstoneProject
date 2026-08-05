#pragma once

#include <memory>

class TerrainHeightMap;
class ServerTerrainSampler;

class ServerTerrain
{
public:
    bool Load();

    float GetHeight(float x, float z) const;

private:
    std::shared_ptr<TerrainHeightMap> mHeightMap;
};
