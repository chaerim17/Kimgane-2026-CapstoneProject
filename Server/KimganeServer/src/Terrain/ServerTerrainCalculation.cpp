#include "ServerTerrainCalculation.h"
#include "../World/Player.h"
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

void UpdateCharacter(Player& player,
    const Physics::CollisionWorld& collisionWorld, float deltaTimeSec)
{
    Physics::CharacterMotionInput input = {};
    // 이동 yaw와 시선 yaw는 별개입니다. 반대 키를 같이 누르면 해당 축 입력을 상쇄합니다.
    const bool hasMovement = player.mMoveUp != player.mMoveDown ||
                             player.mMoveRight != player.mMoveLeft;
    if (hasMovement)
    {
        input.direction = {std::sin(player.mMoveYaw), 0.0F, std::cos(player.mMoveYaw)};
    }
    input.jumpRequested = player.mJumpRequested;
    player.mJumpRequested = false;
    Physics::StepCharacterMovementInWorld(player.mMovementState, input,
        deltaTimeSec, player.GetId(), collisionWorld);
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
