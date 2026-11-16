#include "GameWorld.h"
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
    auto terrain = Kimgane::Shared::World::LoadTestMapTerrain();
    auto houseGeometry = Kimgane::Shared::Geometry::ObjLoader::Load(
        Kimgane::Shared::World::TestMapSettings::HOUSE_MODEL_PATH);
    namespace Crate = ItemBoxAssetSettings;
    auto bodyGeometry = Kimgane::Shared::Geometry::ObjLoader::Load(Crate::BODY_MODEL_PATH);
    auto lidGeometry = Kimgane::Shared::Geometry::ObjLoader::Load(Crate::LID_MODEL_PATH);
    const auto crateBoxes = Kimgane::Shared::Geometry::CollisionBoxLoader::Load(Crate::COLLISION_PATH);
    // 클라이언트와 동일하게 닫힌 몸체+뚜껑을 감싸는 단일 박스를 사용.
    if (crateBoxes.size() != 1)
        throw std::runtime_error("Item box collision requires one closed-crate box");
    mMapCollision.Load(terrain);
    mTerrain = std::move(terrain);
    mHouseGeometry = std::move(houseGeometry);
    mItemBoxBodyGeometry = std::move(bodyGeometry);
    mItemBoxLidGeometry = std::move(lidGeometry);
    mItemBoxLocalCollision = crateBoxes.front().box;
    mObjects.clear();
    mNextObjectId = MAX_PLAYERS;
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
    auto player = std::make_unique<Player>(id);
    //디버깅용
    (void)player->GetInventory().TryAdd({Inventory::ItemId::PuriSyringe, 1});
    return static_cast<Player&>(AddObject(std::move(player)));
}

int GameWorld::AllocateObjectId()
{
    const auto& collisionWorld = mMapCollision.GetWorld();
    while (mNextObjectId <= std::numeric_limits<int>::max())
    {
        const int id = static_cast<int>(mNextObjectId++);
        if (!collisionWorld.ContainsBody(id))
            return id;
    }
    throw std::overflow_error("World object IDs exhausted");
}

Npc& GameWorld::CreateNpc()
{
    return static_cast<Npc&>(AddObject(std::make_unique<Npc>(AllocateObjectId())));
}

ItemBox& GameWorld::CreateItemBox(const GameObject::Vec3& positionM)
{
    if (!mTerrain)
        throw std::logic_error("Load the map before creating item boxes");
    const int id = AllocateObjectId();
    auto& box = static_cast<ItemBox&>(AddObject(std::make_unique<ItemBox>(id, positionM, mItemBoxLocalCollision)));
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
    namespace Settings = ItemBoxSpawnSettings;
    if (!mTerrain)
        throw std::logic_error("Load the map before spawning item boxes");
    if (count <= 0)
        return 0;

    const auto half = mItemBoxLocalCollision.halfExtentsM;
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
    for (; spawned < count; ++spawned)
    {
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
            CreateItemBox(position);
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
