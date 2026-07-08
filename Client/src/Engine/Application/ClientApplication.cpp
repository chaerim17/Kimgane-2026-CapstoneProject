#include "Pch.h"

#include "ClientApplication.h"

#include "../Camera/CameraSettings.h"
#include "../Camera/SpringArmCamera.h"
#include "../Core/WindowSettings.h"
#include "../Diagnostics/ComponentSmokeTests.h"
#include "../Rendering/Mesh.h"
#include "../Scene/TestSceneSettings.h"
#include "../Terrain/TerrainHeightMap.h"
#include "../Terrain/TerrainMesh.h"
#include "../Terrain/TerrainSettings.h"

#include <stdexcept>

namespace Kimgane::Engine
{
ClientApplication::~ClientApplication() = default;

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
    SyncCameraToScene();
    mGameClock.Reset();
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
    UpdateCamera(deltaTimeSec);
    Render();
}

void ClientApplication::ProcessInput()
{
    mInputManager.Update();
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
