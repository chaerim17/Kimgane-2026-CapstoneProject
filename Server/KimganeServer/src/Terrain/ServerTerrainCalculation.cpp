#include "ServerTerrainCalculation.h"
#include "../Core/Session.h"
#include "../../../../Shared/Terrain/TerrainConfig.h"
#include "../../../../Shared/Physics/CharacterMovement.h"

namespace
{
namespace Physics = Kimgane::Shared::Physics;
namespace Raycast = Physics::RaycastQueries;

// 서버 높이맵의 중앙 원점 좌표를 Shared 지형 조회 인터페이스에 연결합니다.
class ShootTerrainSampler final : public Physics::TerrainSampler
{
public:
    explicit ShootTerrainSampler(const TerrainHeightMap& terrain) : mTerrain(terrain) {}

    bool SampleHeightAtWorld(const Physics::Vec3& position, Physics::TerrainSample& sample) const noexcept override
    {
        const float x = position.x + mTerrain.GetWorldWidthM() * 0.5F;
        const float z = position.z + mTerrain.GetWorldLengthM() * 0.5F;
        if (!mTerrain.ContainsSamplePositionM(x, z))
            return false;
        sample.heightM = mTerrain.SampleHeightM(x, z);
        return true;
    }

private:
    const TerrainHeightMap& mTerrain;
};
}

namespace ServerTerrainCalculation
{
using namespace Kimgane::Shared::Physics;

std::shared_ptr<TerrainHeightMap> LoadTerrain()
{
    return TerrainHeightMap::LoadRawAuto(TerrainConfig::TERRAIN_RAW_PATH,
        TerrainConfig::CELL_SPACING, TerrainConfig::HEIGHT_SCALE);
}

std::vector<Box> BuildGroundBoxes(
    std::span<const Kimgane::Shared::Geometry::NamedCollisionBox> collisionBoxes,
    float worldOffsetY)
{
    // 서버가 로드한 충돌 박스를 Shared 계산에 필요한 월드 좌표로 변환 (y값 보정해주기)
    std::vector<Box> groundBoxes;
    groundBoxes.reserve(collisionBoxes.size());
    for (const auto& collisionBox : collisionBoxes)
    {
        Box worldBox = collisionBox.box;
        worldBox.centerM.y += worldOffsetY;
        groundBoxes.push_back(worldBox);
    }
    return groundBoxes;
}

float UpdateHorizontal(Session& session, int objectId, const TerrainHeightMap& terrain,
    const CollisionWorld& collisionWorld, std::span<const Box> groundBoxes,
    float moveSpeed, float deltaTime)
{
    CharacterMovementState state{{session.mX, session.mY, session.mZ},
                                 session.mVelocityY, session.mIsJumping};
    const CharacterMovementInput input{session.mMoveYaw, session.mMoveUp, session.mMoveDown,
                                       session.mMoveRight, session.mMoveLeft};
    // 지형 높이 조회
    const float sampleX = state.positionM.x + terrain.GetWorldWidthM() * 0.5f;
    const float sampleZ = state.positionM.z + terrain.GetWorldLengthM() * 0.5f;
    const float terrainHeight = terrain.SampleHeightM(sampleX, sampleZ);

    const float groundHeight = StepCharacterHorizontalMovement(
        state, input, objectId, collisionWorld, terrainHeight, groundBoxes, moveSpeed, deltaTime);

    // 계산 위치 세션에 반영
    session.mX = state.positionM.x;
    session.mY = state.positionM.y;
    session.mZ = state.positionM.z;

    return groundHeight;
}

void UpdateVertical(Session& session, float groundHeight, float gravity, float deltaTime)
{
    CharacterMovementState state{{session.mX, session.mY, session.mZ},
                                 session.mVelocityY, session.mIsJumping};
    StepCharacterVerticalMovement(state, groundHeight, gravity, deltaTime);
    session.mY = state.positionM.y;
    session.mVelocityY = state.velocityYMps;
    session.mIsJumping = state.isJumping;
}

bool BlocksShot(const TerrainHeightMap& terrain, const Raycast::Ray& ray)
{
    const ShootTerrainSampler sampler(terrain);
    Physics::TerrainSample originSample{};
    if (!sampler.SampleHeightAtWorld(ray.originM, originSample) || ray.originM.y <= originSample.heightM)
        return true;
    Raycast::RaycastHit terrainHit{};
    return Raycast::RaycastTerrain(ray, Physics::TerrainSurface{&sampler}, terrainHit);
}
}
