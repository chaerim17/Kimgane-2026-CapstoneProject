#include "CollisionWorld.h"

#include "CollisionQueries.h"
#include "CollisionResolver.h"

#include <algorithm>

namespace Kimgane::Shared::Physics
{
void CollisionWorld::Clear() noexcept
{
    mBodies.clear();
}

bool CollisionWorld::AddOrUpdateBody(const CollisionBody& body)
{
    // id가 없으면 자기 자신의 이전 body 제외나 갱신이 불가능하므로 등록하지 않습니다.
    if (body.objectId == INVALID_OBJECT_ID)
    {
        return false;
    }

    auto iter = std::find_if(mBodies.begin(), mBodies.end(), [body](const CollisionBody& existing) {
        return existing.objectId == body.objectId;
    });
    if (iter != mBodies.end())
    {
        // 후보 이동이 허용된 뒤 저장된 body를 새 위치로 교체합니다.
        *iter = body;
        return true;
    }

    // 신규 player/NPC/static body 등록 경로입니다.
    mBodies.push_back(body);
    return true;
}

bool CollisionWorld::RemoveBody(ObjectId objectId) noexcept
{
    const auto oldSize = mBodies.size();
    mBodies.erase(std::remove_if(mBodies.begin(),
                                 mBodies.end(),
                                 [objectId](const CollisionBody& body) {
                                     return body.objectId == objectId;
                                 }),
                  mBodies.end());
    return mBodies.size() != oldSize;
}

std::size_t CollisionWorld::GetBodyCount() const noexcept
{
    return mBodies.size();
}

const CollisionBody* CollisionWorld::FindBody(ObjectId objectId) const noexcept
{
    const auto iter = std::find_if(mBodies.begin(), mBodies.end(), [objectId](const CollisionBody& body) {
        return body.objectId == objectId;
    });
    return iter != mBodies.end() ? &(*iter) : nullptr;
}

bool CollisionWorld::ContainsBody(ObjectId objectId) const noexcept
{
    return FindBody(objectId) != nullptr;
}

std::vector<ContactInfo> CollisionWorld::QueryContacts(const CollisionBody& body, ObjectId ignoredObjectId) const
{
    std::vector<ContactInfo> contacts;
    for (const CollisionBody& other : mBodies)
    {
        // 후보 이동 검사에서는 ignoredObjectId로 자기 자신의 이전 body를 제외합니다.
        if (other.objectId == body.objectId || other.objectId == ignoredObjectId || !ShouldCollide(body, other))
        {
            continue;
        }

        ContactInfo contact = {};
        if (CollisionQueries::CheckCollision(body, other, contact))
        {
            // QueryContacts는 발판/포탈처럼 막지 않는 trigger contact도 모을 수 있게 둡니다.
            contacts.push_back(contact);
        }
    }

    return contacts;
}

bool CollisionWorld::QueryContacts(ObjectId objectId, std::vector<ContactInfo>& outContacts) const
{
    outContacts.clear();

    const CollisionBody* body = FindBody(objectId);
    if (body == nullptr)
    {
        return false;
    }

    outContacts = QueryContacts(*body);
    return !outContacts.empty();
}

bool CollisionWorld::HasBlockingContact(const CollisionBody& body, ObjectId ignoredObjectId) const
{
    // 이동 차단용 빠른 경로라 첫 blocking contact에서 바로 종료합니다.
    for (const CollisionBody& other : mBodies)
    {
        if (other.objectId == body.objectId || other.objectId == ignoredObjectId || body.isTrigger || other.isTrigger ||
            !ShouldCollide(body, other))
        {
            continue;
        }

        ContactInfo contact = {};
        if (CollisionQueries::CheckCollision(body, other, contact) &&
            CollisionResolver::ShouldBlockMovement(contact))
        {
            return true;
        }
    }

    return false;
}

bool CollisionWorld::ShouldCollide(const CollisionBody& lhs, const CollisionBody& rhs) noexcept
{
    // 한쪽만 허용한 충돌을 피하기 위해 양쪽 mask를 모두 확인합니다.
    return (lhs.collidesWithMask & rhs.layerMask) != 0U && (rhs.collidesWithMask & lhs.layerMask) != 0U;
}
} // namespace Kimgane::Shared::Physics
