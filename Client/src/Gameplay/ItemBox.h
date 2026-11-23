#pragma once

#include "../Core/GameObject.h"

#include <string>

namespace Kimgane::Client
{
// 서버 ItemBox에 대응하는 아이템 상자. 이 객체가 본체이고, 뚜껑은 따로 그려지는 GameObject를 붙여서 같이 다룸.
// 충돌 body는 GameScene이 같은 id로 Shared 충돌 월드에 등록함(서버 GameWorld::CreateItemBox와 같은 박스).
class ItemBox final : public GameObject
{
public:
    using GameObject::GameObject;

    // 뚜껑을 닫힌 위치(본체 기준 로컬 오프셋)에 붙임.
    void AttachLid(GameObject& lid, const DirectX::XMFLOAT3& lidLocalOffsetM) noexcept;
    // 서버 목록에서 사라졌을 때 본체와 뚜껑을 함께 숨김.
    void Despawn() noexcept;

private:
    GameObject* mLid = nullptr;
};
} // namespace Kimgane::Client
