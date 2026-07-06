#include "Pch.h"

#include "Engine/Camera/CameraSettings.h"
#include "Engine/Camera/SpringArmCamera.h"
#include "Engine/Core/GameClock.h"
#include "Engine/Core/WindowSettings.h"
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
        windowClass.lpszClassName = Kimgane::Engine::WindowSettings::kWindowClassName;

        if (RegisterClassExW(&windowClass) == 0U)
        {
            throw std::runtime_error("Failed to register the window class.");
        }

        RECT windowRect = {0,
                           0,
                           static_cast<LONG>(Kimgane::Engine::WindowSettings::kDefaultClientWidthPx),
                           static_cast<LONG>(Kimgane::Engine::WindowSettings::kDefaultClientHeightPx)};
        if (AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE) == FALSE)
        {
            throw std::runtime_error("Failed to adjust the window rectangle.");
        }

        windowHandle_ = CreateWindowExW(0,
                                        Kimgane::Engine::WindowSettings::kWindowClassName,
                                        Kimgane::Engine::WindowSettings::kWindowTitle,
                                        WS_OVERLAPPEDWINDOW,
                                        CW_USEDEFAULT,
                                        CW_USEDEFAULT,
                                        windowRect.right - windowRect.left,
                                        windowRect.bottom - windowRect.top,
                                        nullptr,
                                        nullptr,
                                        instance,
                                        this);

        if (windowHandle_ == nullptr)
        {
            throw std::runtime_error("Failed to create the window.");
        }

        ShowWindow(windowHandle_, commandShow);
    }

    void InitializeClient()
    {
        renderer_.Initialize(windowHandle_,
                             Kimgane::Engine::WindowSettings::kDefaultClientWidthPx,
                             Kimgane::Engine::WindowSettings::kDefaultClientHeightPx);

        cubeMesh_ = Kimgane::Engine::Mesh::CreateCube(renderer_.GetDevice(),
                                                      Kimgane::Engine::TestSceneSettings::kCubeSizeM);
        terrainHeightMap_ =
            Kimgane::Engine::TerrainHeightMap::CreateWaveField(Kimgane::Engine::TerrainSettings::kDefaultSampleWidth,
                                                               Kimgane::Engine::TerrainSettings::kDefaultSampleLength,
                                                               Kimgane::Engine::TerrainSettings::kDefaultCellSpacingM,
                                                               Kimgane::Engine::TerrainSettings::kDefaultWaveAmplitudeM,
                                                               Kimgane::Engine::TerrainSettings::kDefaultWaveFrequency);
        terrainMesh_ = Kimgane::Engine::TerrainMeshBuilder::CreateMesh(renderer_.GetDevice(), *terrainHeightMap_);
        scene_.Build(cubeMesh_, terrainMesh_, terrainHeightMap_);
        camera_ = std::make_unique<Kimgane::Engine::SpringArmCamera>();
        camera_->SetLens(Kimgane::Engine::CameraSettings::kDefaultFovYRad,
                         static_cast<float>(Kimgane::Engine::WindowSettings::kDefaultClientWidthPx) /
                             static_cast<float>(Kimgane::Engine::WindowSettings::kDefaultClientHeightPx),
                         Kimgane::Engine::CameraSettings::kDefaultNearClipM,
                         Kimgane::Engine::CameraSettings::kDefaultFarClipM);
        camera_->UpdateEye(Kimgane::Engine::TestSceneSettings::kCameraLookAtPositionM);
        renderer_.SetViewProjection(camera_->GetViewProjectionMatrix4x4());

        gameClock_.Reset();
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

        renderer_.WaitForGpu();
        return static_cast<int>(message.wParam);
    }

    void UpdateAndRender()
    {
        const float deltaTimeSec = gameClock_.Tick();
        scene_.Update(deltaTimeSec);
        camera_->Update(deltaTimeSec);
        camera_->UpdateEye(Kimgane::Engine::TestSceneSettings::kCameraLookAtPositionM);
        renderer_.SetViewProjection(camera_->GetViewProjectionMatrix4x4());
        renderer_.Render(scene_);
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

    HWND windowHandle_ = nullptr;
    Kimgane::Engine::GameClock gameClock_;
    std::shared_ptr<Kimgane::Engine::Mesh> cubeMesh_;
    std::shared_ptr<Kimgane::Engine::Mesh> terrainMesh_;
    std::shared_ptr<Kimgane::Engine::TerrainHeightMap> terrainHeightMap_;
    std::unique_ptr<Kimgane::Engine::SpringArmCamera> camera_;
    Kimgane::Engine::TestScene scene_;
    Kimgane::Engine::Dx12Renderer renderer_;
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
