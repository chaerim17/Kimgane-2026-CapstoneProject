#pragma once

#include "Scene.h"
#include "../Camera/Camera.h"

#include <array>
#include <cstddef>
#include <memory>

namespace Kimgane::Client
{
class InputManager;

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
    std::array<TextLabel*, static_cast<std::size_t>(TitleMenuOption::Count)> mOptionLabels = {};
    TitleMenuOption mSelectedOption = TitleMenuOption::LocalGame;
    TitleSceneAction mPendingAction = TitleSceneAction::None;
};
} // namespace Kimgane::Client
