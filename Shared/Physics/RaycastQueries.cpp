#include "RaycastQueries.h"

#include "CollisionQueries.h"

#include <algorithm>
#include <cmath>

// shared raycast 구현입니다.
// Client는 크로스헤어가 가리키는 조준점을 찾을 때, Server는 발사 방향으로 실제 피격을 검증할 때
// 같은 함수를 쓸 수 있음. 좌표계는 CollisionTypes.h와 동일하게 meter, +Y가 위쪽.
namespace Kimgane::Shared::Physics::RaycastQueries
{
namespace
{
// Capsule 양 끝 반구(sphere cap)를 검사할 때 쓰는 내부 helper.
[[nodiscard]] bool RaycastSphereCore(const Ray& ray, const Vec3& centerM, float radiusM, RaycastHit& outHit) noexcept
{
    outHit = {};

    const Vec3 originToCenterM = Subtract(ray.originM, centerM);
    const float b = Dot(ray.directionM, originToCenterM);
    const float c = Dot(originToCenterM, originToCenterM) - radiusM * radiusM;
    const float h = b * b - c;
    if (h < 0.0F)
    {
        // 판별식이 음수면 ray가 sphere를 아예 스치지 않음.
        return false;
    }

    const float sqrtHM = std::sqrt(h);
    float tM = -b - sqrtHM;
    if (tM < 0.0F)
    {
        // 더 가까운 교차점이 origin보다 뒤에 있으면(= origin이 sphere 안), 먼 쪽 교차점을 씀.
        tM = -b + sqrtHM;
    }
    if (tM < 0.0F || tM > ray.maxDistanceM)
    {
        return false;
    }

    outHit.hit = true;
    outHit.pointM = Add(ray.originM, Scale(ray.directionM, tM));
    outHit.normal = CollisionQueries::NormalizeOrUp(Subtract(outHit.pointM, centerM));
    outHit.distanceM = tM;
    return true;
}
} // namespace

bool RaycastAabb(const Ray& ray, const Vec3& centerM, const Vec3& extentsM, RaycastHit& outHit) noexcept
{
    outHit = {};

    const Vec3 minM = Subtract(centerM, extentsM);
    const Vec3 maxM = Add(centerM, extentsM);

    const float originArr[3] = {ray.originM.x, ray.originM.y, ray.originM.z};
    const float dirArr[3] = {ray.directionM.x, ray.directionM.y, ray.directionM.z};
    const float minArr[3] = {minM.x, minM.y, minM.z};
    const float maxArr[3] = {maxM.x, maxM.y, maxM.z};
    const Vec3 axisNormals[3] = {{1.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}, {0.0F, 0.0F, 1.0F}};

    // Slab method: 각 축마다 ray가 box의 min/max 평면을 지나는 t 구간을 구하고,
    // 세 축의 구간을 교집합으로 좁혀서 마지막에 남는 [tMin, tMax]가 실제 교차 구간임.
    float tMin = 0.0F;
    float tMax = ray.maxDistanceM;
    Vec3 normal = {0.0F, 1.0F, 0.0F};

    for (int axis = 0; axis < 3; ++axis)
    {
        if (std::fabs(dirArr[axis]) < Settings::MIN_COLLIDER_SIZE_M)
        {
            // 이 축으로는 거의 움직이지 않으므로, origin이 이미 슬랩 밖이면 만나지 않음.
            if (originArr[axis] < minArr[axis] || originArr[axis] > maxArr[axis])
            {
                return false;
            }
            continue;
        }

        const float invDir = 1.0F / dirArr[axis];
        float tNearM = (minArr[axis] - originArr[axis]) * invDir;
        float tFarM = (maxArr[axis] - originArr[axis]) * invDir;
        float enterSign = -1.0F;
        if (tNearM > tFarM)
        {
            std::swap(tNearM, tFarM);
            enterSign = 1.0F;
        }

        if (tNearM > tMin)
        {
            tMin = tNearM;
            normal = Scale(axisNormals[axis], enterSign);
        }
        tMax = std::min(tMax, tFarM);

        if (tMin > tMax)
        {
            return false;
        }
    }

    outHit.hit = true;
    outHit.pointM = Add(ray.originM, Scale(ray.directionM, tMin));
    outHit.normal = normal;
    outHit.distanceM = tMin;
    return true;
}

bool RaycastBox(const Ray& ray, const Box& box, RaycastHit& outHit) noexcept
{
    return RaycastAabb(ray, box.centerM, box.halfExtentsM, outHit);
}

bool RaycastCapsule(const Ray& ray, const Capsule& capsule, RaycastHit& outHit) noexcept
{
    outHit = {};

    const Vec3 segStartM = CollisionQueries::GetCapsuleSegmentStartM(capsule);
    const Vec3 segEndM = CollisionQueries::GetCapsuleSegmentEndM(capsule);
    const float radiusM = capsule.radiusM;

    const Vec3 axisM = Subtract(segEndM, segStartM);
    const Vec3 startToOriginM = Subtract(ray.originM, segStartM);

    const float axisAxis = Dot(axisM, axisM);
    const float axisDir = Dot(axisM, ray.directionM);
    const float axisToOrigin = Dot(axisM, startToOriginM);
    const float dirToOrigin = Dot(ray.directionM, startToOriginM);
    const float originToOrigin = Dot(startToOriginM, startToOriginM);

    // 무한 원기둥(몸통)과의 교차를 먼저 품. a가 0에 가까우면 ray가 축과 거의 평행하다는
    // 뜻이라 이 공식이 불안정해지므로, 그 경우는 몸통을 건너뛰고 반구 검사만 함.
    const float a = axisAxis - axisDir * axisDir;
    if (std::fabs(a) > Settings::MIN_COLLIDER_SIZE_M)
    {
        const float b = axisAxis * dirToOrigin - axisToOrigin * axisDir;
        const float c = axisAxis * originToOrigin - axisToOrigin * axisToOrigin - radiusM * radiusM * axisAxis;
        const float h = b * b - a * c;

        if (h >= 0.0F)
        {
            const float tM = (-b - std::sqrt(h)) / a;
            const float yM = axisToOrigin + tM * axisDir;

            // yM이 [0, axisAxis] 안에 있으면 반구가 아니라 원기둥 몸통에 맞은 것.
            if (yM > 0.0F && yM < axisAxis && tM >= 0.0F && tM <= ray.maxDistanceM)
            {
                const Vec3 pointM = Add(ray.originM, Scale(ray.directionM, tM));
                const Vec3 axisPointM = Add(segStartM, Scale(axisM, yM / axisAxis));

                outHit.hit = true;
                outHit.pointM = pointM;
                outHit.normal = CollisionQueries::NormalizeOrUp(Subtract(pointM, axisPointM));
                outHit.distanceM = tM;
                return true;
            }
        }
    }

    // 몸통에 안 맞았으면 양 끝 반구를 각각 검사해서 더 가까운 쪽을 결과로 씀.
    RaycastHit startCapHit{};
    RaycastHit endCapHit{};
    const bool hasStartCap = RaycastSphereCore(ray, segStartM, radiusM, startCapHit);
    const bool hasEndCap = RaycastSphereCore(ray, segEndM, radiusM, endCapHit);

    if (hasStartCap && (!hasEndCap || startCapHit.distanceM <= endCapHit.distanceM))
    {
        outHit = startCapHit;
        return true;
    }
    if (hasEndCap)
    {
        outHit = endCapHit;
        return true;
    }

    return false;
}

bool RaycastTerrain(const Ray& ray, const TerrainSurface& terrain, RaycastHit& outHit) noexcept
{
    outHit = {};
    if (terrain.sampler == nullptr || ray.maxDistanceM <= 0.0F)
    {
        return false;
    }

    // 지형은 heightmap이라 닫힌 식이 없어서, 일정 간격(STEP_M)으로 전진하며
    // "ray 높이 - 지형 높이" 부호가 바뀌는 구간(양수 -> 0 이하)을 지형을 뚫고 들어간 지점으로 봄.
    constexpr float STEP_M = 0.25F;

    float prevTM = 0.0F;
    TerrainSample prevSample{};
    if (!terrain.sampler->SampleHeightAtWorld(ray.originM, prevSample))
    {
        return false;
    }
    float prevDiffM = ray.originM.y - prevSample.heightM;

    for (float tM = STEP_M;; tM += STEP_M)
    {
        const bool isLastStep = tM >= ray.maxDistanceM;
        const float clampedTM = isLastStep ? ray.maxDistanceM : tM;
        const Vec3 pointM = Add(ray.originM, Scale(ray.directionM, clampedTM));

        TerrainSample sample{};
        if (!terrain.sampler->SampleHeightAtWorld(pointM, sample))
        {
            // 지형 범위를 벗어나면 그 뒤로는 더 진행해도 의미가 없음.
            return false;
        }

        const float diffM = pointM.y - sample.heightM;
        if (diffM <= 0.0F)
        {
            // 이전 지점(양수)과 현재 지점(0 이하) 사이를 선형 보간해 교차 지점을 근사함.
            const float denomM = prevDiffM - diffM;
            const float ratio = (denomM > Settings::MIN_COLLIDER_SIZE_M) ? (prevDiffM / denomM) : 0.0F;
            const float hitTM = prevTM + (clampedTM - prevTM) * ratio;

            outHit.hit = true;
            outHit.pointM = Add(ray.originM, Scale(ray.directionM, hitTM));
            outHit.normal = sample.normal;
            outHit.distanceM = hitTM;
            return true;
        }

        if (isLastStep)
        {
            return false;
        }

        prevTM = clampedTM;
        prevDiffM = diffM;
    }
}

bool RaycastCollisionBody(const Ray& ray, const CollisionBody& body, RaycastHit& outHit) noexcept
{
    const bool hit = std::visit(
        [&ray, &outHit](const auto& shape) -> bool
        {
            using ShapeType = std::decay_t<decltype(shape)>;
            if constexpr (std::is_same_v<ShapeType, Box>)
            {
                return RaycastBox(ray, shape, outHit);
            }
            else if constexpr (std::is_same_v<ShapeType, Capsule>)
            {
                return RaycastCapsule(ray, shape, outHit);
            }
            else if constexpr (std::is_same_v<ShapeType, TerrainSurface>)
            {
                return RaycastTerrain(ray, shape, outHit);
            }
            else
            {
                // Sphere/Cylinder/Ramp는 아직 발사 판정에 쓰지 않으므로, 정밀한 식 대신
                // 감싸는 AABB(BuildAabb)로 보수적으로만 검사.
                const Aabb aabbM = CollisionQueries::BuildAabb(shape);
                return RaycastAabb(ray, aabbM.centerM, aabbM.extentsM, outHit);
            }
        },
        body.shape);

    if (hit)
    {
        outHit.objectId = body.objectId;
    }
    return hit;
}

bool RaycastClosest(const Ray& ray,
                     const std::vector<CollisionBody>& bodies,
                     ObjectId ignoredObjectId,
                     RaycastHit& outHit)
{
    outHit = {};
    bool foundAny = false;

    for (const CollisionBody& body : bodies)
    {
        if (body.objectId == ignoredObjectId)
        {
            continue;
        }

        RaycastHit candidateHit{};
        if (!RaycastCollisionBody(ray, body, candidateHit))
        {
            continue;
        }

        if (!foundAny || candidateHit.distanceM < outHit.distanceM)
        {
            outHit = candidateHit;
            foundAny = true;
        }
    }

    return foundAny;
}
} // namespace Kimgane::Shared::Physics::RaycastQueries
