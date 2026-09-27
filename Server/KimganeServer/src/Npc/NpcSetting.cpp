#include <random>
#include "NpcSetting.h"
#include "Npc.h"
#include "../Terrain/TerrainHeightMap.h"
#include "../World/GameWorld.h"

void NpcSetting::Initialize(GameWorld& world)
{
    auto& terrain = world.GetTerrain();
    static std::mt19937 rng(std::random_device{}());

    const float halfWidth = terrain.GetWorldWidthM() * 0.5f;
    const float halfLength = terrain.GetWorldLengthM() * 0.5f;

    std::uniform_real_distribution<float> distX(-halfWidth, halfWidth);
    std::uniform_real_distribution<float> distZ(-halfLength, halfLength);

    for (int i = 0; i < COUNT; ++i)
    {
        auto* npc = &world.CreateNpc(MAX_PLAYERS + i);

        npc->mPositionM.x = distX(rng);
        npc->mPositionM.z = distZ(rng);

        const float sampleX = npc->mPositionM.x + halfWidth;
        const float sampleZ = npc->mPositionM.z + halfLength;

        npc->mPositionM.y = terrain.SampleHeightM(sampleX, sampleZ);

        npc->mLastMove = std::chrono::steady_clock::now();
    }

    std::cout << "[NPC] Initialization Complete. Total NPCs: " << COUNT << '\n';
}

std::vector<Npc*> NpcSetting::Update(GameWorld& world)
{
    auto& terrain = world.GetTerrain();
    std::vector<Npc*> updated;
    const auto now = std::chrono::steady_clock::now();

    const float halfWidth = terrain.GetWorldWidthM() * 0.5f;
    const float halfLength = terrain.GetWorldLengthM() * 0.5f;

    for (const auto& [id, object] : world.GetObjects())
    {
        if (object->GetType() != GameObject::ObjectType::Npc)
            continue;
        auto* npc = static_cast<Npc*>(object.get());
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - npc->mLastMove);

        if (elapsed.count() < MOVE_INTERVAL_MS)
            continue;

        npc->mLastMove = now;

        npc->RandomMove();

        //npc 움직임 디버깅용
        //std::cout << "[NPC MOVE] " << npc->GetId() << " (" << npc->mPositionM.x << ", " << npc->mPositionM.y << ", " << npc->mPositionM.z << ")\n";

        const float sampleX = npc->mPositionM.x + halfWidth;
        const float sampleZ = npc->mPositionM.z + halfLength;

        npc->mPositionM.y = terrain.SampleHeightM(sampleX, sampleZ);

        updated.push_back(npc);
    }
    return updated;
}
