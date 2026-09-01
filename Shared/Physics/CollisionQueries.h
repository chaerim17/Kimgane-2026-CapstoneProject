#pragma once

#include "CollisionTypes.h"

// 상태 없는 충돌 함수입니다.
// 위치 보정 정책은 클라/서버가 다르므로 여기서는 겹침 여부와 ContactInfo만 반환합니다.
namespace Kimgane::Shared::Physics::CollisionQueries
{
/// 방향 벡터를 단위 벡터로 바꿉니다. 길이가 거의 0이면 안정성을 위해 world up을 반환합니다.
[[nodiscard]] Vec3 NormalizeOrUp(const Vec3& value) noexcept;

/// 세로 capsule의 아래쪽 반구 중심을 반환합니다.
[[nodiscard]] Vec3 GetCapsuleSegmentStartM(const Capsule& capsule) noexcept;

/// 세로 capsule의 위쪽 반구 중심을 반환합니다.
[[nodiscard]] Vec3 GetCapsuleSegmentEndM(const Capsule& capsule) noexcept;

/// 지형 접지 검사에 쓰는 capsule 최하단 Y 좌표를 반환합니다.
[[nodiscard]] float GetCapsuleBottomM(const Capsule& capsule) noexcept;

/// 지형/램프 접지 검사에 쓰는 sphere 최하단 Y 좌표를 반환합니다.
[[nodiscard]] float GetSphereBottomM(const Sphere& sphere) noexcept;

/// 지형/램프 접지 검사에 쓰는 cylinder 최하단 Y 좌표를 반환합니다.
[[nodiscard]] float GetCylinderBottomM(const Cylinder& cylinder) noexcept;

/// Box를 broad-phase용 AABB로 변환합니다.
[[nodiscard]] Aabb BuildAabb(const Box& box) noexcept;

/// Sphere를 broad-phase용 AABB로 변환합니다.
[[nodiscard]] Aabb BuildAabb(const Sphere& sphere) noexcept;

/// Capsule을 broad-phase용 AABB로 변환합니다.
[[nodiscard]] Aabb BuildAabb(const Capsule& capsule) noexcept;

/// Cylinder를 broad-phase용 AABB로 변환합니다.
[[nodiscard]] Aabb BuildAabb(const Cylinder& cylinder) noexcept;

/// Ramp를 broad-phase용 AABB로 변환합니다.
[[nodiscard]] Aabb BuildAabb(const Ramp& ramp) noexcept;

/// Ramp 표면 normal을 반환합니다. +Y가 위쪽이고 direction은 오르막 방향입니다.
[[nodiscard]] Vec3 GetRampSurfaceNormal(const Ramp& ramp) noexcept;

/// 두 AABB가 겹치는지 검사합니다. 좁은 단계 검사 전에 빠른 필터로 사용합니다.
[[nodiscard]] bool Intersects(const Aabb& lhs, const Aabb& rhs) noexcept;

/// Capsule끼리 겹치면 true를 반환하고, outContact에 A에서 B를 향하는 normal과 깊이를 채웁니다.
[[nodiscard]] bool CheckCapsuleCapsule(const Capsule& lhs,
                                       const Capsule& rhs,
                                       ContactInfo& outContact) noexcept;

/// Sphere끼리 겹치면 true를 반환합니다.
[[nodiscard]] bool CheckSphereSphere(const Sphere& lhs,
                                     const Sphere& rhs,
                                     ContactInfo& outContact) noexcept;

/// Capsule과 Sphere가 겹치면 true를 반환합니다.
[[nodiscard]] bool CheckCapsuleSphere(const Capsule& capsule,
                                      const Sphere& sphere,
                                      ContactInfo& outContact) noexcept;

/// Sphere-Capsule 순서로 검사합니다.
[[nodiscard]] bool CheckSphereCapsule(const Sphere& sphere,
                                      const Capsule& capsule,
                                      ContactInfo& outContact) noexcept;

/// Capsule과 Box가 겹치면 true를 반환하고, outContact는 capsule을 objectA로 보는 방향을 사용합니다.
[[nodiscard]] bool CheckCapsuleBox(const Capsule& capsule,
                                   const Box& box,
                                   ContactInfo& outContact) noexcept;

/// Box와 Capsule이 겹치면 true를 반환하고, outContact는 box를 objectA로 보는 방향을 사용합니다.
[[nodiscard]] bool CheckBoxCapsule(const Box& box,
                                   const Capsule& capsule,
                                   ContactInfo& outContact) noexcept;

/// Sphere와 Box가 겹치면 true를 반환합니다.
[[nodiscard]] bool CheckSphereBox(const Sphere& sphere,
                                  const Box& box,
                                  ContactInfo& outContact) noexcept;

/// Box-Sphere 순서로 검사합니다.
[[nodiscard]] bool CheckBoxSphere(const Box& box,
                                  const Sphere& sphere,
                                  ContactInfo& outContact) noexcept;

/// Cylinder끼리 겹치면 true를 반환합니다.
[[nodiscard]] bool CheckCylinderCylinder(const Cylinder& lhs,
                                         const Cylinder& rhs,
                                         ContactInfo& outContact) noexcept;

/// Cylinder와 Sphere가 겹치면 true를 반환합니다.
[[nodiscard]] bool CheckCylinderSphere(const Cylinder& cylinder,
                                       const Sphere& sphere,
                                       ContactInfo& outContact) noexcept;

/// Sphere-Cylinder 순서로 검사합니다.
[[nodiscard]] bool CheckSphereCylinder(const Sphere& sphere,
                                       const Cylinder& cylinder,
                                       ContactInfo& outContact) noexcept;

/// Cylinder와 Capsule이 겹치면 true를 반환합니다.
[[nodiscard]] bool CheckCylinderCapsule(const Cylinder& cylinder,
                                        const Capsule& capsule,
                                        ContactInfo& outContact) noexcept;

/// Capsule-Cylinder 순서로 검사합니다.
[[nodiscard]] bool CheckCapsuleCylinder(const Capsule& capsule,
                                        const Cylinder& cylinder,
                                        ContactInfo& outContact) noexcept;

/// Cylinder와 Box가 겹치면 true를 반환합니다.
[[nodiscard]] bool CheckCylinderBox(const Cylinder& cylinder,
                                    const Box& box,
                                    ContactInfo& outContact) noexcept;

/// Box-Cylinder 순서로 검사합니다.
[[nodiscard]] bool CheckBoxCylinder(const Box& box,
                                    const Cylinder& cylinder,
                                    ContactInfo& outContact) noexcept;

/// Ramp와 Capsule이 겹치면 true를 반환합니다. Ramp는 walkable surface contact로 표시됩니다.
[[nodiscard]] bool CheckRampCapsule(const Ramp& ramp,
                                    const Capsule& capsule,
                                    ContactInfo& outContact) noexcept;

/// Capsule-Ramp 순서로 검사합니다. normal 방향은 capsule을 objectA로 맞춰 반환합니다.
[[nodiscard]] bool CheckCapsuleRamp(const Capsule& capsule,
                                    const Ramp& ramp,
                                    ContactInfo& outContact) noexcept;

/// Ramp와 Sphere가 겹치면 true를 반환합니다.
[[nodiscard]] bool CheckRampSphere(const Ramp& ramp,
                                   const Sphere& sphere,
                                   ContactInfo& outContact) noexcept;

/// Sphere-Ramp 순서로 검사합니다.
[[nodiscard]] bool CheckSphereRamp(const Sphere& sphere,
                                   const Ramp& ramp,
                                   ContactInfo& outContact) noexcept;

/// Ramp와 Cylinder가 겹치면 true를 반환합니다.
[[nodiscard]] bool CheckRampCylinder(const Ramp& ramp,
                                     const Cylinder& cylinder,
                                     ContactInfo& outContact) noexcept;

/// Cylinder-Ramp 순서로 검사합니다.
[[nodiscard]] bool CheckCylinderRamp(const Cylinder& cylinder,
                                     const Ramp& ramp,
                                     ContactInfo& outContact) noexcept;

/// TerrainSampler에서 높이/normal을 받아 capsule 바닥 접촉과 경사 정보를 계산합니다.
[[nodiscard]] bool CheckTerrainCapsule(const TerrainSurface& terrain,
                                       const Capsule& capsule,
                                       ContactInfo& outContact) noexcept;

/// Capsule-Terrain 순서로 검사합니다. normal 방향은 capsule을 objectA로 맞춰 반환합니다.
[[nodiscard]] bool CheckCapsuleTerrain(const Capsule& capsule,
                                       const TerrainSurface& terrain,
                                       ContactInfo& outContact) noexcept;

/// TerrainSampler에서 높이/normal을 받아 sphere 바닥 접촉을 계산합니다.
[[nodiscard]] bool CheckTerrainSphere(const TerrainSurface& terrain,
                                      const Sphere& sphere,
                                      ContactInfo& outContact) noexcept;

/// Sphere-Terrain 순서로 검사합니다.
[[nodiscard]] bool CheckSphereTerrain(const Sphere& sphere,
                                      const TerrainSurface& terrain,
                                      ContactInfo& outContact) noexcept;

/// TerrainSampler에서 높이/normal을 받아 cylinder 바닥 접촉을 계산합니다.
[[nodiscard]] bool CheckTerrainCylinder(const TerrainSurface& terrain,
                                        const Cylinder& cylinder,
                                        ContactInfo& outContact) noexcept;

/// Cylinder-Terrain 순서로 검사합니다.
[[nodiscard]] bool CheckCylinderTerrain(const Cylinder& cylinder,
                                        const TerrainSurface& terrain,
                                        ContactInfo& outContact) noexcept;

/// CollisionBody의 shape variant를 보고 지원하는 조합에 맞는 검사 함수를 호출합니다.
[[nodiscard]] bool CheckCollision(const CollisionBody& lhs,
                                  const CollisionBody& rhs,
                                  ContactInfo& outContact) noexcept;
} // namespace Kimgane::Shared::Physics::CollisionQueries
