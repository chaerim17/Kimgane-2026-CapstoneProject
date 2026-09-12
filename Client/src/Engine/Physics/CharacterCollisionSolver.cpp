#include "Pch.h"

#include "CharacterCollisionSolver.h"

#include "../Core/GameObject.h"
#include "ColliderComponent.h"
#include "CollisionManager.h"
#include "RigidbodyComponent.h"
#include "../../Shared/Physics/CharacterMovement.h"

namespace Kimgane::Engine
{
namespace
{
namespace SharedPhysics = Kimgane::Shared::Physics;

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

// Shared가 요청한 후보 위치를 클라이언트 형상에 적용하고 접촉을 값으로 반환합니다.
// 환경을 ObjectA, 캐릭터를 ObjectB로 조회하는 순서는 Shared 보정 규약과 같습니다.
class ClientCharacterContactQuery final : public SharedPhysics::CharacterContactQuery
{
public:
    ClientCharacterContactQuery(CapsuleColliderComponent& collider,
                                const CollisionManager& collisionManager,
                                const std::vector<ColliderComponent*>& collisionTargets)
        : mCollider(collider), mCollisionManager(collisionManager), mCollisionTargets(collisionTargets)
    {
    }

    void QueryContacts(const SharedPhysics::Vec3& positionM,
                       std::vector<SharedPhysics::ContactInfo>& outContacts) override
    {
        outContacts.clear();
        mCollider.GetOwner().GetTransform().SetPositionM(ToXMFloat3(positionM));
        mCollider.Update(0.0F);
        for (ColliderComponent* target : mCollisionTargets)
        {
            if (target == nullptr || target == &mCollider || !target->GetOwner().IsActive())
            {
                continue;
            }
            target->Update(0.0F);
            ContactInfo contact = {};
            if (mCollisionManager.CheckCollision(*target, mCollider, contact))
            {
                outContacts.push_back(ToSharedContact(contact));
            }
        }
    }

private:
    CapsuleColliderComponent& mCollider;
    const CollisionManager& mCollisionManager;
    const std::vector<ColliderComponent*>& mCollisionTargets;
};

void UpdateCharacter(GameObject& character,
                     const CollisionManager& collisionManager,
                     const std::vector<ColliderComponent*>& collisionTargets,
                     const SharedPhysics::CharacterMotionInput* input,
                     float deltaTimeSec)
{
    auto* collider = character.GetComponent<CapsuleColliderComponent>();
    if (collider == nullptr)
    {
        return;
    }

    auto* rigidbody = character.GetComponent<RigidbodyComponent>();
    // Stepは外部更新に切り替えたRigidbody専用です。二重積分を防ぎます。
    if (input != nullptr && (rigidbody == nullptr || rigidbody->IsAutomaticIntegrationEnabled()))
    {
        return;
    }

    SharedPhysics::RigidbodyState state = rigidbody != nullptr
        ? rigidbody->GetSharedState() : SharedPhysics::RigidbodyState{};
    state.positionM = ToSharedVec3(character.GetTransform().GetPositionM());
    ClientCharacterContactQuery query(*collider, collisionManager, collisionTargets);
    if (input != nullptr)
    {
        SharedPhysics::StepCharacterMovement(state, *input, deltaTimeSec, query);
    }
    else
    {
        SharedPhysics::ResolveCharacterContacts(state, query);
    }

    // Sharedの最終状態を反映します。Rigidbodyの設定と積分履歴もstateに保持されています。
    if (rigidbody != nullptr)
    {
        rigidbody->SetSharedState(state);
    }
    else
    {
        character.GetTransform().SetPositionM(ToXMFloat3(state.positionM));
    }
    collider->Update(0.0F);
}
} // namespace

void CharacterCollisionSolver::Solve(GameObject& character,
                                     const CollisionManager& collisionManager,
                                     const std::vector<ColliderComponent*>& collisionTargets)
{
    UpdateCharacter(character, collisionManager, collisionTargets, nullptr, 0.0F);
}

void CharacterCollisionSolver::Step(GameObject& character,
                                    const CollisionManager& collisionManager,
                                    const std::vector<ColliderComponent*>& collisionTargets,
                                    const SharedPhysics::CharacterMotionInput& input,
                                    float deltaTimeSec)
{
    UpdateCharacter(character, collisionManager, collisionTargets, &input, deltaTimeSec);
}
} // namespace Kimgane::Engine
