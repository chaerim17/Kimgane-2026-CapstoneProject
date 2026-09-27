#include "GameWorld.h"
#include "../Terrain/ServerTerrainCalculation.h"

#include <stdexcept>
#include <utility>

void GameWorld::LoadMap()
{
    auto terrain = ServerTerrainCalculation::LoadTerrain();
    mMapCollision.Load(terrain);
    mTerrain = std::move(terrain);
    mObjects.clear();
}

GameObject& GameWorld::AddObject(std::unique_ptr<GameObject> object)
{
    const int id = object->GetId();
    auto [it, inserted] = mObjects.try_emplace(id, std::move(object));
    if (!inserted)
        throw std::logic_error("Duplicate world object ID");
    return *it->second;
}

Player& GameWorld::CreatePlayer(int id)
{
    if (id < 0 || id >= MAX_PLAYERS)
        throw std::invalid_argument("Invalid player ID");
    return static_cast<Player&>(AddObject(std::make_unique<Player>(id)));
}

Npc& GameWorld::CreateNpc(int id)
{
    if (id < MAX_PLAYERS || id >= MAX_OBJECTS)
        throw std::invalid_argument("Invalid NPC ID");
    return static_cast<Npc&>(AddObject(std::make_unique<Npc>(id)));
}

ItemBox& GameWorld::CreateItemBox(int id, const GameObject::Vec3& positionM)
{
    // 기존 캐릭터 ID와 정적 맵 collider ID를 침범하지 않게 검사
    if (id < MAX_OBJECTS || id >= Kimgane::Shared::World::TestMapSettings::TERRAIN_COLLIDER_ID)
        throw std::invalid_argument("Invalid item box ID");
    auto& box = static_cast<ItemBox&>(AddObject(std::make_unique<ItemBox>(id, positionM)));
    namespace Physics = Kimgane::Shared::Physics;
    try
    {
        if (!mMapCollision.GetWorld().AddOrUpdateBody({id, box.GetCollisionBox(),
                Physics::CollisionLayer::STATIC_WORLD, Physics::CollisionLayer::ALL, false}))
            throw std::invalid_argument("Invalid item box collision body");
    }
    catch (...)
    {
        mObjects.erase(id);
        throw;
    }
    return box;
}

bool GameWorld::RemoveObject(int id)
{
    if (mObjects.erase(id) == 0)
        return false;
    mMapCollision.GetWorld().RemoveBody(id);
    return true;
}

GameObject* GameWorld::FindObject(int id) noexcept
{
    const auto it = mObjects.find(id);
    return it == mObjects.end() ? nullptr : it->second.get();
}

Player* GameWorld::FindPlayer(int id) noexcept
{
    auto* object = FindObject(id);
    return object && object->GetType() == GameObject::ObjectType::Player ? static_cast<Player*>(object) : nullptr;
}
