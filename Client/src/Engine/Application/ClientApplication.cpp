#include "Pch.h"

#include <iostream>

#include "ClientApplication.h"

#include "../Camera/Camera.h"
#include "../Core/WindowSettings.h"
//#include "../Diagnostics/ComponentSmokeTests.h"
#include "../../Shared/Protocol.h"
#include "../Rendering/FbxModelMesh.h" // 26.07.10 모델 메쉬 로드용 헤더
#include "../Rendering/ObjModelMesh.h"
#include "../Rendering/Mesh.h"
#include "../Scene/TestSceneSettings.h"
#include "../../Shared/Terrain/TerrainHeightMap.h"
#include "../Terrain/TerrainMesh.h"
#include "../../Shared/Terrain/TerrainSettings.h"

#include <cstdio>
#include <stdexcept>
#include <string>

namespace Kimgane::Engine
{
ClientApplication::~ClientApplication()
{
    mNetwork.Shutdown();
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
    mInputManager.Initialize(mWindowHandle);

    // 네트워크 연결 디버깅용 콘솔
    //-------------------------------------------------
    AllocConsole();
    FILE* file = nullptr;
    freopen_s(&file, "CONOUT$", "w", stdout);
    freopen_s(&file, "CONIN$", "r", stdin);
    //-------------------------------------------------

    mRenderer.Initialize(mWindowHandle, WindowSettings::DEFAULT_CLIENT_WIDTH_PX,
                         WindowSettings::DEFAULT_CLIENT_HEIGHT_PX);
    // Diagnostics::RunClientComponentSmokeTests(mRenderer.GetDevice());

    CreateTestAssets();
    BuildTitleAndOverlayScenes();
    SyncCameraToScene();
    RefreshWindowTitle();
    mGameClock.Reset();
}

//void ClientApplication::InitializeNetwork()
//{
//    // 클라이언트 실행 시 네트워크 계층을 한 번 초기화합니다.
//    // 실제 서버 구현이 들어와도 Main.cpp는 건드리지 않고 이 흐름을 유지합니다.
//    mNetwork.InitializeNetwork();
//}

void ClientApplication::CreateTestAssets()
{
    mCubeMesh = Mesh::CreateCube(mRenderer.GetDevice(), TestSceneSettings::CUBE_SIZE_M);
    mPlayerModelMesh = FbxModelMesh::Load(mRenderer.GetDevice(), TestSceneSettings::PLAYER_MODEL_PATH);     // 26.07.10 모델 메쉬 로드
    mNpcModelMesh = FbxModelMesh::Load(mRenderer.GetDevice(), TestSceneSettings::NPC_MODEL_PATH); // NPC 모델 메쉬 로드
    mHouseModelMesh = ObjModelMesh::Load(mRenderer.GetDevice(), TestSceneSettings::HOUSE_MODEL_PATH);
    mUiMesh = Mesh::CreateCube(mRenderer.GetDevice(), 1.0F);
    mTerrainHeightMap = TerrainHeightMap::LoadRaw8(TerrainSettings::RAW_HEIGHTMAP_PATH,
                                                   TerrainSettings::RAW_SAMPLE_WIDTH,
                                                   TerrainSettings::RAW_SAMPLE_LENGTH,
                                                   TerrainSettings::RAW_CELL_SPACING_M,
                                                   TerrainSettings::RAW_HEIGHT_SCALE_M);
    mTerrainMesh = TerrainMeshBuilder::CreateMesh(mRenderer.GetDevice(), *mTerrainHeightMap);
}

void ClientApplication::BuildTitleAndOverlayScenes()
{
    mTitleScene.Build(mUiMesh, mInputManager, GetCameraAspectRatio());
    mSettingsOverlayScene.Build(mUiMesh, mInputManager, mShowFpsInWindowTitle, GetCameraAspectRatio());
}

void ClientApplication::BuildGameScene(GameScene& scene)
{
    scene.Build(mCubeMesh,
                mPlayerModelMesh,
                mNpcModelMesh,
                mHouseModelMesh,
                mTerrainMesh,
                mTerrainHeightMap,
                mInputManager,
                mNetwork,
                GetCameraAspectRatio());
}

void ClientApplication::EnterLocalGameScene()
{
    if (mNetwork.IsConnected())
    {
        mNetwork.Shutdown();
    }

    auto scene = std::make_unique<LocalGameScene>();
    BuildGameScene(*scene);
    mActiveGameScene = std::move(scene);
    mActiveSceneType = ActiveSceneType::LocalGame;
    mSettingsOverlayVisible = false;
    SyncCameraToScene();
    RefreshWindowTitle();
}

void ClientApplication::EnterOnlineGameScene()
{
    if (!mNetwork.IsConnected())
    {
        mNetwork.Initialize();
    }

    auto scene = std::make_unique<OnlineGameScene>();
    BuildGameScene(*scene);
    mActiveGameScene = std::move(scene);
    mActiveSceneType = ActiveSceneType::OnlineGame;
    mSettingsOverlayVisible = false;
    SyncCameraToScene();
    RefreshWindowTitle();
}

void ClientApplication::OpenSettingsOverlay()
{
    mSettingsOverlayScene.SetFpsInWindowTitleEnabled(mShowFpsInWindowTitle);
    mSettingsOverlayVisible = true;
    RefreshWindowTitle();
}

void ClientApplication::CloseSettingsOverlay()
{
    mSettingsOverlayVisible = false;
    RefreshWindowTitle();
}

float ClientApplication::GetCameraAspectRatio() const noexcept
{
    return static_cast<float>(WindowSettings::DEFAULT_CLIENT_WIDTH_PX) /
           static_cast<float>(WindowSettings::DEFAULT_CLIENT_HEIGHT_PX);
}

void ClientApplication::SyncCameraToScene()
{
    if (mActiveGameScene != nullptr)
    {
        mActiveGameScene->RefreshGameplayCamera();
    }

    const Camera* camera = GetActiveSceneCamera();
    if (camera == nullptr)
    {
        return;
    }

    mRenderer.SetCameraPositionM(camera->GetEyeM());
    mRenderer.SetViewProjection(camera->GetViewProjectionMatrix4x4());
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
    SendLocalPlayerStatePacket(deltaTimeSec);
    mNetwork.Update(deltaTimeSec);
    ApplyNetworkPlayerLocations();
    UpdateCamera(deltaTimeSec);
    Render();
    UpdateWindowTitle(deltaTimeSec);
}

void ClientApplication::ProcessInput()
{
    const bool acceptsInput = mWindowHandle != nullptr && GetForegroundWindow() == mWindowHandle;
    mInputManager.Update(acceptsInput);
}

void ClientApplication::SendLocalPlayerStatePacket(float deltaTimeSec)
{
    const GameScene* gameScene = GetActiveGameScene();
    if (mActiveSceneType != ActiveSceneType::OnlineGame || gameScene == nullptr ||
        !mNetwork.ConsumePlayerStateSyncTick(deltaTimeSec))
    {
        return;
    }

    // 서버 요청: PlayerState는 클라 예측 위치 보고 / 서버 오차 확인용으로 전송합니다.
    mNetwork.SendPlayerState(gameScene->GetLocalPlayerPositionM(), gameScene->GetLocalPlayerYaw(), false);
}

void ClientApplication::ApplyNetworkPlayerLocations()
{
    GameScene* gameScene = GetActiveGameScene();
    if (mActiveSceneType != ActiveSceneType::OnlineGame || gameScene == nullptr)
    {
        return;
    }

    int playerId;
    float x;
    float y;
    float z;
    float yaw;

    while (mNetwork.GetPlayerLocation(&playerId, &x, &y, &z, &yaw))
    {
        //std::cout << "[APPLY] " << playerId << " (" << x << ", " << y << ", " << z << ", " << yaw << ")\n";
        gameScene->UpdateNetworkPlayerPosition(playerId, {x, y, z}, yaw);
    }

    int removedPlayerId;

    while (mNetwork.GetRemovedPlayer(&removedPlayerId))
    {
        gameScene->RemoveNetworkPlayer(removedPlayerId);
    }
}

void ClientApplication::UpdateScene(float deltaTimeSec)
{
    if (mSettingsOverlayVisible)
    {
        mSettingsOverlayScene.Update(deltaTimeSec);
        HandleSettingsOverlayAction();
        return;
    }

    if (mActiveSceneType == ActiveSceneType::Title)
    {
        mTitleScene.Update(deltaTimeSec);
        HandleTitleSceneAction();
        return;
    }

    if (mInputManager.WasKeyPressed(InputKey::Cancel))
    {
        OpenSettingsOverlay();
        return;
    }

    if (mActiveGameScene != nullptr)
    {
        mActiveGameScene->Update(deltaTimeSec);
    }
}

void ClientApplication::HandleTitleSceneAction()
{
    switch (mTitleScene.ConsumePendingAction())
    {
    case TitleSceneAction::StartLocalGame:
        EnterLocalGameScene();
        break;
    case TitleSceneAction::StartOnlineGame:
        EnterOnlineGameScene();
        break;
    case TitleSceneAction::OpenSettings:
        OpenSettingsOverlay();
        break;
    case TitleSceneAction::None:
    default:
        break;
    }
}

void ClientApplication::HandleSettingsOverlayAction()
{
    mShowFpsInWindowTitle = mSettingsOverlayScene.IsFpsInWindowTitleEnabled();

    if (mSettingsOverlayScene.ConsumeCloseRequested())
    {
        CloseSettingsOverlay();
        return;
    }

    RefreshWindowTitle();
}

void ClientApplication::UpdateCamera(float deltaTimeSec)
{
    (void)deltaTimeSec;
    SyncCameraToScene();
}

void ClientApplication::Render()
{
    const Scene* activeScene = GetActiveScene();
    if (activeScene == nullptr)
    {
        return;
    }

    SyncCameraToScene();
    mRenderer.BeginFrame();

    const Camera* activeCamera = GetActiveSceneCamera();
    if (activeCamera != nullptr)
    {
        mRenderer.SetCameraPositionM(activeCamera->GetEyeM());
        mRenderer.SetViewProjection(activeCamera->GetViewProjectionMatrix4x4());
    }
    mRenderer.RenderScene(*activeScene, RenderPass::World, !mSettingsOverlayVisible);

    if (mSettingsOverlayVisible)
    {
        const Camera* overlayCamera = mSettingsOverlayScene.GetUiCamera();
        if (overlayCamera != nullptr)
        {
            mRenderer.SetCameraPositionM(overlayCamera->GetEyeM());
            mRenderer.SetViewProjection(overlayCamera->GetViewProjectionMatrix4x4());
        }
        mRenderer.RenderScene(mSettingsOverlayScene, RenderPass::Overlay);
    }

    mRenderer.EndFrame();
}

void ClientApplication::UpdateWindowTitle(float deltaTimeSec)
{
    ++mFpsFrameCount;
    mFpsElapsedTimeSec += std::max(deltaTimeSec, 0.0F);

    if (mFpsElapsedTimeSec < 0.25F)
    {
        return;
    }

    mLastFps = static_cast<float>(mFpsFrameCount) / mFpsElapsedTimeSec;
    mFpsFrameCount = 0U;
    mFpsElapsedTimeSec = 0.0F;
    RefreshWindowTitle();
}

void ClientApplication::RefreshWindowTitle() const
{
    if (mWindowHandle == nullptr)
    {
        return;
    }

    std::wstring title = WindowSettings::WINDOW_TITLE;

    if (mShowFpsInWindowTitle)
    {
        wchar_t fpsText[32] = {};
        swprintf_s(fpsText, L" | FPS %.1f", static_cast<double>(mLastFps));
        title += fpsText;
    }

    SetWindowTextW(mWindowHandle, title.c_str());
}

const Scene* ClientApplication::GetActiveScene() const noexcept
{
    switch (mActiveSceneType)
    {
    case ActiveSceneType::Title:
        return &mTitleScene;
    case ActiveSceneType::LocalGame:
    case ActiveSceneType::OnlineGame:
        return mActiveGameScene.get();
    default:
        return nullptr;
    }
}

GameScene* ClientApplication::GetActiveGameScene() noexcept
{
    return mActiveGameScene.get();
}

const GameScene* ClientApplication::GetActiveGameScene() const noexcept
{
    return mActiveGameScene.get();
}

const Camera* ClientApplication::GetActiveSceneCamera() const noexcept
{
    if (mActiveSceneType == ActiveSceneType::Title)
    {
        return mTitleScene.GetUiCamera();
    }

    return mActiveGameScene != nullptr ? mActiveGameScene->GetGameplayCamera() : nullptr;
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
