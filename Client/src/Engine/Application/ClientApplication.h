#pragma once

#include "../Camera/SpringArmCamera.h"
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
    void InitializeWindow(HINSTANCE instance, int commandShow);
    void InitializeClient();
    //void InitializeNetwork();
    void CreateTestAssets();
    void InitializeCamera();
    void SyncCameraToScene();
    [[nodiscard]] int RunMessageLoop();
    void UpdateAndRender();
    void ProcessInput();
    //void UpdateNetwork(float deltaTimeSec);
    void ApplyNetworkPlayerLocations();
    void UpdateScene(float deltaTimeSec);
    void UpdateCamera(float deltaTimeSec);
    void Render();

    static LRESULT CALLBACK WindowProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam);

    HWND mWindowHandle = nullptr;
    GameClock mGameClock;
    InputManager mInputManager;
    NetworkManager mNetwork;
    std::shared_ptr<Mesh> mCubeMesh;
    std::shared_ptr<Mesh> mPlayerModelMesh;     // 26.07.10 모델 메쉬 멤버 변수 추가
    std::shared_ptr<Mesh> mTerrainMesh;
    std::shared_ptr<TerrainHeightMap> mTerrainHeightMap;
    std::unique_ptr<SpringArmCamera> mCamera;
    TestScene mScene;
    Dx12Renderer mRenderer;
};
} // namespace Kimgane::Engine
