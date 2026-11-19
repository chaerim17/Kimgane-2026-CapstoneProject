#include "Pch.h"

#include "SettingsOverlayScene.h"

#include "MenuWidgets.h"
#include "../Input/InputManager.h"

#include <utility>

namespace Kimgane::Client
{
namespace
{
const wchar_t* GetFpsToggleLabelW(bool enabled) noexcept
{
    return enabled ? L"FPS 표시 켜짐" : L"FPS 표시 꺼짐";
}
} // namespace

const Camera* OverlayScene::GetUiCamera() const noexcept
{
    return &mUiCamera;
}

void OverlayScene::ConfigureOverlayCamera(float cameraAspectRatio) noexcept
{
    ConfigureUiCamera(mUiCamera, cameraAspectRatio);
}

void SettingsOverlayScene::Build(std::shared_ptr<Mesh> uiMesh,
                                 const InputManager& inputManager,
                                 bool fpsInWindowTitleEnabled,
                                 float cameraAspectRatio)
{
    Clear();
    mInputManager = &inputManager;
    mFpsInWindowTitleEnabled = fpsInWindowTitleEnabled;
    mCloseRequested = false;
    mFpsTogglePanel = nullptr;
    mFpsToggleLabel = nullptr;

    ConfigureOverlayCamera(cameraAspectRatio);

    const float uiWidthM = GetUiOrthographicWidthM(cameraAspectRatio);
    CreateUiPanel(*this,
                  uiMesh,
                  "Settings Dim Background",
                  {0.0F, 0.0F, 1.0F},
                  {uiWidthM, UI_ORTHOGRAPHIC_HEIGHT_M, 0.1F},
                  {0.0F, 0.0F, 0.0F, 0.45F});
    CreateUiPanel(*this, uiMesh, "Settings Panel", {0.0F, 0.0F, 0.0F}, {5.8F, 3.4F, 0.1F}, {0.10F, 0.14F, 0.20F, 0.86F});

    GameObject& fpsToggle =
        CreateUiPanel(*this, uiMesh, "Settings FPS Toggle", {0.0F, 0.55F, -0.1F}, {3.8F, 0.8F, 0.1F}, {0.12F, 0.62F, 0.36F, 0.95F});
    mFpsTogglePanel = &fpsToggle;
    mFpsToggleLabel = &AddUiButtonLabel(*this, fpsToggle, GetFpsToggleLabelW(mFpsInWindowTitleEnabled), 28.0F);

    GameObject& closeHint =
        CreateUiPanel(*this, uiMesh, "Settings Close Hint", {0.0F, -1.05F, -0.1F}, {2.8F, 0.55F, 0.1F}, {0.78F, 0.22F, 0.18F, 0.95F});
    AddUiButtonLabel(*this, closeHint, L"닫기", 24.0F);

    RefreshVisualState();
}

void SettingsOverlayScene::Update(float deltaTimeSec)
{
    (void)deltaTimeSec;

    if (mInputManager == nullptr)
    {
        return;
    }

    if (mInputManager->WasKeyPressed(InputKey::Confirm))
    {
        SetFpsInWindowTitleEnabled(!mFpsInWindowTitleEnabled);
    }

    if (mInputManager->WasKeyPressed(InputKey::Cancel))
    {
        mCloseRequested = true;
    }
}

void SettingsOverlayScene::SetFpsInWindowTitleEnabled(bool enabled) noexcept
{
    mFpsInWindowTitleEnabled = enabled;
    RefreshVisualState();
}

bool SettingsOverlayScene::IsFpsInWindowTitleEnabled() const noexcept
{
    return mFpsInWindowTitleEnabled;
}

bool SettingsOverlayScene::ConsumeCloseRequested() noexcept
{
    const bool closeRequested = mCloseRequested;
    mCloseRequested = false;
    return closeRequested;
}

void SettingsOverlayScene::RefreshVisualState() noexcept
{
    if (mFpsTogglePanel == nullptr)
    {
        return;
    }

    mFpsTogglePanel->GetMaterial().SetBaseColorLinear(mFpsInWindowTitleEnabled ? DirectX::XMFLOAT4{0.12F, 0.62F, 0.36F, 0.95F}
                                                                                 : DirectX::XMFLOAT4{0.72F, 0.18F, 0.15F, 0.95F});

    if (mFpsToggleLabel != nullptr)
    {
        mFpsToggleLabel->SetText(GetFpsToggleLabelW(mFpsInWindowTitleEnabled));
        mFpsToggleLabel->SetColorLinear({1.0F, 1.0F, 1.0F, 1.0F});
    }
}

} // namespace Kimgane::Client
