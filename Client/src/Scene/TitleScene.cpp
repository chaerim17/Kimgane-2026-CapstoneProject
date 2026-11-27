#include "Pch.h"

#include "TitleScene.h"

#include "MenuWidgets.h"
#include "../Input/InputManager.h"

#include <utility>

namespace Kimgane::Client
{
namespace
{
std::size_t ToIndex(TitleMenuOption option) noexcept
{
    return static_cast<std::size_t>(option);
}

const wchar_t* GetTitleOptionLabelW(TitleMenuOption option) noexcept
{
    switch (option)
    {
    case TitleMenuOption::LocalGame:
        return L"로컬 테스트";
    case TitleMenuOption::OnlineGame:
        return L"서버 접속";
    case TitleMenuOption::Settings:
        return L"설정";
    default:
        return L"알 수 없음";
    }
}
} // namespace

void TitleScene::Build(std::shared_ptr<Mesh> uiMesh, const InputManager& inputManager, float cameraAspectRatio)
{
    Clear();
    mInputManager = &inputManager;
    mSelectedOption = TitleMenuOption::LocalGame;
    mPendingAction = TitleSceneAction::None;
    mOptionPanels.fill(nullptr);
    mOptionLabels.fill(nullptr);

    ConfigureUiCamera(mUiCamera, cameraAspectRatio);

    const float uiWidthM = GetUiOrthographicWidthM(cameraAspectRatio);
    CreateUiPanel(*this,
                  uiMesh,
                  "Title Background",
                  {0.0F, 0.0F, 1.0F},
                  {uiWidthM, UI_ORTHOGRAPHIC_HEIGHT_M, 0.1F},
                  {0.05F, 0.07F, 0.10F, 1.0F});
    CreateUiPanel(*this, uiMesh, "Title Header", {0.0F, 2.9F, 0.0F}, {6.8F, 1.2F, 0.1F}, {0.78F, 0.22F, 0.18F, 1.0F});

    GameObject& localGameOption =
        CreateUiPanel(*this, uiMesh, "Title Option Local", {0.0F, 1.1F, 0.0F}, {4.8F, 0.8F, 0.1F}, {0.12F, 0.18F, 0.25F, 1.0F});
    mOptionPanels[ToIndex(TitleMenuOption::LocalGame)] = &localGameOption;
    mOptionLabels[ToIndex(TitleMenuOption::LocalGame)] =
        &AddUiButtonLabel(*this, localGameOption, GetTitleOptionLabelW(TitleMenuOption::LocalGame), 32.0F);

    GameObject& onlineGameOption =
        CreateUiPanel(*this, uiMesh, "Title Option Online", {0.0F, 0.0F, 0.0F}, {4.8F, 0.8F, 0.1F}, {0.12F, 0.18F, 0.25F, 1.0F});
    mOptionPanels[ToIndex(TitleMenuOption::OnlineGame)] = &onlineGameOption;
    mOptionLabels[ToIndex(TitleMenuOption::OnlineGame)] =
        &AddUiButtonLabel(*this, onlineGameOption, GetTitleOptionLabelW(TitleMenuOption::OnlineGame), 32.0F);

    GameObject& settingsOption =
        CreateUiPanel(*this, uiMesh, "Title Option Settings", {0.0F, -1.1F, 0.0F}, {4.8F, 0.8F, 0.1F}, {0.12F, 0.18F, 0.25F, 1.0F});
    mOptionPanels[ToIndex(TitleMenuOption::Settings)] = &settingsOption;
    mOptionLabels[ToIndex(TitleMenuOption::Settings)] =
        &AddUiButtonLabel(*this, settingsOption, GetTitleOptionLabelW(TitleMenuOption::Settings), 32.0F);

    RefreshVisualState();
}

void TitleScene::Update(float deltaTimeSec)
{
    (void)deltaTimeSec;

    if (mInputManager == nullptr)
    {
        return;
    }

    if (mInputManager->WasKeyPressed(InputKey::MenuUp))
    {
        MoveSelection(-1);
    }

    if (mInputManager->WasKeyPressed(InputKey::MenuDown))
    {
        MoveSelection(1);
    }

    if (mInputManager->WasKeyPressed(InputKey::Cancel))
    {
        mPendingAction = TitleSceneAction::OpenSettings;
    }

    if (mInputManager->WasKeyPressed(InputKey::Confirm))
    {
        switch (mSelectedOption)
        {
        case TitleMenuOption::LocalGame:
            mPendingAction = TitleSceneAction::StartLocalGame;
            break;
        case TitleMenuOption::OnlineGame:
            mPendingAction = TitleSceneAction::StartOnlineGame;
            break;
        case TitleMenuOption::Settings:
            mPendingAction = TitleSceneAction::OpenSettings;
            break;
        default:
            mPendingAction = TitleSceneAction::None;
            break;
        }
    }
}

const Camera* TitleScene::GetUiCamera() const noexcept
{
    return &mUiCamera;
}

TitleSceneAction TitleScene::ConsumePendingAction() noexcept
{
    const TitleSceneAction action = mPendingAction;
    mPendingAction = TitleSceneAction::None;
    return action;
}

TitleMenuOption TitleScene::GetSelectedOption() const noexcept
{
    return mSelectedOption;
}

const wchar_t* TitleScene::GetSelectedOptionLabelW() const noexcept
{
    return GetTitleOptionLabelW(mSelectedOption);
}

void TitleScene::MoveSelection(int direction) noexcept
{
    constexpr int OPTION_COUNT = static_cast<int>(TitleMenuOption::Count);
    int selectedIndex = static_cast<int>(mSelectedOption);
    selectedIndex = (selectedIndex + direction + OPTION_COUNT) % OPTION_COUNT;
    mSelectedOption = static_cast<TitleMenuOption>(selectedIndex);
    RefreshVisualState();
}

void TitleScene::RefreshVisualState() noexcept
{
    for (std::size_t index = 0; index < mOptionPanels.size(); ++index)
    {
        GameObject* panel = mOptionPanels[index];
        if (panel == nullptr)
        {
            continue;
        }

        const bool selected = index == ToIndex(mSelectedOption);
        panel->GetMaterial().SetBaseColorLinear(selected ? DirectX::XMFLOAT4{0.12F, 0.62F, 0.36F, 1.0F}
                                                                     : DirectX::XMFLOAT4{0.12F, 0.18F, 0.25F, 1.0F});

        TextLabel* label = mOptionLabels[index];
        if (label != nullptr)
        {
            label->SetColorLinear(selected ? DirectX::XMFLOAT4{1.0F, 1.0F, 1.0F, 1.0F}
                                           : DirectX::XMFLOAT4{0.72F, 0.78F, 0.86F, 1.0F});
        }
    }
}

} // namespace Kimgane::Client
