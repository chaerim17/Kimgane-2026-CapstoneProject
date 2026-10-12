#pragma once

#include "../Pch.h"
#include "../Config/ServerConfig.h"
#include <chrono>
#include "../World/GameObject.h"
#include "../../../../Shared/Physics/CharacterMovement.h"
#include "NpcHitDetection.h"

class Npc final : public GameObject
{
public:
    explicit Npc(int id) : GameObject(id, ObjectType::Npc),
        mMovementState(Kimgane::Shared::Physics::MakePlayerMovementState({})) {}
    [[nodiscard]] const Vec3& GetPositionM() const noexcept override { return mMovementState.positionM; }
    [[nodiscard]] float GetYawRad() const noexcept override { return mYaw; }

    Kimgane::Shared::Physics::RigidbodyState mMovementState;
    Vec3 mMoveDirection = {};
    float mYaw = 0.0F;

    float mMoveSpeed = 2.0f;
    
    int mMaxHp{NPC_MAX_HP};
    int mCurrentHp{mMaxHp};

    std::chrono::steady_clock::time_point mLastMove;

    // 방향만 선택하고 위치 적분은 공통 movement에서 수행합니다.
    void RandomMove();
    void UpdateMovement(const Kimgane::Shared::Physics::CollisionWorld& world, float deltaTimeSec);
    void ResolveSpawnContacts(const Kimgane::Shared::Physics::CollisionWorld& world);
    // 이동 캡슐과 별도인 사격 전용 로컬 박스를 검사
    [[nodiscard]] bool RaycastHitBoxes(
        const Kimgane::Shared::Physics::RaycastQueries::Ray& worldRay, NpcShotHit& hit) const;
};

