#include <random>
#include "NpcSetting.h"
#include "Npc.h"
#include "../Terrain/ServerTerrain.h"
#include "../Core/Server.h"

std::vector<std::unique_ptr<Npc>> NpcSetting::gNpcs;

bool NpcSetting::IsNpc(int objectId)
{
    return objectId >= MAX_PLAYERS && objectId < MAX_PLAYERS + COUNT;
}

void NpcSetting::Initialize(TerrainHeightMap& terrain)
{
    static std::mt19937 rng(std::random_device{}());

    const float halfWidth = terrain.GetWorldWidthM() * 0.5f;
    const float halfLength = terrain.GetWorldLengthM() * 0.5f;

    std::uniform_real_distribution<float> distX(-halfWidth, halfWidth);
    std::uniform_real_distribution<float> distZ(-halfLength, halfLength);

    gNpcs.reserve(COUNT);

    for (int i = 0; i < COUNT; ++i)
    {
        auto npc = std::make_unique<Npc>();

        npc->mId = MAX_PLAYERS + i;

        npc->mX = distX(rng);
        npc->mZ = distZ(rng);

        const float sampleX = npc->mX + halfWidth;
        const float sampleZ = npc->mZ + halfLength;

        npc->mY = terrain.SampleHeightM(sampleX, sampleZ);

        npc->mLastMove = std::chrono::steady_clock::now();

        gNpcs.push_back(std::move(npc));
    }
}

void NpcSetting::Update(TerrainHeightMap& terrain)
{
    const auto now = std::chrono::steady_clock::now();

    const float halfWidth = terrain.GetWorldWidthM() * 0.5f;
    const float halfLength = terrain.GetWorldLengthM() * 0.5f;

    for (auto& npc : gNpcs)
    {
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - npc->mLastMove);

        if (elapsed.count() < MOVE_INTERVAL_MS)
            continue;

        npc->mLastMove = now;

        npc->RandomMove();

        const float sampleX = npc->mX + halfWidth;
        const float sampleZ = npc->mZ + halfLength;

        npc->mY = terrain.SampleHeightM(sampleX, sampleZ);


        //// 패킷 전송
        //for (int i = 0; i < MAX_PLAYERS; ++i)
        //{
        //    if (!clients[i] || !clients[i]->IsConnected())
        //        continue;
        //    clients[i]->SendMoveObject(npc->mId);
        //}
    }
}
