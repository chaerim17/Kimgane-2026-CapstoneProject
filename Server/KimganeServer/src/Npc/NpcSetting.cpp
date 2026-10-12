#include <random>
#include <stdexcept>
#include "NpcSetting.h"
#include "Npc.h"
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

        namespace Physics = Kimgane::Shared::Physics;
        const auto origin = Kimgane::Shared::World::TestMapSettings::TERRAIN_POSITION_M;
        auto& position = npc->mMovementState.positionM;
        position = {origin.x + distX(rng), origin.y, origin.z + distZ(rng)};
        Physics::TerrainSample sample{};
        if (!world.GetMapCollision().GetTerrainSampler().SampleHeightAtWorld(position, sample))
            throw std::runtime_error("NPC spawn is outside terrain");
        position.y = sample.heightM;
        npc->ResolveSpawnContacts(world.GetMapCollision().GetWorld());
        npc->RandomMove();

        npc->mLastMove = std::chrono::steady_clock::now();
    }

    std::cout << "[NPC] Initialization Complete. Total NPCs: " << COUNT << '\n';
}

std::vector<Npc*> NpcSetting::Update(GameWorld& world, float deltaTimeSec)
{
    std::vector<Npc*> updated;
    const auto now = std::chrono::steady_clock::now();

    for (const auto& [id, object] : world.GetObjects())
    {
        if (object->GetType() != GameObject::ObjectType::Npc)
            continue;
        auto* npc = static_cast<Npc*>(object.get());
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - npc->mLastMove);

        if (elapsed.count() >= MOVE_INTERVAL_MS)
        {
            npc->mLastMove = now;
            npc->RandomMove();
        }
        npc->UpdateMovement(world.GetMapCollision().GetWorld(), deltaTimeSec);

        updated.push_back(npc);
    }
    return updated;
}
