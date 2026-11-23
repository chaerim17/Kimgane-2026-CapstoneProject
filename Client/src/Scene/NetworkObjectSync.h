#pragma once

#include "../Gameplay/ItemBox.h"
#include "../Gameplay/Npc.h"
#include "../Gameplay/Player.h"
#include "../../Shared/Geometry/CollisionBoxLoader.h"
#include "../../Shared/Physics/CollisionWorld.h"

#include <DirectXMath.h>
#include <d3d12.h>

#include <memory>
#include <unordered_map>
#include <vector>

namespace Kimgane::Client
{
class Mesh;
class NetworkManager;
class Scene;

// 서버 오브젝트를 그릴 때 쓰는 모델·텍스처·충돌 박스.
struct NetworkObjectAssets
{
    PlayerAppearance playerAppearance;
    std::shared_ptr<Mesh> npcMesh;
    D3D12_GPU_DESCRIPTOR_HANDLE npcTextureGpuHandle = {};
    std::shared_ptr<Mesh> itemBoxBodyMesh;
    std::shared_ptr<Mesh> itemBoxLidMesh;
    D3D12_GPU_DESCRIPTOR_HANDLE itemBoxTextureGpuHandle = {};
    // 상자 본체 기준 로컬 충돌 박스(S07_Supply_Crate_collision.txt). 서버와 같은 박스로 충돌 월드에 등록함.
    std::vector<Kimgane::Shared::Geometry::NamedCollisionBox> itemBoxCollisionBoxes;
};

// 서버 GameWorld에 대응. NetworkManager가 받은 서버 오브젝트 목록을 클라 객체(RemotePlayer / Npc / ItemBox)로 맞춤.
// 내 캐릭터(LocalPlayer)는 다루지 않음 — 예측·보정이 필요해서 GameScene이 직접 처리함.
class NetworkObjectSync final
{
public:
    // 씬을 새로 만들 때 호출. 이전 목록을 비우고 객체를 만들 씬과 충돌 월드를 연결함.
    void Reset(Scene& scene,
               NetworkManager& networkManager,
               Kimgane::Shared::Physics::CollisionWorld& collisionWorld,
               NetworkObjectAssets assets);

    // 서버가 보낸 위치·방향 반영. 처음 보는 id면 서버 오브젝트 종류에 따라 NPC 또는 원격 플레이어를 만듦.
    void ApplyServerTransform(int objectId, const DirectX::XMFLOAT3& positionM, float yawRad);
    // 서버에서 사라진 플레이어·NPC를 숨기고 목록에서 뺌.
    void Remove(int objectId);
    // 매 프레임: NPC 체력바 HP, 원격 플레이어 변이 외형, 아이템 상자 생성·제거를 서버 상태에 맞춤.
    void Update();

    // 서버가 보낸 ObjectType으로 NPC인지 판단함(아직 클라 객체가 없어도 판단 가능).
    [[nodiscard]] bool IsNpc(int objectId) const noexcept;
    [[nodiscard]] const Npc* FindNpc(int objectId) const noexcept;
    [[nodiscard]] const std::unordered_map<int, Npc*>& GetNpcs() const noexcept;
    [[nodiscard]] const std::unordered_map<int, ItemBox*>& GetItemBoxes() const noexcept;

private:
    void CreateNetworkObject(int objectId, const DirectX::XMFLOAT3& positionM, float yawRad);
    void CreateItemBox(int itemBoxId, const DirectX::XMFLOAT3& positionM);
    void RefreshHealthBars();
    void RefreshRemoteMutationAppearances();
    // 서버 목록에 새로 생긴 상자는 만들고, 사라진 상자는 숨기고 충돌 body도 지움.
    void SyncItemBoxes();

    Scene* mScene = nullptr;
    NetworkManager* mNetworkManager = nullptr;
    Kimgane::Shared::Physics::CollisionWorld* mCollisionWorld = nullptr;
    NetworkObjectAssets mAssets;
    // 서버 오브젝트 id → 클라 객체. 서버 GameWorld처럼 플레이어와 NPC를 나눠 둠.
    std::unordered_map<int, RemotePlayer*> mRemotePlayers;
    std::unordered_map<int, Npc*> mNpcs;
    std::unordered_map<int, ItemBox*> mItemBoxes;
};
} // namespace Kimgane::Client
