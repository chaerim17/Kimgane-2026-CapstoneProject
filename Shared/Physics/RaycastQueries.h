#pragma once

#include "CollisionTypes.h"

#include <vector>

// CollisionQueries.h가 겹침 검사를 다루는 것과 대칭으로, 여기서는 ray가 shape에 맞았는지와
// 맞은 지점/거리만 반환. 위치 보정이나 데미지 판정은 호출하는 쪽(client 조준, server 판정)에서 함.
namespace Kimgane::Shared::Physics::RaycastQueries
{
// directionM은 반드시 정규화된 벡터. maxDistanceM은 사거리.
struct Ray
{
    Vec3 originM = {};
    Vec3 directionM = {0.0F, 0.0F, 1.0F};
    float maxDistanceM = 100.0F;
};

// hit이 false면 나머지 변수는 의미가 없음.
struct RaycastHit
{
    bool hit = false;
    ObjectId objectId = INVALID_OBJECT_ID;
    Vec3 pointM = {};
    Vec3 normal = {0.0F, 1.0F, 0.0F};
    float distanceM = 0.0F;
};

/// Ray와 축 정렬 box(중심/half-extents 기준)의 교차를 검사
[[nodiscard]] bool RaycastAabb(const Ray& ray, const Vec3& centerM, const Vec3& extentsM, RaycastHit& outHit) noexcept;

/// Ray와 Box(TestHouse 등 정적 장애물)의 교차를 검사
[[nodiscard]] bool RaycastBox(const Ray& ray, const Box& box, RaycastHit& outHit) noexcept;

/// Ray와 세로 capsule(플레이어/NPC)의 교차를 검사.
[[nodiscard]] bool RaycastCapsule(const Ray& ray, const Capsule& capsule, RaycastHit& outHit) noexcept;

/// Ray와 TerrainSampler 높이맵의 교차를 검사. 일정 간격으로 전진하며 지형을 통과하는 지점을 찾음.
[[nodiscard]] bool RaycastTerrain(const Ray& ray, const TerrainSurface& terrain, RaycastHit& outHit) noexcept;

/// CollisionBody의 shape variant를 보고 맞는 Raycast 함수를 호출. outHit.objectId는 body.objectId로 채움.
[[nodiscard]] bool RaycastCollisionBody(const Ray& ray, const CollisionBody& body, RaycastHit& outHit) noexcept;

/// 여러 body 중 가장 가까운 hit을 찾음. ignoredObjectId는 쏜 사람 자신을 제외할 때 씀.
[[nodiscard]] bool RaycastClosest(const Ray& ray,
                                  const std::vector<CollisionBody>& bodies,
                                  ObjectId ignoredObjectId,
                                  RaycastHit& outHit);
} // namespace Kimgane::Shared::Physics::RaycastQueries
