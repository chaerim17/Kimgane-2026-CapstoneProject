#include "ServerTerrainCalculation.h"
#include "../Core/Session.h"
#include "../../../../Shared/Physics/CharacterMovementWorld.h"
#include "../../../../Shared/World/TestMapCollision.h"

#include <cmath>

namespace
{
namespace Physics = Kimgane::Shared::Physics;
namespace Raycast = Physics::RaycastQueries;
}

namespace ServerTerrainCalculation
{
std::shared_ptr<TerrainHeightMap> LoadTerrain()
{
    return Kimgane::Shared::World::LoadTestMapTerrain();
}

void UpdateCharacter(Session& session, int objectId,
    const Physics::CollisionWorld& collisionWorld, float deltaTimeSec)
{
    Physics::CharacterMotionInput input = {};
    // 이동 yaw와 시선 yaw는 별개입니다. 반대 키를 같이 누르면 해당 축 입력을 상쇄합니다.
    const bool hasMovement = session.mMoveUp != session.mMoveDown ||
                             session.mMoveRight != session.mMoveLeft;
    if (hasMovement)
    {
        input.direction = {std::sin(session.mMoveYaw), 0.0F, std::cos(session.mMoveYaw)};
    }
    input.jumpRequested = session.mJumpRequested;
    session.mJumpRequested = false;
    Physics::StepCharacterMovementInWorld(session.mMovementState, input,
        deltaTimeSec, objectId, collisionWorld);

    // 계산이 끝난 위치를 기존 Session 송신 필드에 반영합니다.
    session.mX = session.mMovementState.positionM.x;
    session.mY = session.mMovementState.positionM.y;
    session.mZ = session.mMovementState.positionM.z;
}

bool BlocksShot(const Physics::TerrainSampler& terrain, const Raycast::Ray& ray)
{
    Physics::TerrainSample originSample{};
    if (!terrain.SampleHeightAtWorld(ray.originM, originSample) || ray.originM.y <= originSample.heightM)
        return true;
    Raycast::RaycastHit terrainHit{};
    return Raycast::RaycastTerrain(ray, Physics::TerrainSurface{&terrain}, terrainHit);
}
}
