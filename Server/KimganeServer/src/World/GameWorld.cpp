#include "GameWorld.h"
#include "../Terrain/ServerTerrainCalculation.h"
#include "../../../../Shared/Geometry/ObjLoader.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include "../Config/ServerConfig.h"

#include <stdexcept>
#include <utility>

void GameWorld::LoadMap()
{
    auto terrain = ServerTerrainCalculation::LoadTerrain();
    auto houseGeometry = Kimgane::Shared::Geometry::ObjLoader::Load(
        Kimgane::Shared::World::TestMapSettings::HOUSE_MODEL_PATH);
    mMapCollision.Load(terrain);
    mTerrain = std::move(terrain);
    mHouseGeometry = std::move(houseGeometry);
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

int GameWorld::SpawnItemBoxes(int count)
{
    namespace Physics = Kimgane::Shared::Physics;
    namespace Map = Kimgane::Shared::World::TestMapSettings;
    namespace Definition = Kimgane::Shared::World::CrateDefinition;
    namespace Settings = ItemBoxSpawnSettings;
    if (!mTerrain)
        throw std::logic_error("Load the map before spawning item boxes");
    if (count <= 0)
        return 0;

    const auto half = Definition::HALF_EXTENTS_M;
    const float terrainMinX = Map::TERRAIN_POSITION_M.x - mTerrain->GetWorldWidthM() * 0.5F;
    const float terrainMinZ = Map::TERRAIN_POSITION_M.z - mTerrain->GetWorldLengthM() * 0.5F;
    const float minX = terrainMinX + half.x + Settings::EDGE_MARGIN_M;
    const float minZ = terrainMinZ + half.z + Settings::EDGE_MARGIN_M;
    const float maxX = terrainMinX + mTerrain->GetWorldWidthM() - half.x - Settings::EDGE_MARGIN_M;
    const float maxZ = terrainMinZ + mTerrain->GetWorldLengthM() - half.z - Settings::EDGE_MARGIN_M;
    if (minX > maxX || minZ > maxZ)
        return 0;

    std::mt19937 random(std::random_device{}());
    std::uniform_real_distribution<float> randomX(minX, maxX);
    std::uniform_real_distribution<float> randomZ(minZ, maxZ);
    const auto& sampler = mMapCollision.GetTerrainSampler();

    // 집 내부/아래와 기존 상자 위에도 배치하지 않게 높이에 관계없이 수평 여유를 확보
    const auto overlaps = [half](const Physics::Vec3& position, const Physics::Box& obstacle, float gap)
    {
        return std::abs(position.x - obstacle.centerM.x) <= half.x + obstacle.halfExtentsM.x + gap &&
               std::abs(position.z - obstacle.centerM.z) <= half.z + obstacle.halfExtentsM.z + gap;
    };
    // 바닥 경계와 그 안의 격자점을 검사, 모서리가 묻히지 않도록
    const float spacing = mTerrain->GetCellSpacingM();
    const auto sampleAxis = [spacing](float low, float high, float origin)
    {
        std::vector<float> samples{low, high, (low + high) * 0.5F};
        const int first = static_cast<int>(std::ceil((low - origin) / spacing));
        const int last = static_cast<int>(std::floor((high - origin) / spacing));
        for (int i = first; i <= last; ++i)
            samples.push_back(origin + static_cast<float>(i) * spacing);
        return samples;
    };

    int spawned = 0;
    int id = MAX_OBJECTS;
    for (; spawned < count; ++spawned, ++id)
    {
        while (id < Map::TERRAIN_COLLIDER_ID &&
               (FindObject(id) || mMapCollision.GetWorld().ContainsBody(id)))
            ++id;
        if (id >= Map::TERRAIN_COLLIDER_ID)
            break;

        bool placed = false;
        for (int attempt = 0; attempt < Settings::MAX_ATTEMPTS_PER_BOX; ++attempt)
        {
            Physics::Vec3 position{randomX(random), 0.0F, randomZ(random)};
            if (overlaps(position, {Map::PLAYER_SPAWN_POSITION_M, {}}, Settings::PLAYER_SPAWN_CLEARANCE_M))
                continue;
            bool blocked = false;
            for (const auto& house : mMapCollision.GetHouseBoxes())
                blocked = blocked || overlaps(position, house.box, Settings::OBSTACLE_GAP_M);
            for (const auto& [objectId, object] : mObjects)
            {
                if (object->GetType() == GameObject::ObjectType::ItemBox)
                    blocked = blocked || overlaps(position, static_cast<const ItemBox&>(*object).GetCollisionBox(),
                                                   Settings::OBSTACLE_GAP_M);
                else
                {
                    const float radius = object->GetType() == GameObject::ObjectType::Player
                        ? Physics::Settings::PLAYER_CAPSULE_RADIUS_M : Physics::Settings::NPC_CAPSULE_RADIUS_M;
                    blocked = blocked || overlaps(position, {object->GetPositionM(), {radius, 0.0F, radius}},
                                                   Settings::OBSTACLE_GAP_M);
                }
            }
            if (blocked)
                continue;

            float lowest = std::numeric_limits<float>::max();
            float highest = std::numeric_limits<float>::lowest();
            const auto xs = sampleAxis(position.x - half.x, position.x + half.x, terrainMinX);
            const auto zs = sampleAxis(position.z - half.z, position.z + half.z, terrainMinZ);
            for (float x : xs)
            {
                for (float z : zs)
                {
                    Physics::TerrainSample sample{};
                    if (!sampler.SampleHeightAtWorld({x, 0.0F, z}, sample) ||
                        !std::isfinite(sample.heightM) || !std::isfinite(sample.normal.y) ||
                        sample.normal.y < Settings::MIN_GROUND_NORMAL_Y)
                    {
                        blocked = true;
                        break;
                    }
                    lowest = std::min(lowest, sample.heightM);
                    highest = std::max(highest, sample.heightM);
                }
                if (blocked)
                    break;
            }
            if (blocked || highest - lowest > Settings::MAX_GROUND_HEIGHT_DIFFERENCE_M)
                continue;

            position.y = highest;
            CreateItemBox(id, position);
            placed = true;
            break;
        }
        // 자리를 찾지 못하면 현재까지 생성한 수만 유지
        if (!placed)
            break;
    }
    return spawned;
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
