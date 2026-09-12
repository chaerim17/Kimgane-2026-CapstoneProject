#pragma once

#include <vector>

namespace Kimgane::Shared::Physics
{
struct CharacterMotionInput;
}

namespace Kimgane::Engine
{
class GameObject;
class ColliderComponent;
class CollisionManager;

// 클라이언트 컴포넌트와 Shared 캐릭터 이동 계산을 연결합니다.
// 후보 위치의 충돌은 CollisionManager로 조회하며 입력 적용/적분/보정 정책은 Shared에 있습니다.
// 키보드/카메라 입력 해석, 서버 위치 동기화, HP 감소와 삭제는 담당하지 않습니다.
// 씬을 참조하거나 객체를 소유하지 않으며, 호출 사이에 상태나 참조를 보관하지 않습니다.
class CharacterCollisionSolver final
{
public:
    // 자동 적분을 끈 Rigidbody와 캡슐이 있는 캐릭터를 한 번 갱신합니다.
    // 입력은 현재 프레임의 월드 방향과 점프 요청이며, 호출자가 프레임당 한 번 전달합니다.
    static void Step(GameObject& character,
                     const CollisionManager& collisionManager,
                     const std::vector<ColliderComponent*>& collisionTargets,
                     const Kimgane::Shared::Physics::CharacterMotionInput& input,
                     float deltaTimeSec);

    // 이미 적분한 캐릭터에는 충돌 보정만 적용하는 진입점을 사용합니다.
    // 컴포넌트 상태 읽기 → 접촉 재조회와 반복 보정 → 결과를 컴포넌트에 반영하는 순서입니다.
    // 캡슐이 없으면 건너뛰며, Rigidbody가 없으면 Transform 위치만 보정합니다.
    // collisionTargets는 호출 중 유효한 비소유 포인터 목록이며, 자신/null/비활성 객체는 제외합니다.
    static void Solve(GameObject& character,
                      const CollisionManager& collisionManager,
                      const std::vector<ColliderComponent*>& collisionTargets);
};
} // namespace Kimgane::Engine
