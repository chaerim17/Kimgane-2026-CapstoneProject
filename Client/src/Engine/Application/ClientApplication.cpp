#include "Pch.h"

#include "ClientApplication.h"

#include "../Camera/CameraSettings.h"
#include "../Camera/SpringArmCamera.h"
#include "../Core/WindowSettings.h"
#include "../Diagnostics/ComponentSmokeTests.h"
#include "../Network/NetworkSettings.h"
#include "../Rendering/Mesh.h"
#include "../Scene/TestSceneSettings.h"
#include "../Terrain/TerrainHeightMap.h"
#include "../Terrain/TerrainMesh.h"
#include "../Terrain/TerrainSettings.h"

#include <stdexcept>

namespace Kimgane::Engine
{
ClientApplication::~ClientApplication()
{
    mNetwork.ShutdownNetwork();
}

int ClientApplication::Run(HINSTANCE instance, int commandShow)
{
    InitializeWindow(instance, commandShow);
    InitializeClient();
    return RunMessageLoop();
}

void ClientApplication::InitializeWindow(HINSTANCE instance, int commandShow)
{
    WNDCLASSEXW windowClass = {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = &ClientApplication::WindowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.lpszClassName = WindowSettings::WINDOW_CLASS_NAME;

    if (RegisterClassExW(&windowClass) == 0U)
    {
        throw std::runtime_error("Failed to register the window class.");
    }

    RECT windowRect = {0,
                       0,
                       static_cast<LONG>(WindowSettings::DEFAULT_CLIENT_WIDTH_PX),
                       static_cast<LONG>(WindowSettings::DEFAULT_CLIENT_HEIGHT_PX)};
    if (AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE) == FALSE)
    {
        throw std::runtime_error("Failed to adjust the window rectangle.");
    }

    mWindowHandle = CreateWindowExW(0,
                                    WindowSettings::WINDOW_CLASS_NAME,
                                    WindowSettings::WINDOW_TITLE,
                                    WS_OVERLAPPEDWINDOW,
                                    CW_USEDEFAULT,
                                    CW_USEDEFAULT,
                                    windowRect.right - windowRect.left,
                                    windowRect.bottom - windowRect.top,
                                    nullptr,
                                    nullptr,
                                    instance,
                                    this);

    if (mWindowHandle == nullptr)
    {
        throw std::runtime_error("Failed to create the window.");
    }

    ShowWindow(mWindowHandle, commandShow);
}

void ClientApplication::InitializeClient()
{
    mRenderer.Initialize(mWindowHandle,
                         WindowSettings::DEFAULT_CLIENT_WIDTH_PX,
                         WindowSettings::DEFAULT_CLIENT_HEIGHT_PX);
    Diagnostics::RunClientComponentSmokeTests(mRenderer.GetDevice());

    CreateTestAssets();
    InitializeCamera();
    mScene.Build(mCubeMesh, mTerrainMesh, mTerrainHeightMap, mInputManager, *mCamera);
    InitializeNetwork();
    SyncCameraToScene();
    mGameClock.Reset();
}

void ClientApplication::InitializeNetwork()
{
    // 클라이언트 실행 시 네트워크 계층을 한 번 초기화합니다.
    // 실제 서버 구현이 들어와도 Main.cpp는 건드리지 않고 이 흐름을 유지합니다.
    mNetwork.InitializeNetwork();
}

void ClientApplication::CreateTestAssets()
{
    mCubeMesh = Mesh::CreateCube(mRenderer.GetDevice(), TestSceneSettings::CUBE_SIZE_M);
    mTerrainHeightMap = TerrainHeightMap::CreateWaveField(TerrainSettings::DEFAULT_SAMPLE_WIDTH,
                                                          TerrainSettings::DEFAULT_SAMPLE_LENGTH,
                                                          TerrainSettings::DEFAULT_CELL_SPACING_M,
                                                          TerrainSettings::DEFAULT_WAVE_AMPLITUDE_M,
                                                          TerrainSettings::DEFAULT_WAVE_FREQUENCY);
    mTerrainMesh = TerrainMeshBuilder::CreateMesh(mRenderer.GetDevice(), *mTerrainHeightMap);
}

void ClientApplication::InitializeCamera()
{
    mCamera = std::make_unique<SpringArmCamera>();
    mCamera->SetLens(CameraSettings::DEFAULT_FOV_Y_RAD,
                     static_cast<float>(WindowSettings::DEFAULT_CLIENT_WIDTH_PX) /
                         static_cast<float>(WindowSettings::DEFAULT_CLIENT_HEIGHT_PX),
                     CameraSettings::DEFAULT_NEAR_CLIP_M,
                     CameraSettings::DEFAULT_FAR_CLIP_M);
}

void ClientApplication::SyncCameraToScene()
{
    mCamera->UpdateEye(mScene.GetCameraTargetPositionM());
    mRenderer.SetCameraPositionM(mCamera->GetEyeM());
    mRenderer.SetViewProjection(mCamera->GetViewProjectionMatrix4x4());
}

int ClientApplication::RunMessageLoop()
{
    MSG message = {};
    while (message.message != WM_QUIT)
    {
        if (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE) != FALSE)
        {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        else
        {
            UpdateAndRender();
        }
    }

    mRenderer.WaitForGpu();
    return static_cast<int>(message.wParam);
}

void ClientApplication::UpdateAndRender()
{
    const float deltaTimeSec = mGameClock.Tick();
    ProcessInput();
    UpdateScene(deltaTimeSec);
    UpdateNetwork(deltaTimeSec);
    UpdateCamera(deltaTimeSec);
    Render();
}

void ClientApplication::ProcessInput()
{
    mInputManager.Update();
}

void ClientApplication::UpdateNetwork(float deltaTimeSec)
{
    // 1. 현재 클라이언트의 로컬 플레이어 위치를 서버로 보낼 함수에 전달합니다.
    mNetwork.SendLocalPlayerPosition(NetworkSettings::LOCAL_PLAYER_ID, mScene.GetLocalPlayerPositionM());

    // 2. 서버에서 받은 위치 데이터를 갱신합니다. 현재는 NetworkFacade 내부 mock 데이터를 사용합니다.
    mNetwork.UpdateNetwork(deltaTimeSec);

    // 3. 수신된 플레이어 위치를 씬 오브젝트 위치에 반영해 렌더링되도록 합니다.
    ApplyNetworkPlayerLocations();
}

void ClientApplication::ApplyNetworkPlayerLocations()
{
    int playerId = 0;
    float xM = 0.0F;
    float yM = 0.0F;
    float zM = 0.0F;

    while (mNetwork.get_player_location(&playerId, &xM, &yM, &zM))
    {
        // 서버가 넘겨준 플레이어 id와 좌표를 기준으로 네트워크 플레이어 오브젝트를 생성/갱신합니다.
        mScene.UpdateNetworkPlayerPosition(playerId, {xM, yM, zM});
    }
}

void ClientApplication::UpdateScene(float deltaTimeSec)
{
    mScene.Update(deltaTimeSec);
}

void ClientApplication::UpdateCamera(float deltaTimeSec)
{
    mCamera->Update(deltaTimeSec);
    SyncCameraToScene();
}

void ClientApplication::Render()
{
    mRenderer.Render(mScene);
}

LRESULT CALLBACK ClientApplication::WindowProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_NCCREATE)
    {
        const auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(windowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
    }

    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(windowHandle, message, wParam, lParam);
    }
}
} // namespace Kimgane::Engine
