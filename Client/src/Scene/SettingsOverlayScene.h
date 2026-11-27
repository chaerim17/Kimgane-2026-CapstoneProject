#pragma once

#include "Scene.h"
#include "../Camera/Camera.h"

#include <memory>

namespace Kimgane::Client
{
class InputManager;

// 게임 화면 위에 겹쳐 띄우는 UI 씬의 공통 부모. 직교 UI 카메라를 가짐.
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
    TextLabel* mFpsToggleLabel = nullptr;
    bool mFpsInWindowTitleEnabled = true;
    bool mCloseRequested = false;
};
} // namespace Kimgane::Client
