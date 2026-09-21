#pragma once

#include "TerrainHeightMap.h"
#include "../../../../Shared/Physics/CollisionWorld.h"
#include "../../../../Shared/Physics/RaycastQueries.h"

#include <memory>

class Session;

namespace ServerTerrainCalculation
{
std::shared_ptr<TerrainHeightMap> LoadTerrain();

// 잠금과 전송은 Server가 담당하고, 수평·수직·접촉 계산은 Shared에서 함께 처리합니다.
void UpdateCharacter(Session& session, int objectId,
    const Kimgane::Shared::Physics::CollisionWorld& collisionWorld, float deltaTimeSec);

bool BlocksShot(const Kimgane::Shared::Physics::TerrainSampler& terrain,
    const Kimgane::Shared::Physics::RaycastQueries::Ray& ray);
}
