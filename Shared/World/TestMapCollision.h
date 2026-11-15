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

// 복사본끼리 변하지 않는 지형 데이터는 공유하지만, 충돌 월드(등록 목록)는 각자 따로 가짐.
class TestMapCollision final
{
public:
    void Load(std::shared_ptr<const Terrain::HeightMapData> terrain);
    // localPurifierBoxes가 비어 있으면 정화기를 등록하지 않음(테스트에서 사용).
    void Build(std::shared_ptr<const Terrain::HeightMapData> terrain,
               std::span<const Geometry::NamedCollisionBox> localHouseBoxes,
               std::span<const Geometry::NamedCollisionBox> localPurifierBoxes = {});

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
