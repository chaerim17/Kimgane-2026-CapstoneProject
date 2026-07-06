#include "Pch.h"

#include "Engine/Camera/CameraSettings.h"
#include "Engine/Camera/SpringArmCamera.h"
#include "Engine/Core/GameClock.h"
#include "Engine/Core/WindowSettings.h"
#include "Engine/Diagnostics/ComponentSmokeTests.h"
#include "Engine/Rendering/Dx12Renderer.h"
#include "Engine/Rendering/Mesh.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/TestSceneSettings.h"
#include "Engine/Terrain/TerrainHeightMap.h"
#include "Engine/Terrain/TerrainMesh.h"
#include "Engine/Terrain/TerrainSettings.h"

#include <Windows.h>

#include <memory>
#include <stdexcept>

namespace
{
class Dx12ClientApp
{
public:
    int Run(HINSTANCE instance, int commandShow)
    {
        InitializeWindow(instance, commandShow);
        InitializeClient();
        return RunMessageLoop();
    }

private:
    void InitializeWindow(HINSTANCE instance, int commandShow)
    {
        WNDCLASSEXW windowClass = {};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.lpfnWndProc = &Dx12ClientApp::WindowProc;
        windowClass.hInstance = instance;
        windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        windowClass.lpszClassName = Kimgane::Engine::WindowSettings::WINDOW_CLASS_NAME;

        if (RegisterClassExW(&windowClass) == 0U)
        {
            throw std::runtime_error("Failed to register the window class.");
        }

        RECT windowRect = {0,
                           0,
                           static_cast<LONG>(Kimgane::Engine::WindowSettings::DEFAULT_CLIENT_WIDTH_PX),
                           static_cast<LONG>(Kimgane::Engine::WindowSettings::DEFAULT_CLIENT_HEIGHT_PX)};
        if (AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE) == FALSE)
        {
            throw std::runtime_error("Failed to adjust the window rectangle.");
        }

        mWindowHandle = CreateWindowExW(0,
                                        Kimgane::Engine::WindowSettings::WINDOW_CLASS_NAME,
                                        Kimgane::Engine::WindowSettings::WINDOW_TITLE,
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

    void InitializeClient()
    {
        mRenderer.Initialize(mWindowHandle,
                             Kimgane::Engine::WindowSettings::DEFAULT_CLIENT_WIDTH_PX,
                             Kimgane::Engine::WindowSettings::DEFAULT_CLIENT_HEIGHT_PX);
        Kimgane::Engine::Diagnostics::RunClientComponentSmokeTests(mRenderer.GetDevice());

        mCubeMesh = Kimgane::Engine::Mesh::CreateCube(mRenderer.GetDevice(),
                                                      Kimgane::Engine::TestSceneSettings::CUBE_SIZE_M);
        mTerrainHeightMap =
            Kimgane::Engine::TerrainHeightMap::CreateWaveField(Kimgane::Engine::TerrainSettings::DEFAULT_SAMPLE_WIDTH,
                                                               Kimgane::Engine::TerrainSettings::DEFAULT_SAMPLE_LENGTH,
                                                               Kimgane::Engine::TerrainSettings::DEFAULT_CELL_SPACING_M,
                                                               Kimgane::Engine::TerrainSettings::DEFAULT_WAVE_AMPLITUDE_M,
                                                               Kimgane::Engine::TerrainSettings::DEFAULT_WAVE_FREQUENCY);
        mTerrainMesh = Kimgane::Engine::TerrainMeshBuilder::CreateMesh(mRenderer.GetDevice(), *mTerrainHeightMap);
        mScene.Build(mCubeMesh, mTerrainMesh, mTerrainHeightMap);
        mCamera = std::make_unique<Kimgane::Engine::SpringArmCamera>();
        mCamera->SetLens(Kimgane::Engine::CameraSettings::DEFAULT_FOV_Y_RAD,
                         static_cast<float>(Kimgane::Engine::WindowSettings::DEFAULT_CLIENT_WIDTH_PX) /
                             static_cast<float>(Kimgane::Engine::WindowSettings::DEFAULT_CLIENT_HEIGHT_PX),
                         Kimgane::Engine::CameraSettings::DEFAULT_NEAR_CLIP_M,
                         Kimgane::Engine::CameraSettings::DEFAULT_FAR_CLIP_M);
        mCamera->UpdateEye(Kimgane::Engine::TestSceneSettings::CAMERA_LOOK_AT_POSITION_M);
        mRenderer.SetCameraPositionM(mCamera->GetEyeM());
        mRenderer.SetViewProjection(mCamera->GetViewProjectionMatrix4x4());

        mGameClock.Reset();
    }

    int RunMessageLoop()
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

    void UpdateAndRender()
    {
        const float deltaTimeSec = mGameClock.Tick();
        mScene.Update(deltaTimeSec);
        mCamera->Update(deltaTimeSec);
        mCamera->UpdateEye(Kimgane::Engine::TestSceneSettings::CAMERA_LOOK_AT_POSITION_M);
        mRenderer.SetCameraPositionM(mCamera->GetEyeM());
        mRenderer.SetViewProjection(mCamera->GetViewProjectionMatrix4x4());
        mRenderer.Render(mScene);
    }

    static LRESULT CALLBACK WindowProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
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

    HWND mWindowHandle = nullptr;
    Kimgane::Engine::GameClock mGameClock;
    std::shared_ptr<Kimgane::Engine::Mesh> mCubeMesh;
    std::shared_ptr<Kimgane::Engine::Mesh> mTerrainMesh;
    std::shared_ptr<Kimgane::Engine::TerrainHeightMap> mTerrainHeightMap;
    std::unique_ptr<Kimgane::Engine::SpringArmCamera> mCamera;
    Kimgane::Engine::TestScene mScene;
    Kimgane::Engine::Dx12Renderer mRenderer;
};
} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previousInstance, PWSTR commandLine, int commandShow)
{
    UNREFERENCED_PARAMETER(previousInstance);
    UNREFERENCED_PARAMETER(commandLine);

    try
    {
        Dx12ClientApp app;
        return app.Run(instance, commandShow);
    }
    catch (const std::exception& exception)
    {
        MessageBoxA(nullptr, exception.what(), "Kimgane.Client", MB_OK | MB_ICONERROR);
        return -1;
    }
}
