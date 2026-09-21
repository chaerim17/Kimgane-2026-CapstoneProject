#pragma once

#include "TerrainHeightMap.h"
#include "../../../../Shared/Physics/CollisionWorld.h"
#include "../../../../Shared/Physics/RaycastQueries.h"

#include <memory>

class Session;

// 서버 상태를 Shared 계산에 연결합니다. 잠금과 패킷 전송은 호출자가 담당합니다.
namespace ServerTerrainCalculation
{
std::shared_ptr<TerrainHeightMap> LoadTerrain();

void ResolveSpawn(Session& session, int objectId,
    const Kimgane::Shared::Physics::CollisionWorld& collisionWorld);

void UpdateCharacter(Session& session, int objectId,
    const Kimgane::Shared::Physics::CollisionWorld& collisionWorld, float deltaTimeSec);

// 맵이 소유한 공통 sampler로 지형만 검사합니다. NPC 선택·집 차단·데미지는 Server의 책임입니다.
bool BlocksShot(const Kimgane::Shared::Physics::TerrainSampler& terrain,
    const Kimgane::Shared::Physics::RaycastQueries::Ray& ray);
}
