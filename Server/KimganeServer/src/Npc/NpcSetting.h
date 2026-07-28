#pragma once
#include "../Pch.h"

class Npc;
class TerrainHeightMap;

namespace NpcSetting
{
    constexpr int COUNT = 10;
    constexpr int MOVE_INTERVAL_MS = 1000;

    extern std::vector<std::unique_ptr<Npc>> gNpcs;

    void Initialize(TerrainHeightMap& terrain);
    void Update(TerrainHeightMap& terrain);

    bool IsNpc(int objectId);
} // namespace NpcSetting
