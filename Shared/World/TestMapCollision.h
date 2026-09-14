#pragma once

#include "TestMapSettings.h"
#include "../Geometry/CollisionBoxLoader.h"
#include "../Physics/CollisionWorld.h"
#include "../Terrain/HeightMapSampler.h"

#include <memory>
#include <span>
#include <vector>

namespace Kimgane::Shared::World
{
[[nodiscard]] std::shared_ptr<Terrain::HeightMapData> LoadTestMapTerrain();

// Copies share immutable terrain, but keep independent collision registries.
class TestMapCollision final
{
public:
    void Load(std::shared_ptr<const Terrain::HeightMapData> terrain);
    void Build(std::shared_ptr<const Terrain::HeightMapData> terrain,
               std::span<const Geometry::NamedCollisionBox> localHouseBoxes);

    [[nodiscard]] Physics::CollisionWorld& GetWorld() noexcept { return mWorld; }
    [[nodiscard]] const Physics::CollisionWorld& GetWorld() const noexcept { return mWorld; }
    [[nodiscard]] const std::vector<Geometry::NamedCollisionBox>& GetHouseBoxes() const noexcept { return mHouseBoxes; }
    [[nodiscard]] const Physics::TerrainSampler& GetTerrainSampler() const noexcept { return *mTerrainSampler; }
    [[nodiscard]] bool IsHouseCollider(Physics::ObjectId id) const noexcept;

private:
    std::shared_ptr<const Terrain::HeightMapSampler> mTerrainSampler;
    std::vector<Geometry::NamedCollisionBox> mHouseBoxes;
    Physics::CollisionWorld mWorld;
};
} // namespace Kimgane::Shared::World
