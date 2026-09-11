#include "Pch.h"

#include "CharacterCollisionSolver.h"

#include "../Core/GameObject.h"
#include "ColliderComponent.h"
#include "CollisionManager.h"
#include "RigidbodyComponent.h"
#include "../../Shared/Physics/CollisionResolver.h"

namespace Kimgane::Engine
{
namespace
{
namespace SharedPhysics = Kimgane::Shared::Physics;
namespace SharedCollisionResolver = Kimgane::Shared::Physics::CollisionResolver;

constexpr int LOCAL_PLAYER_COLLISION_SOLVER_ITERATIONS = 4;
constexpr float LOCAL_PLAYER_MIN_CORRECTION_SQ_M = 0.00000025F;
constexpr SharedCollisionResolver::PositionCorrectionSettings LOCAL_PLAYER_POSITION_CORRECTION_SETTINGS = {1.0F,
                                                                                                            0.001F};

SharedPhysics::Vec3 ToSharedVec3(const DirectX::XMFLOAT3& value) noexcept
{
    return {value.x, value.y, value.z};
}

DirectX::XMFLOAT3 ToXMFloat3(const SharedPhysics::Vec3& value) noexcept
{
    return {value.x, value.y, value.z};
}

SharedPhysics::ContactInfo ToSharedContact(const ContactInfo& contact) noexcept
{
    SharedPhysics::ContactInfo sharedContact = {};
    sharedContact.normal = ToSharedVec3(contact.normal);
    sharedContact.surfaceNormal = ToSharedVec3(contact.surfaceNormal);
    sharedContact.penetrationM = contact.penetrationM;
    sharedContact.isTerrainContact = contact.isTerrainContact;
    sharedContact.isGroundCandidate = contact.isGroundCandidate;
    sharedContact.isWalkable = contact.isWalkable;
    sharedContact.slopeAngleRad = contact.slopeAngleRad;
    return sharedContact;
}

bool IsPositionChanged(const SharedPhysics::Vec3& fromM, const SharedPhysics::Vec3& toM) noexcept
{
    return SharedPhysics::LengthSquared(SharedPhysics::Subtract(toM, fromM)) > LOCAL_PLAYER_MIN_CORRECTION_SQ_M;
}

// 접촉면 안으로 향하는 속도를 제거하고 접선 방향 속도를 남겨 슬라이딩을 허용합니다.
bool RemoveVelocityIntoResolvedContact(SharedPhysics::Vec3& velocityMps,
                                       const SharedPhysics::ContactInfo& contact) noexcept
{
    if (!SharedCollisionResolver::ShouldBlockMovement(contact) &&
        !SharedCollisionResolver::IsWalkableGround(contact))
    {
        return false;
    }

    const SharedPhysics::Vec3 slidVelocityMps =
        SharedCollisionResolver::SlideMovement(velocityMps,
                                               contact,
                                               SharedCollisionResolver::ContactParticipant::ObjectB);
    if (!IsPositionChanged(velocityMps, slidVelocityMps))
    {
        return false;
    }

    velocityMps = slidVelocityMps;
    return true;
}

// 보정 중인 Transform에 맞춰 충돌 형상을 갱신한 후 지정된 대상만 조회합니다.
// 환경을 ObjectA, 캐릭터를 ObjectB로 전달하며 보정 계산에서도 같은 순서를 사용합니다.
std::vector<ContactInfo> QueryContacts(CapsuleColliderComponent& playerCollider,
                                       const CollisionManager& collisionManager,
                                       const std::vector<ColliderComponent*>& collisionTargets)
{
    std::vector<ContactInfo> contacts;
    playerCollider.Update(0.0F);

    for (ColliderComponent* targetCollider : collisionTargets)
    {
        if (targetCollider == nullptr || targetCollider == &playerCollider || !targetCollider->GetOwner().IsActive())
        {
            continue;
        }

        targetCollider->Update(0.0F);
        ContactInfo contact = {};
        if (collisionManager.CheckCollision(*targetCollider, playerCollider, contact))
        {
            contacts.push_back(contact);
        }
    }

    return contacts;
}

} // namespace

void CharacterCollisionSolver::Solve(GameObject& character,
                                     const CollisionManager& collisionManager,
                                     const std::vector<ColliderComponent*>& collisionTargets)
{
    auto* playerCollider = character.GetComponent<CapsuleColliderComponent>();
    if (playerCollider == nullptr)
    {
        return;
    }

    // 컴포넌트 접근 경계: 현재 위치와 속도를 Shared 계산용 값으로 읽습니다.
    auto* playerRigidbody = character.GetComponent<RigidbodyComponent>();
    SharedPhysics::Vec3 resolvedPositionM = ToSharedVec3(character.GetTransform().GetPositionM());
    SharedPhysics::Vec3 resolvedVelocityMps =
        playerRigidbody != nullptr ? ToSharedVec3(playerRigidbody->GetVelocityMps()) : SharedPhysics::Vec3{};

    bool positionChanged = false;
    bool velocityChanged = false;
    bool groundedOnWalkableSurface = false;

    // 한 접촉의 보정이 다른 접촉을 바꿀 수 있어, 보정된 위치에서 최대 4회 다시 조회합니다.
    for (int iteration = 0; iteration < LOCAL_PLAYER_COLLISION_SOLVER_ITERATIONS; ++iteration)
    {
        character.GetTransform().SetPositionM(ToXMFloat3(resolvedPositionM));
        const std::vector<ContactInfo> contacts = QueryContacts(*playerCollider, collisionManager, collisionTargets);
        if (contacts.empty())
        {
            break;
        }

        bool iterationChanged = false;
        for (const ContactInfo& contact : contacts)
        {
            const SharedPhysics::ContactInfo sharedContact = ToSharedContact(contact);
            const bool isWalkableGround = SharedCollisionResolver::IsWalkableGround(sharedContact);
            const bool blocksMovement = SharedCollisionResolver::ShouldBlockMovement(sharedContact);
            if (!isWalkableGround && !blocksMovement)
            {
                continue;
            }

            // 기존 정책: 이번 Solve의 어느 반복에서든 보행 가능한 지면을 만나면 접지로 기록합니다.
            groundedOnWalkableSurface = groundedOnWalkableSurface || isWalkableGround;

            const SharedPhysics::Vec3 previousPositionM = resolvedPositionM;
            resolvedPositionM =
                SharedCollisionResolver::ResolvePosition(resolvedPositionM,
                                                         sharedContact,
                                                         SharedCollisionResolver::ContactParticipant::ObjectB,
                                                         LOCAL_PLAYER_POSITION_CORRECTION_SETTINGS);
            if (IsPositionChanged(previousPositionM, resolvedPositionM))
            {
                positionChanged = true;
                iterationChanged = true;
            }

            velocityChanged = RemoveVelocityIntoResolvedContact(resolvedVelocityMps, sharedContact) || velocityChanged;
        }

        if (!iterationChanged)
        {
            break;
        }
    }

    // 결과 반영 경계: 기존 Rigidbody 속성을 유지하면서 위치/속도/접지만 갱신합니다.
    // SetSharedState가 Transform도 갱신하므로 이후 콜라이더 형상을 동기화합니다.
    if (playerRigidbody != nullptr)
    {
        SharedPhysics::RigidbodyState state = playerRigidbody->GetSharedState();
        const bool groundedChanged = state.isGrounded != groundedOnWalkableSurface;
        if (positionChanged || velocityChanged || groundedChanged)
        {
            state.positionM = resolvedPositionM;
            state.velocityMps = resolvedVelocityMps;
            state.isGrounded = groundedOnWalkableSurface;
            playerRigidbody->SetSharedState(state);
            playerCollider->Update(0.0F);
        }

        return;
    }

    if (positionChanged || velocityChanged || groundedOnWalkableSurface)
    {
        character.GetTransform().SetPositionM(ToXMFloat3(resolvedPositionM));
        playerCollider->Update(0.0F);
    }
}

} // namespace Kimgane::Engine
