#pragma once

#include "../Core/GameObject.h"
#include "../Physics/CollisionManager.h"
#include "../Rendering/Light.h"
#include "../Rendering/Mesh.h"
#include "../Terrain/TerrainHeightMap.h"

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
#include <string>
#include <unordered_map>
#include <vector>

namespace Kimgane::Engine
{
class Scene
{
public:
    Scene() = default;
    virtual ~Scene() = default;

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) = delete;
    Scene& operator=(Scene&&) = delete;

    GameObject& CreateObject(std::string name = {});
    void Clear() noexcept;

    virtual void Update(float deltaTimeSec);
    virtual void Render(ID3D12GraphicsCommandList& commandList) const;

    [[nodiscard]] const std::vector<std::unique_ptr<GameObject>>& GetObjects() const noexcept;
    [[nodiscard]] CollisionManager& GetCollisionManager() noexcept;
    [[nodiscard]] const CollisionManager& GetCollisionManager() const noexcept;
    [[nodiscard]] const DirectionalLight& GetDirectionalLight() const noexcept;

private:
    CollisionManager mCollisionManager;
    DirectionalLight mDirectionalLight;
    std::vector<std::unique_ptr<GameObject>> mObjects;
};

class Camera;
class InputManager;

class TestScene final : public Scene
{
public:
    void Build(std::shared_ptr<Mesh> cubeMesh,
               std::shared_ptr<Mesh> terrainMesh,
               std::shared_ptr<const TerrainHeightMap> terrainHeightMap,
               const InputManager& inputManager,
               const Camera& gameplayCamera);
    void Update(float deltaTimeSec) override;
    [[nodiscard]] DirectX::XMFLOAT3 GetCameraTargetPositionM() const noexcept;
    [[nodiscard]] DirectX::XMFLOAT3 GetLocalPlayerPositionM() const noexcept;
    void UpdateNetworkPlayerPosition(int playerId, const DirectX::XMFLOAT3& positionM);

private:
    GameObject& CreateNetworkPlayer(int playerId, const DirectX::XMFLOAT3& positionM);

    std::shared_ptr<Mesh> mPlayerMesh;
    GameObject* mTestCube = nullptr;
    GameObject* mTerrain = nullptr;
    GameObject* mLocalPlayer = nullptr;
    std::unordered_map<int, GameObject*> mNetworkPlayers;
    float mCubeRotationRad = DirectX::XMConvertToRadians(36.0F);
};
} // namespace Kimgane::Engine
