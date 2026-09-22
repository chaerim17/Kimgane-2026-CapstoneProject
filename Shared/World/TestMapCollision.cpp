#include "TestMapCollision.h"

#include <limits>
#include <stdexcept>
#include <utility>

namespace Kimgane::Shared::World
{
std::shared_ptr<Terrain::HeightMapData> LoadTestMapTerrain()
{
    return Terrain::HeightMapData::LoadRaw8(TestMapSettings::TERRAIN_RAW_PATH,
        TestMapSettings::TERRAIN_WIDTH, TestMapSettings::TERRAIN_LENGTH,
        TestMapSettings::TERRAIN_CELL_SPACING_M, TestMapSettings::TERRAIN_HEIGHT_SCALE_M);
}

void TestMapCollision::Load(std::shared_ptr<const Terrain::HeightMapData> terrain)
{
    const auto houseBoxes = Geometry::CollisionBoxLoader::Load(TestMapSettings::HOUSE_COLLISION_PATH);
    Build(std::move(terrain), houseBoxes);
}

void TestMapCollision::Build(std::shared_ptr<const Terrain::HeightMapData> terrain,
                            std::span<const Geometry::NamedCollisionBox> localHouseBoxes)
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

    // A failed load leaves the previous world usable. Sampler addresses survive moves.
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
    *this = std::move(next);
}

bool TestMapCollision::IsHouseCollider(Physics::ObjectId id) const noexcept
{
    return id >= TestMapSettings::FIRST_HOUSE_COLLIDER_ID &&
        static_cast<std::size_t>(id - TestMapSettings::FIRST_HOUSE_COLLIDER_ID) < mHouseBoxes.size();
}
} // namespace Kimgane::Shared::World
