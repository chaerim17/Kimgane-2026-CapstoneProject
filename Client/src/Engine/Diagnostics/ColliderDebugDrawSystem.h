#pragma once

#include "../../Shared/Physics/CollisionWorld.h"
#include "../../Shared/Terrain/TerrainHeightMap.h"

#include <DirectXMath.h>

#include <memory>
#include <unordered_map>

struct ID3D12Device;

namespace Kimgane::Engine
{
class GameObject;
class Scene;

// Shared CollisionWorld에 등록된 충돌 형상을 선으로 그리는 디버그 표시.
// 보이는 동안에만 메쉬를 만들고, 월드에서 빠진 body의 표시는 끔.
class ColliderDebugDrawSystem final
{
public:
    void Initialize(ID3D12Device& device,
                    std::shared_ptr<const TerrainHeightMap> terrainHeightMap,
                    const DirectX::XMFLOAT3& terrainPositionM);
    void Clear() noexcept;
    void SetVisible(bool visible) noexcept;
    void ToggleVisible() noexcept;
    [[nodiscard]] bool IsVisible() const noexcept;

    // 월드 body 목록과 디버그 오브젝트를 맞추고, 로컬 플레이어 캡슐을 발 위치로 옮김.
    // 로컬 플레이어 캡슐은 월드에 등록되지 않고 이동 계산 때만 만들어지므로 따로 그림.
    void Sync(Scene& scene,
              const Kimgane::Shared::Physics::CollisionWorld& world,
              const DirectX::XMFLOAT3& localPlayerFootM);

private:
    void HideAll() noexcept;

    ID3D12Device* mDevice = nullptr;
    std::shared_ptr<const TerrainHeightMap> mTerrainHeightMap;
    DirectX::XMFLOAT3 mTerrainPositionM = {};
    std::unordered_map<Kimgane::Shared::Physics::ObjectId, GameObject*> mBodyObjects;
    GameObject* mLocalPlayerCapsuleObject = nullptr;
    bool mVisible = false;
};
} // namespace Kimgane::Engine
