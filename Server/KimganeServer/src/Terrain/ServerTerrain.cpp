#include "ServerTerrain.h"
#include "TerrainHeightMap.h"

bool ServerTerrain::Load()
{
    mHeightMap = TerrainHeightMap::LoadRawAuto(L"Map/height.raw", 1.0f, 100.0f);

    return mHeightMap != nullptr;
}

float ServerTerrain::GetHeight(float x, float z) const
{
    if (!mHeightMap)
        return 0.f;

    return mHeightMap->SampleHeightM(x, z);
}
