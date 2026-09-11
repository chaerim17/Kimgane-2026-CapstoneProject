#pragma once

#include <vector>

namespace Kimgane::Engine
{
class GameObject;
class ColliderComponent;
class CollisionManager;

// 이동 적분 후 캡슐 캐릭터의 접촉을 수집하고 위치/속도/접지 상태를 보정하는 클라이언트 전용 처리기입니다.
// 충돌 판정은 CollisionManager에, 접촉에 따른 보정 계산은 Shared의 CollisionResolver에 맡깁니다.
// 입력/중력/이동 적분, 서버 위치 동기화, HP 감소와 삭제는 담당하지 않습니다.
// 씬을 참조하거나 객체를 소유하지 않으며, 호출 사이에 상태나 참조를 보관하지 않습니다.
class CharacterCollisionSolver final
{
public:
    // 컴포넌트 상태 읽기 → 접촉 재조회와 반복 보정 → 결과를 컴포넌트에 반영하는 순서입니다.
    // 캡슐이 없으면 건너뛰며, Rigidbody가 없으면 Transform 위치만 보정합니다.
    // collisionTargets는 호출 중 유효한 비소유 포인터 목록이며, 자신/null/비활성 객체는 제외합니다.
    static void Solve(GameObject& character,
                      const CollisionManager& collisionManager,
                      const std::vector<ColliderComponent*>& collisionTargets);
};
} // namespace Kimgane::Engine
