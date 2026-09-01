#pragma once

#include "../Core/GameClock.h"
#include "../Input/InputManager.h"
#include "../Network/NetworkManager.h"
#include "../Rendering/Dx12Renderer.h"
#include "../Scene/Scene.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include <memory>

namespace Kimgane::Engine
{
class Mesh;
class TerrainHeightMap;

class ClientApplication final
{
public:
    ClientApplication() = default;
    ~ClientApplication();

    ClientApplication(const ClientApplication&) = delete;
    ClientApplication& operator=(const ClientApplication&) = delete;
    ClientApplication(ClientApplication&&) = delete;
    ClientApplication& operator=(ClientApplication&&) = delete;

    [[nodiscard]] int Run(HINSTANCE instance, int commandShow);

private:
    enum class ActiveSceneType
    {
        Title,
        LocalGame,
        OnlineGame
    };

    void InitializeWindow(HINSTANCE instance, int commandShow);
    void InitializeClient();
    //void InitializeNetwork();
    void CreateTestAssets();
    void BuildTitleAndOverlayScenes();
    void BuildGameScene(GameScene& scene);
    void EnterLocalGameScene();
    void EnterOnlineGameScene();
    void OpenSettingsOverlay();
    void CloseSettingsOverlay();
    [[nodiscard]] float GetCameraAspectRatio() const noexcept;
    void SyncCameraToScene();
    [[nodiscard]] int RunMessageLoop();
    void UpdateAndRender();
    void ProcessInput();
    void ApplyNetworkPlayerLocations();
    void UpdateScene(float deltaTimeSec);
    void HandleTitleSceneAction();
    void HandleSettingsOverlayAction();
    void UpdateCamera(float deltaTimeSec);
    void Render();
    void UpdateWindowTitle(float deltaTimeSec);
    void RefreshWindowTitle() const;
    [[nodiscard]] const Scene* GetActiveScene() const noexcept;
    [[nodiscard]] GameScene* GetActiveGameScene() noexcept;
    [[nodiscard]] const GameScene* GetActiveGameScene() const noexcept;
    [[nodiscard]] const Camera* GetActiveSceneCamera() const noexcept;

    static LRESULT CALLBACK WindowProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);

    HWND mWindowHandle = nullptr;
    GameClock mGameClock;
    InputManager mInputManager;
    NetworkManager mNetwork;
    std::shared_ptr<Mesh> mCubeMesh;
    std::shared_ptr<Mesh> mPlayerModelMesh;     // 26.07.10 모델 메쉬 멤버 변수 추가
    std::shared_ptr<Mesh> mNpcModelMesh;    // NPC 모델 메쉬 멤버 변수 추가
    std::shared_ptr<Mesh> mHouseModelMesh;
    std::shared_ptr<Mesh> mTerrainMesh;
    std::shared_ptr<Mesh> mUiMesh;
    std::shared_ptr<TerrainHeightMap> mTerrainHeightMap;
    TitleScene mTitleScene;
    SettingsOverlayScene mSettingsOverlayScene;
    std::unique_ptr<GameScene> mActiveGameScene;
    Dx12Renderer mRenderer;
    ActiveSceneType mActiveSceneType = ActiveSceneType::Title;
    bool mSettingsOverlayVisible = false;
    bool mShowFpsInWindowTitle = true;
    float mFpsElapsedTimeSec = 0.0F;
    float mLastFps = 0.0F;
    unsigned int mFpsFrameCount = 0U;
};
} // namespace Kimgane::Engine
