#pragma once

#include "../Camera/Camera.h"
#include "../Core/GameObject.h"
#include "../Diagnostics/ColliderDebugDrawSystem.h"
#include "../Physics/ColliderComponent.h"
#include "../Physics/CollisionManager.h"
#include "../Rendering/Light.h"
#include "../Rendering/Mesh.h"
#include "../../Shared/Terrain/TerrainHeightMap.h"

#include <DirectXMath.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <d3d12.h>

#include <array>
#include <cstddef>
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
    virtual void Render(ID3D12GraphicsCommandList& commandList,
                        MeshPrimitiveTopology primitiveTopology = MeshPrimitiveTopology::TriangleList) const;

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
class CameraComponent;
class NetworkManager;
class InputManager;
class TextComponent;

enum class TitleSceneAction
{
    None,
    StartLocalGame,
    StartOnlineGame,
    OpenSettings
};

enum class TitleMenuOption : std::size_t
{
    LocalGame,
    OnlineGame,
    Settings,
    Count
};

class TitleScene final : public Scene
{
public:
    void Build(std::shared_ptr<Mesh> uiMesh, const InputManager& inputManager, float cameraAspectRatio);
    void Update(float deltaTimeSec) override;

    [[nodiscard]] const Camera* GetUiCamera() const noexcept;
    [[nodiscard]] TitleSceneAction ConsumePendingAction() noexcept;
    [[nodiscard]] TitleMenuOption GetSelectedOption() const noexcept;
    [[nodiscard]] const wchar_t* GetSelectedOptionLabelW() const noexcept;

private:
    void MoveSelection(int direction) noexcept;
    void RefreshVisualState() noexcept;

    const InputManager* mInputManager = nullptr;
    OrthographicCamera mUiCamera;
    std::array<GameObject*, static_cast<std::size_t>(TitleMenuOption::Count)> mOptionPanels = {};
    std::array<TextComponent*, static_cast<std::size_t>(TitleMenuOption::Count)> mOptionLabels = {};
    TitleMenuOption mSelectedOption = TitleMenuOption::LocalGame;
    TitleSceneAction mPendingAction = TitleSceneAction::None;
};

class OverlayScene : public Scene
{
public:
    [[nodiscard]] const Camera* GetUiCamera() const noexcept;

protected:
    void ConfigureOverlayCamera(float cameraAspectRatio) noexcept;

private:
    OrthographicCamera mUiCamera;
};

class SettingsOverlayScene final : public OverlayScene
{
public:
    void Build(std::shared_ptr<Mesh> uiMesh,
               const InputManager& inputManager,
               bool fpsInWindowTitleEnabled,
               float cameraAspectRatio);
    void Update(float deltaTimeSec) override;

    void SetFpsInWindowTitleEnabled(bool enabled) noexcept;
    [[nodiscard]] bool IsFpsInWindowTitleEnabled() const noexcept;
    [[nodiscard]] bool ConsumeCloseRequested() noexcept;

private:
    void RefreshVisualState() noexcept;

    const InputManager* mInputManager = nullptr;
    GameObject* mFpsTogglePanel = nullptr;
    TextComponent* mFpsToggleLabel = nullptr;
    bool mFpsInWindowTitleEnabled = true;
    bool mCloseRequested = false;
};

class GameScene : public Scene
{
public:
    void Build(std::shared_ptr<Mesh> cubeMesh,
               ID3D12Device& device,
               std::shared_ptr<Mesh> playerModelMesh,       // 26.07.10 모델 메쉬 매개변수 추가
               std::shared_ptr<Mesh> npcModelMesh,          // NPC 모델 메쉬 매개변수 추가
               std::shared_ptr<Mesh> houseModelMesh,        // 집 모델 메쉬 매개변수 추가
               std::shared_ptr<Mesh> terrainMesh,
               std::shared_ptr<const TerrainHeightMap> terrainHeightMap,
               const InputManager& inputManager,
               NetworkManager& networkManager,
               float cameraAspectRatio);
    void Update(float deltaTimeSec) override;
    void RefreshGameplayCamera() noexcept;
    [[nodiscard]] const Camera* GetGameplayCamera() const noexcept;
    [[nodiscard]] DirectX::XMFLOAT3 GetCameraTargetPositionM() const noexcept;
    [[nodiscard]] DirectX::XMFLOAT3 GetLocalPlayerPositionM() const noexcept;
    [[nodiscard]] float GetLocalPlayerYaw() const noexcept;
    void UpdateNetworkPlayerPosition(int playerId, const DirectX::XMFLOAT3& positionM, float yaw);
    void RemoveNetworkPlayer(int playerId);
    [[nodiscard]] std::vector<ContactInfo> CheckLocalPlayerHouseCollision(); // 충돌처리 체크

protected:
    [[nodiscard]] virtual bool UsesNetworkInput() const noexcept = 0;

private:
    void RegisterLocalPlayerCollisionTarget(ColliderComponent& collider);
    void RegisterColliderDebugTarget(ColliderComponent& collider);
    [[nodiscard]] std::vector<ContactInfo> QueryLocalPlayerContacts(CapsuleColliderComponent& playerCollider);
    void ResolveLocalPlayerCollisions();
    GameObject& CreateNetworkPlayer(int playerId, const DirectX::XMFLOAT3& positionM);
    void CorrectLocalPlayerState(const DirectX::XMFLOAT3& authoritativePositionM, float authoritativeYaw) noexcept;

    NetworkManager* mNetworkManager = nullptr;
    const InputManager* mInputManager = nullptr;
    ID3D12Device* mDebugDevice = nullptr;

    std::shared_ptr<Mesh> mPlayerMesh;
    std::shared_ptr<Mesh> mNpcMesh;
    std::vector<BoxColliderComponent*> mHouseColliders; // TestHouse의 박스 콜라이더들을 저장하는 벡터
    std::vector<ColliderComponent*> mLocalPlayerCollisionTargets;
    bool mIsLocalPlayerCollidingWithHouse = false;      // 충돌처리 체크
    GameObject* mTestCube = nullptr;
    GameObject* mTerrain = nullptr;
    GameObject* mLocalPlayer = nullptr;
    CameraComponent* mGameplayCamera = nullptr;
    ColliderDebugDrawSystem mColliderDebugDraw;
    std::unordered_map<int, GameObject*> mNetworkPlayers;
    float mCubeRotationRad = DirectX::XMConvertToRadians(36.0F);
};

class LocalGameScene final : public GameScene
{
private:
    [[nodiscard]] bool UsesNetworkInput() const noexcept override;
};

class OnlineGameScene final : public GameScene
{
private:
    [[nodiscard]] bool UsesNetworkInput() const noexcept override;
};
} // namespace Kimgane::Engine
