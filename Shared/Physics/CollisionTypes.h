#pragma once

#include "PhysicsSettings.h"

#include <algorithm>
#include <cstdint>
#include <variant>

// Client와 Server가 같이 쓰는 순수 충돌 데이터입니다.
// Shared는 양쪽에서 include되므로 GameObject, DirectX, Session 같은 구현 의존성을 두지 않습니다.
namespace Kimgane::Shared::Physics
{
using ObjectId = std::int32_t;
using CollisionLayerMask = std::uint32_t;

// 실제 player, NPC, obstacle과 연결되지 않은 body/contact를 나타냅니다.
inline constexpr ObjectId INVALID_OBJECT_ID = -1;

// layerMask는 "내 종류", collidesWithMask는 "충돌할 대상"입니다.
// CollisionWorld는 양쪽 mask가 모두 허용할 때만 충돌을 검사합니다.
namespace CollisionLayer
{
inline constexpr CollisionLayerMask DEFAULT = 1U << 0U;
inline constexpr CollisionLayerMask PLAYER = 1U << 1U;
inline constexpr CollisionLayerMask NPC = 1U << 2U;
inline constexpr CollisionLayerMask STATIC_WORLD = 1U << 3U;
inline constexpr CollisionLayerMask TERRAIN = 1U << 4U;
inline constexpr CollisionLayerMask ALL = 0xFFFFFFFFU;
} // namespace CollisionLayer

// DirectX 의존을 피하기 위한 meter 단위 벡터입니다. +Y가 위쪽입니다.
struct Vec3
{
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
};

// 월드 축에 정렬된 box입니다. extentsM은 각 축의 half size입니다.
struct Aabb
{
    Vec3 centerM = {};
    Vec3 extentsM = {};
};

// 현재 서버 장애물은 단순 AABB로 충분해서 회전 박스는 별도 타입으로 미룹니다.
struct Box
{
    Vec3 centerM = {};
    Vec3 halfExtentsM = {};
};

// 캐릭터용 세로 capsule입니다.
// 서버 위치가 발/ground 기준이면 MakeCapsuleFromFootPosition()으로 center 기준에 맞춥니다.
struct Capsule
{
    Vec3 centerM = {};
    float radiusM = Settings::PLAYER_CAPSULE_RADIUS_M;
    float heightM = Settings::PLAYER_CAPSULE_HEIGHT_M;
};

// TerrainSampler가 반환하는 지형 높이와 표면 normal입니다.
struct TerrainSample
{
    float heightM = 0.0F;
    Vec3 normal = {0.0F, 1.0F, 0.0F};
};

// 클라/서버 지형 구현을 숨기기 위한 adapter입니다.
// Shared는 이 인터페이스만 보고 Terrain-Capsule 충돌을 재사용합니다.
class TerrainSampler
{
public:
    virtual ~TerrainSampler() = default;

    /// worldPositionM의 x/z 위치에서 지형 높이와 표면 normal을 가져옵니다.
    [[nodiscard]] virtual bool SampleHeightAtWorld(const Vec3& worldPositionM,
                                                   TerrainSample& outSample) const noexcept = 0;
};

// sampler는 비소유 포인터이므로 CollisionWorld보다 오래 살아야 합니다.
struct TerrainSurface
{
    const TerrainSampler* sampler = nullptr;
};

// 상속/RTTI 없이 shape를 값 타입으로 다루기 위해 std::variant를 사용합니다.
using CollisionShape = std::variant<Box, Capsule, TerrainSurface>;

// CollisionWorld에 등록되는 충돌 단위입니다.
// shape뿐 아니라 id/layer/trigger 상태가 필요해서 하나로 묶었습니다.
struct CollisionBody
{
    ObjectId objectId = INVALID_OBJECT_ID;
    CollisionShape shape = Box{};
    CollisionLayerMask layerMask = CollisionLayer::DEFAULT;
    CollisionLayerMask collidesWithMask = CollisionLayer::ALL;
    bool isTrigger = false;
};

// 충돌 결과입니다. 실제 이동 취소/보정은 서버 gameplay 로직에서 결정합니다.
// normal은 objectA에서 objectB 방향이고, penetrationM은 겹친 깊이입니다.
struct ContactInfo
{
    ObjectId objectA = INVALID_OBJECT_ID;
    ObjectId objectB = INVALID_OBJECT_ID;
    Vec3 normal = {0.0F, 1.0F, 0.0F};
    Vec3 surfaceNormal = {0.0F, 1.0F, 0.0F};
    float penetrationM = 0.0F;
    bool isTerrainContact = false;
    bool isWalkable = true;
    float slopeAngleRad = 0.0F;
};

/// 두 벡터를 더합니다.
[[nodiscard]] constexpr Vec3 Add(const Vec3& lhs, const Vec3& rhs) noexcept
{
    return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

/// lhs에서 rhs를 뺍니다.
[[nodiscard]] constexpr Vec3 Subtract(const Vec3& lhs, const Vec3& rhs) noexcept
{
    return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

/// 벡터에 스칼라 값을 곱합니다.
[[nodiscard]] constexpr Vec3 Scale(const Vec3& value, float scalar) noexcept
{
    return {value.x * scalar, value.y * scalar, value.z * scalar};
}

/// 두 벡터의 내적을 반환합니다.
[[nodiscard]] constexpr float Dot(const Vec3& lhs, const Vec3& rhs) noexcept
{
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

/// 벡터 길이의 제곱을 반환합니다. sqrt가 필요 없는 거리 비교에 사용합니다.
[[nodiscard]] constexpr float LengthSquared(const Vec3& value) noexcept
{
    return Dot(value, value);
}

/// 발 위치를 기준으로 저장된 서버 좌표를 center 기준 capsule로 변환합니다.
[[nodiscard]] constexpr Capsule MakeCapsuleFromFootPosition(const Vec3& footPositionM,
                                                            float radiusM,
                                                            float heightM) noexcept
{
    radiusM = std::max(radiusM, Settings::MIN_COLLIDER_SIZE_M);
    heightM = std::max(heightM, radiusM * 2.0F);
    return {{footPositionM.x, footPositionM.y + heightM * 0.5F, footPositionM.z}, radiusM, heightM};
}

/// 이미 capsule 중심을 알고 있는 클라/서버 코드에서 center 기준 capsule을 만듭니다.
[[nodiscard]] constexpr Capsule MakeCapsuleFromCenter(const Vec3& centerM, float radiusM, float heightM) noexcept
{
    radiusM = std::max(radiusM, Settings::MIN_COLLIDER_SIZE_M);
    heightM = std::max(heightM, radiusM * 2.0F);
    return {centerM, radiusM, heightM};
}
} // namespace Kimgane::Shared::Physics
