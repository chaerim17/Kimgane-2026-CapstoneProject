#pragma once

#include "CollisionTypes.h"

#include <cstddef>
#include <vector>

// objectId 기반 충돌 registry입니다.
// 후보 위치가 막히는지만 알려주고, 실제 위치 확정은 클라/서버 gameplay 코드가 결정합니다.
namespace Kimgane::Shared::Physics
{
class CollisionWorld final
{
public:
    /// 등록된 모든 body를 제거합니다. 맵 전환이나 테스트 초기화 때 사용합니다.
    void Clear() noexcept;

    /// objectId가 같으면 갱신하고, 없으면 새 body로 등록합니다.
    bool AddOrUpdateBody(const CollisionBody& body);

    /// despawn/disconnect된 object의 body를 제거합니다. 제거되면 true를 반환합니다.
    bool RemoveBody(ObjectId objectId) noexcept;

    /// 현재 등록된 body 개수를 반환합니다.
    [[nodiscard]] std::size_t GetBodyCount() const noexcept;

    /// objectId로 body를 찾습니다. 반환 포인터는 world가 수정되면 무효화될 수 있습니다.
    [[nodiscard]] const CollisionBody* FindBody(ObjectId objectId) const noexcept;

    /// objectId에 해당하는 body가 등록되어 있는지 확인합니다.
    [[nodiscard]] bool ContainsBody(ObjectId objectId) const noexcept;

    /// 후보 body와 world에 등록된 body들의 충돌을 조회합니다. ignoredObjectId는 예측 이동 때 제외합니다.
    [[nodiscard]] std::vector<ContactInfo> QueryContacts(const CollisionBody& body,
                                                         ObjectId ignoredObjectId = INVALID_OBJECT_ID) const;

    /// 등록된 body 기준으로 충돌 목록을 조회합니다. 충돌이 하나라도 있으면 true를 반환합니다.
    [[nodiscard]] bool QueryContacts(ObjectId objectId, std::vector<ContactInfo>& outContacts) const;

    /// 이동 차단 여부만 빠르게 확인합니다. trigger와 걸을 수 있는 지형 접촉은 blocking으로 보지 않습니다.
    [[nodiscard]] bool HasBlockingContact(const CollisionBody& body,
                                          ObjectId ignoredObjectId = INVALID_OBJECT_ID) const;

private:
    /// 양쪽 layer mask가 서로 허용하는지 확인합니다.
    [[nodiscard]] static bool ShouldCollide(const CollisionBody& lhs, const CollisionBody& rhs) noexcept;

    // 현재 규모에서는 vector가 단순합니다. 오브젝트가 많아지면 내부만 spatial partition으로 바꿀 수 있습니다.
    std::vector<CollisionBody> mBodies;
};
} // namespace Kimgane::Shared::Physics
