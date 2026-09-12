// 공통 캐릭터 이동과 기존 서버 이동 API를 제공합니다.
#pragma once

#include "CollisionWorld.h"
#include "CollisionResolver.h"
#include "RigidbodyTypes.h"

#include <span>
#include <vector>

namespace Kimgane::Shared::Physics
{
// 카메라/키보드 처리를 마친 월드 XZ 방향입니다. 방향은 길이 1 이하로 전달합니다.
// jumpRequested는 누른 순간 한 번만 전달하며, 실제 점프 허용은 접지 상태로 결정합니다.
struct CharacterMotionInput
{
    Vec3 direction = {};
    float moveSpeedMps = 0.0F;
    float jumpVelocityMps = 0.0F;
    bool jumpRequested = false;
};

struct CharacterContactSettings
{
    int maxIterations = 4;
    float minPositionCorrectionSqM = 0.00000025F;
    float minVelocityChangeSqMps = 0.00000025F;
    CollisionResolver::PositionCorrectionSettings positionCorrection = {1.0F, 0.001F};
};

// 후보 위치에서 접촉을 다시 조회하는 환경별 연결부입니다.
// 환경을 ObjectA, 캐릭터를 ObjectB로 하는 접촉을 안정된 순서로 반환해야 합니다.
// 반환 대상에서 자기 자신과 트리거를 제외하는 책임은 구현체에 있습니다.
class CharacterContactQuery
{
public:
    virtual ~CharacterContactQuery() = default;
    virtual void QueryContacts(const Vec3& positionM, std::vector<ContactInfo>& outContacts) = 0;
};

// 적분이 끝난 상태의 위치/속도/접지를 보정합니다. 컴포넌트와 서버 객체에 의존하지 않습니다.
void ResolveCharacterContacts(RigidbodyState& state,
                              CharacterContactQuery& contactQuery,
                              const CharacterContactSettings& settings = {});

// 로컬 캐릭터 기준의 공통 순서: 수평 속도/점프 입력 → 적분 → 반복 충돌 보정.
// 이 함수를 호출하는 캐릭터에는 Rigidbody 적분을 별도로 적용하지 않습니다.
void StepCharacterMovement(RigidbodyState& state,
                           const CharacterMotionInput& input,
                           float deltaTimeSec,
                           CharacterContactQuery& contactQuery,
                           const CharacterContactSettings& settings = {});

// IsJumping은 점프 처리 활성 여부
struct CharacterMovementState
{
    Vec3 positionM = {};
    float velocityYMps = 0.0F;
    bool isJumping = false;
};

struct CharacterMovementInput
{
    float yawRad = 0.0F;
    bool moveUp = false;
    bool moveDown = false;
    bool moveRight = false;
    bool moveLeft = false;
};

// 이동/충돌을 계산해 state의 위치를 갱신
// 반환값은 점프/착지 계산에 사용할 바닥 높이
[[nodiscard]] float StepCharacterHorizontalMovement(
    CharacterMovementState& state,
    const CharacterMovementInput& input,
    ObjectId objectId,
    const CollisionWorld& collisionWorld,
    float terrainHeightM,   // 현재 위치의 지형 높이
    std::span<const Box> groundBoxes,   // 바닥 높이 계산에 사용할 박스 목록
    float moveSpeedMps,
    float deltaTimeSec);

// 점프중인 캐릭터의 위치 갱신 + 착지 처리
// gravityMps2는 아래 방향 가속도
void StepCharacterVerticalMovement(CharacterMovementState& state,
                                   float groundHeightM,
                                   float gravityMps2,
                                   float deltaTimeSec) noexcept;
} // namespace Kimgane::Shared::Physics
