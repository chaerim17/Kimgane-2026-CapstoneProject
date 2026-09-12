#pragma once

#include "../Camera/Camera.h"
#include "../Core/GameObject.h"
#include "../Diagnostics/ColliderDebugDrawSystem.h"
#include "../Physics/ColliderComponent.h"
#include "../Physics/CollisionManager.h"
#include "../Rendering/Light.h"
#include "../Rendering/Mesh.h"
#include "../../Shared/Terrain/TerrainHeightMap.h"
#include "../../Shared/Physics/FixedStepClock.h"

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
// GameObject를 소유하고 생성/정리, 컴포넌트 갱신, 렌더링을 관리하는 기본 씬입니다.
// CollisionManager는 충돌 조회를 제공하며, 캐릭터의 충돌 보정 호출은 GameScene이 담당합니다.
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
    // 렌더 전용 행렬입니다. 실제 Transform/충돌 위치를 변경하지 않습니다.
    [[nodiscard]] virtual DirectX::XMFLOAT4X4 GetRenderWorldMatrix(const GameObject& object) const noexcept;

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

// 플레이 환경과 캐릭터를 구성하고 입력 모드, 카메라, 네트워크 객체 갱신을 연결합니다.
// 충돌 대상을 구성하고 CharacterCollisionSolver를 통해 Shared의 입력/적분/접지 보정을 호출합니다.
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
    // 일반 컴포넌트용 제한 delta와 고정 물리용 실제 경과 시간을 분리합니다.
    void Update(float deltaTimeSec, double physicsElapsedTimeSec);
    [[nodiscard]] DirectX::XMFLOAT4X4 GetRenderWorldMatrix(const GameObject& object) const noexcept override;
    [[nodiscard]] const Camera* GetGameplayCamera() const noexcept;
    [[nodiscard]] DirectX::XMFLOAT3 GetCameraTargetPositionM() const noexcept;
    [[nodiscard]] DirectX::XMFLOAT3 GetLocalPlayerPositionM() const noexcept;
    [[nodiscard]] float GetLocalPlayerYaw() const noexcept;
    void UpdateNetworkPlayerPosition(int playerId, const DirectX::XMFLOAT3& positionM, float yaw);
    void RemoveNetworkPlayer(int playerId);
    // 집과의 접촉을 조회해 로그 판정에 사용합니다. 이동 보정이나 충돌 이벤트 전달은 하지 않습니다.
    [[nodiscard]] std::vector<ContactInfo> CheckLocalPlayerHouseCollision();

protected:
    [[nodiscard]] virtual bool UsesNetworkInput() const noexcept = 0;

private:
    // 충돌 조회 목록, 로컬 플레이어 보정 대상 목록, 디버그 표시 대상을 함께 등록합니다.
    void RegisterLocalPlayerCollisionTarget(ColliderComponent& collider);
    void RegisterColliderDebugTarget(ColliderComponent& collider);
    GameObject& CreateNetworkPlayer(int playerId, const DirectX::XMFLOAT3& positionM);
    void CorrectLocalPlayerState(const DirectX::XMFLOAT3& authoritativePositionM, float authoritativeYaw) noexcept;
    [[nodiscard]] DirectX::XMFLOAT3 GetLocalPlayerRenderPositionM() const noexcept;
    void DecayLocalPlayerRenderCorrection(double elapsedTimeSec) noexcept;

    NetworkManager* mNetworkManager = nullptr;
    const InputManager* mInputManager = nullptr;
    ID3D12Device* mDebugDevice = nullptr;

    std::shared_ptr<Mesh> mPlayerMesh;
    std::shared_ptr<Mesh> mNpcMesh;
    std::vector<BoxColliderComponent*> mHouseColliders; // TestHouse의 박스 콜라이더들을 저장하는 벡터
    // Solver에 전달하는 비소유 목록입니다. 등록된 환경 콜라이더는 호출 동안 유효해야 합니다.
    std::vector<ColliderComponent*> mLocalPlayerCollisionTargets;
    bool mIsLocalPlayerCollidingWithHouse = false;      // 충돌처리 체크
    GameObject* mTestCube = nullptr;
    GameObject* mTerrain = nullptr;
    GameObject* mLocalPlayer = nullptr;
    Kimgane::Shared::Physics::FixedStepClock mPhysicsClock;
    DirectX::XMFLOAT3 mPreviousLocalPlayerPositionM = {};
    // 서버 보정 전 표시 위치와의 차이입니다. 물리/충돌/송신 상태에는 적용하지 않습니다.
    DirectX::XMFLOAT3 mLocalPlayerRenderCorrectionM = {};
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
