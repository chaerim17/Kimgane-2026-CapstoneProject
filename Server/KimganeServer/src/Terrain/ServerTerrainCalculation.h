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

// 접속 시 공통 초기 상태와 이동 입력을 초기화합니다. 월드 접촉은 ResolveSpawn에서 결정합니다.
void InitializeCharacter(Session& session);

void ResolveSpawn(Session& session, int objectId,
    const Kimgane::Shared::Physics::CollisionWorld& collisionWorld);

// 입력 변환부터 수평·수직·접촉 계산과 결과 반영까지 한 번에 처리합니다.
void UpdateCharacter(Session& session, int objectId,
    const Kimgane::Shared::Physics::CollisionWorld& collisionWorld, float deltaTimeSec);

// 맵이 소유한 공통 sampler로 지형만 검사합니다. NPC 선택·집 차단·데미지는 Server의 책임입니다.
bool BlocksShot(const Kimgane::Shared::Physics::TerrainSampler& terrain,
    const Kimgane::Shared::Physics::RaycastQueries::Ray& ray);
}
