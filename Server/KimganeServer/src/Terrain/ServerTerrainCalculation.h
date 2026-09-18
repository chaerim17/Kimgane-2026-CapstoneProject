#pragma once

#include "TerrainHeightMap.h"
#include "../../../../Shared/Geometry/CollisionBoxLoader.h"
#include "../../../../Shared/Physics/CollisionWorld.h"
#include "../../../../Shared/Physics/RaycastQueries.h"

#include <span>
#include <vector>

class Session;

namespace ServerTerrainCalculation
{
std::shared_ptr<TerrainHeightMap> LoadTerrain();

std::vector<Kimgane::Shared::Physics::Box> BuildGroundBoxes(
    std::span<const Kimgane::Shared::Geometry::NamedCollisionBox> collisionBoxes,
    float worldOffsetY);

float UpdateHorizontal(Session& session, int objectId, const TerrainHeightMap& terrain,
    const Kimgane::Shared::Physics::CollisionWorld& collisionWorld,
    std::span<const Kimgane::Shared::Physics::Box> groundBoxes,
    float moveSpeed, float deltaTime);

void UpdateVertical(Session& session, float groundHeight, float gravity, float deltaTime);

bool BlocksShot(const TerrainHeightMap& terrain,
    const Kimgane::Shared::Physics::RaycastQueries::Ray& ray);
}
