#include <random>
#include "Npc.h"
#include <cmath>
#include "NpcHitDetection.h"

namespace
{
namespace Physics = Kimgane::Shared::Physics;

// 캡슐과 레이어를 연결하는 서버 어댑터. 계산은 기존 Shared 구현을 사용합니다.
class NpcContactQuery final : public Physics::CharacterContactQuery
{
public:
    NpcContactQuery(const Physics::CollisionWorld& world, Physics::ObjectId id)
        : mWorld(world), mId(id) {}

    void QueryContacts(const Physics::Vec3& positionM,
        std::vector<Physics::ContactInfo>& contacts) override
    {
        contacts = mWorld.QueryCharacterContacts({mId,
            Physics::MakeCapsuleFromFootPosition(positionM,
                Physics::Settings::NPC_CAPSULE_RADIUS_M, Physics::Settings::NPC_CAPSULE_HEIGHT_M),
            Physics::CollisionLayer::NPC, Physics::CollisionLayer::ALL, false});
    }

private:
    const Physics::CollisionWorld& mWorld;
    Physics::ObjectId mId;
};
}

void Npc::UpdateMovement(const Physics::CollisionWorld& world, float deltaTimeSec)
{
    Physics::CharacterMotionInput input{};
    input.direction = mMoveDirection;
    input.moveSpeedMps = mMoveSpeed;
    NpcContactQuery query(world, GetId());
    Physics::StepCharacterMovement(mMovementState, input, deltaTimeSec, query);
}

void Npc::ResolveSpawnContacts(const Physics::CollisionWorld& world)
{
    NpcContactQuery query(world, GetId());
    Physics::ResolveCharacterContacts(mMovementState, query);
}

void Npc::RandomMove()
{
    static std::mt19937 rng(std::random_device{}());

    // 위치를 직접 변경하지 않고 다음 이동 방향만 선택합니다.
    std::uniform_real_distribution<float> yawDist(-3.14159265F, 3.14159265F);
    mYaw = yawDist(rng);
    mMoveDirection = {std::sin(mYaw), 0.0F, std::cos(mYaw)};
}

bool Npc::RaycastHitBoxes(const Physics::RaycastQueries::Ray& worldRay, NpcShotHit& hit) const
{
    return NpcHitDetection::RaycastHitBoxes(worldRay, GetPositionM(), mYaw,
        Kimgane::Shared::Npc::HUMANOID_HIT_BOXES, hit);
}
