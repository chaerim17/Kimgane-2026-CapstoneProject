#include "TestMapCollision.h"

#include <limits>
#include <stdexcept>
#include <utility>

namespace Kimgane::Shared::World
{
std::shared_ptr<Terrain::HeightMapData> LoadTestMapTerrain()
{
    return Terrain::HeightMapData::LoadRaw16(TestMapSettings::TERRAIN_RAW_PATH,
        TestMapSettings::TERRAIN_WIDTH, TestMapSettings::TERRAIN_LENGTH,
        TestMapSettings::TERRAIN_CELL_SPACING_M, TestMapSettings::TERRAIN_HEIGHT_SCALE_M);
}

void TestMapCollision::Load(std::shared_ptr<const Terrain::HeightMapData> terrain)
{
    const auto houseBoxes = Geometry::CollisionBoxLoader::Load(TestMapSettings::HOUSE_COLLISION_PATH);
    const auto purifierBoxes = Geometry::CollisionBoxLoader::Load(TestMapSettings::PURIFIER_COLLISION_PATH);
    Build(std::move(terrain), houseBoxes, purifierBoxes);
}

void TestMapCollision::Build(std::shared_ptr<const Terrain::HeightMapData> terrain,
                            std::span<const Geometry::NamedCollisionBox> localHouseBoxes,
                            std::span<const Geometry::NamedCollisionBox> localPurifierBoxes)
{
    if (!terrain)
    {
        throw std::invalid_argument("Map collision requires terrain data");
    }
    if (localHouseBoxes.size() > static_cast<std::size_t>(
            std::numeric_limits<Physics::ObjectId>::max() - TestMapSettings::FIRST_HOUSE_COLLIDER_ID))
    {
        throw std::length_error("Too many map colliders");
    }
    if (static_cast<std::size_t>(TestMapSettings::FIRST_HOUSE_COLLIDER_ID) + localHouseBoxes.size() >
        static_cast<std::size_t>(TestMapSettings::FIRST_PURIFIER_COLLIDER_ID))
    {
        throw std::length_error("House collider IDs overlap purifier collider IDs");
    }

    // 구성에 실패해도 기존 월드는 그대로 쓸 수 있게 임시 객체에 만든 뒤 교체함. 샘플러 주소는 이동 후에도 유지됨.
    TestMapCollision next;
    next.mTerrainSampler = std::make_shared<Terrain::HeightMapSampler>(
        std::move(terrain), TestMapSettings::TERRAIN_POSITION_M);
    next.mWorld.AddOrUpdateBody({TestMapSettings::TERRAIN_COLLIDER_ID,
        Physics::TerrainSurface{next.mTerrainSampler.get()}, Physics::CollisionLayer::TERRAIN,
        Physics::CollisionLayer::ALL, false});

    next.mHouseBoxes.reserve(localHouseBoxes.size());
    auto colliderId = TestMapSettings::FIRST_HOUSE_COLLIDER_ID;
    for (const auto& localBox : localHouseBoxes)
    {
        auto worldBox = localBox;
        worldBox.box.centerM = Physics::Add(localBox.box.centerM, TestMapSettings::HOUSE_POSITION_M);
        next.mWorld.AddOrUpdateBody({colliderId++, worldBox.box, Physics::CollisionLayer::STATIC_WORLD,
            Physics::CollisionLayer::ALL, false});
        next.mHouseBoxes.push_back(std::move(worldBox));
    }

    // 정화기: 고체 박스(발판·방출기·필터 뱅크)를 먼저 등록하고 마지막에 입구 경사로를 등록.
    if (!localPurifierBoxes.empty())
    {
        auto purifierColliderId = TestMapSettings::FIRST_PURIFIER_COLLIDER_ID;
        for (const auto& localBox : localPurifierBoxes)
        {
            Physics::Box worldBox = localBox.box;
            worldBox.centerM = Physics::Add(localBox.box.centerM, TestMapSettings::PURIFIER_POSITION_M);
            next.mWorld.AddOrUpdateBody({purifierColliderId++, worldBox, Physics::CollisionLayer::STATIC_WORLD,
                Physics::CollisionLayer::ALL, false});
        }

        Physics::Ramp ramp;
        ramp.centerM = Physics::Add(TestMapSettings::PURIFIER_RAMP_CENTER_M, TestMapSettings::PURIFIER_POSITION_M);
        ramp.sizeM = TestMapSettings::PURIFIER_RAMP_SIZE_M;
        ramp.direction = TestMapSettings::PURIFIER_RAMP_DIRECTION;
        next.mWorld.AddOrUpdateBody({purifierColliderId, ramp, Physics::CollisionLayer::STATIC_WORLD,
            Physics::CollisionLayer::ALL, false});
    }
    *this = std::move(next);
}

bool TestMapCollision::IsHouseCollider(Physics::ObjectId id) const noexcept
{
    return id >= TestMapSettings::FIRST_HOUSE_COLLIDER_ID &&
        static_cast<std::size_t>(id - TestMapSettings::FIRST_HOUSE_COLLIDER_ID) < mHouseBoxes.size();
}
} // namespace Kimgane::Shared::World
