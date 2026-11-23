#pragma once

#include "../Core/GameObject.h"
#include "../Rendering/Light.h"
#include "../Rendering/Mesh.h"
#include "../UI/DamageNumber.h"
#include "../UI/TextLabel.h"
#include "../UI/WorldHealthBar.h"
#include "../../Shared/Physics/CollisionWorld.h"

#include <DirectXMath.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <d3d12.h>

#include <memory>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace Kimgane::Client
{
class LunarTerrainRenderer;
// GameObject를 소유하고 생성/정리, 갱신, 렌더링을 관리하는 기본 씬.
// 충돌 판정은 Shared CollisionWorld가 담당하며, 충돌 월드가 있는 씬(GameScene)만 GetCollisionWorld()를 재정의함.
class Scene
{
public:
    Scene() = default;
    virtual ~Scene() = default;

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) = delete;
    Scene& operator=(Scene&&) = delete;

    // T(기본 GameObject)를 만들어 씬이 소유함. 반환 참조는 Clear() 전까지 유효.
    template <typename T = GameObject, typename... Args>
    T& CreateObject(Args&&... args)
    {
        static_assert(std::is_base_of_v<GameObject, T>, "T는 GameObject를 상속해야 함");
        auto object = std::make_unique<T>(std::forward<Args>(args)...);
        T& reference = *object;
        mObjects.push_back(std::move(object));
        return reference;
    }
    void Clear() noexcept;
    void SetTerrain(std::shared_ptr<LunarTerrainRenderer> terrain) noexcept { mTerrainRenderer = std::move(terrain); }
    [[nodiscard]] const std::shared_ptr<LunarTerrainRenderer>& GetTerrain() const noexcept { return mTerrainRenderer; }

    virtual void Update(float deltaTimeSec);
    virtual void Render(ID3D12GraphicsCommandList& commandList,
                        D3D12_GPU_DESCRIPTOR_HANDLE defaultTextureGpuHandle,
                        MeshPrimitiveTopology primitiveTopology = MeshPrimitiveTopology::TriangleList) const;

    [[nodiscard]] const std::vector<std::unique_ptr<GameObject>>& GetObjects() const noexcept;

    // 패널 위에 그릴 글자를 만들어 씬이 소유함.
    TextLabel& CreateTextLabel(const GameObject& panel, std::wstring text, float fontSizeDip);
    [[nodiscard]] const std::vector<std::unique_ptr<TextLabel>>& GetTextLabels() const noexcept;
    // 렌더러가 그릴 머리 위 체력바 목록. 체력바가 있는 씬(GameScene)만 채움.
    virtual void CollectWorldHealthBars(std::vector<WorldHealthBarView>& outHealthBars) const;
    // 렌더러가 그릴 데미지 숫자 풀. 비활성 항목은 렌더러가 건너뜀.
    [[nodiscard]] virtual std::span<const DamageNumber> GetDamageNumbers() const noexcept;
    // 레이캐스트(체력바 가림 판정 등)에 쓰는 Shared 충돌 월드. 충돌이 없는 UI 씬은 nullptr.
    [[nodiscard]] virtual const Kimgane::Shared::Physics::CollisionWorld* GetCollisionWorld() const noexcept
    {
        return nullptr;
    }
    // 씬에 하나뿐인 태양광. UI 씬은 기본값을 쓰고, GameScene은 Build에서 설정함.
    [[nodiscard]] const DirectionalLight& GetDirectionalLight() const noexcept;
    [[nodiscard]] DirectionalLight& GetDirectionalLight() noexcept;
    // 렌더 전용 행렬입니다. 실제 Transform/충돌 위치를 변경하지 않음.
    [[nodiscard]] virtual DirectX::XMFLOAT4X4 GetRenderWorldMatrix(const GameObject& object) const noexcept;

private:
    std::shared_ptr<LunarTerrainRenderer> mTerrainRenderer;
    DirectionalLight mDirectionalLight;
    std::vector<std::unique_ptr<GameObject>> mObjects;
    std::vector<std::unique_ptr<TextLabel>> mTextLabels;
};
} // namespace Kimgane::Client
