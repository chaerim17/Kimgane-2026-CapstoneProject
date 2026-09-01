#include "Pch.h"

#include "InputManager.h"

#include "InputSettings.h"

#include <algorithm>
#include <cmath>

namespace Kimgane::Engine
{
namespace
{
bool IsVirtualKeyDown(int virtualKey) noexcept
{
    return (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
}

DirectX::XMFLOAT2 NormalizeAxis(float x, float y) noexcept
{
    const float lengthSq = x * x + y * y;
    if (lengthSq <= 0.000001F)
    {
        return {0.0F, 0.0F};
    }

    const float invLength = 1.0F / std::sqrt(lengthSq);
    return {x * invLength, y * invLength};
}
} // namespace

void InputManager::Initialize(HWND windowHandle) noexcept
{
    mWindowHandle = windowHandle;
}

void InputManager::BeginFrame() noexcept
{
    mPreviousKeys = mCurrentKeys;
}

void InputManager::Update() noexcept
{
    Update(true);
}

void InputManager::Update(bool acceptsInput) noexcept
{
    BeginFrame();

    if (!acceptsInput)
    {
        mCurrentKeys.fill(false);
        UpdateMouseDelta(acceptsInput);
        return;
    }

    SetKeyDown(InputKey::MoveForward, IsVirtualKeyDown(InputSettings::MOVE_FORWARD_VIRTUAL_KEY));
    SetKeyDown(InputKey::MoveBackward, IsVirtualKeyDown(InputSettings::MOVE_BACKWARD_VIRTUAL_KEY));
    SetKeyDown(InputKey::MoveLeft, IsVirtualKeyDown(InputSettings::MOVE_LEFT_VIRTUAL_KEY));
    SetKeyDown(InputKey::MoveRight, IsVirtualKeyDown(InputSettings::MOVE_RIGHT_VIRTUAL_KEY));
    SetKeyDown(InputKey::Jump, IsVirtualKeyDown(InputSettings::JUMP_VIRTUAL_KEY));
    SetKeyDown(InputKey::MenuUp,
               IsVirtualKeyDown(InputSettings::MENU_UP_VIRTUAL_KEY) ||
                   IsVirtualKeyDown(InputSettings::MOVE_FORWARD_VIRTUAL_KEY));
    SetKeyDown(InputKey::MenuDown,
               IsVirtualKeyDown(InputSettings::MENU_DOWN_VIRTUAL_KEY) ||
                   IsVirtualKeyDown(InputSettings::MOVE_BACKWARD_VIRTUAL_KEY));
    SetKeyDown(InputKey::Confirm,
               IsVirtualKeyDown(InputSettings::CONFIRM_VIRTUAL_KEY) ||
                   IsVirtualKeyDown(InputSettings::JUMP_VIRTUAL_KEY));
    SetKeyDown(InputKey::Cancel, IsVirtualKeyDown(InputSettings::CANCEL_VIRTUAL_KEY));
    UpdateMouseDelta(acceptsInput);
}

void InputManager::Reset() noexcept
{
    mCurrentKeys.fill(false);
    mPreviousKeys.fill(false);
}

void InputManager::SetKeyDown(InputKey key, bool isDown) noexcept
{
    if (!IsValidKey(key))
    {
        return;
    }

    mCurrentKeys[ToIndex(key)] = isDown;
}

bool InputManager::IsKeyDown(InputKey key) const noexcept
{
    return IsValidKey(key) && mCurrentKeys[ToIndex(key)];
}

bool InputManager::WasKeyPressed(InputKey key) const noexcept
{
    return IsValidKey(key) && mCurrentKeys[ToIndex(key)] && !mPreviousKeys[ToIndex(key)];
}

bool InputManager::WasKeyReleased(InputKey key) const noexcept
{
    return IsValidKey(key) && !mCurrentKeys[ToIndex(key)] && mPreviousKeys[ToIndex(key)];
}

InputState InputManager::GetState() const noexcept
{
    float moveX = 0.0F;
    float moveY = 0.0F;

    if (IsKeyDown(InputKey::MoveRight))
    {
        moveX += 1.0F;
    }

    if (IsKeyDown(InputKey::MoveLeft))
    {
        moveX -= 1.0F;
    }

    if (IsKeyDown(InputKey::MoveForward))
    {
        moveY += 1.0F;
    }

    if (IsKeyDown(InputKey::MoveBackward))
    {
        moveY -= 1.0F;
    }

    InputState state = {};
    state.mMoveAxis = NormalizeAxis(moveX, moveY);
    state.mMouseDeltaPx = mMouseDeltaPx;
    state.mJumpDown = IsKeyDown(InputKey::Jump);
    state.mJumpPressed = WasKeyPressed(InputKey::Jump);
    return state;
}

std::size_t InputManager::ToIndex(InputKey key) noexcept
{
    return static_cast<std::size_t>(key);
}

bool InputManager::IsValidKey(InputKey key) noexcept
{
    return ToIndex(key) < KEY_COUNT;
}

void InputManager::UpdateMouseDelta(bool acceptsInput) noexcept
{
    if (!acceptsInput || mWindowHandle == nullptr)
    {
        if (mCursorHidden)
        {
            ShowCursor(TRUE);
            mCursorHidden = false;
        }
        mMouseDeltaPx = {0.0F, 0.0F};
        return;
    }

    if (!mCursorHidden)
    {
        ShowCursor(FALSE);
        mCursorHidden = true;
    }

    RECT clientRect = {};
    GetClientRect(mWindowHandle, &clientRect);
    POINT centerPoint = {(clientRect.right - clientRect.left) / 2, (clientRect.bottom - clientRect.top) / 2};
    ClientToScreen(mWindowHandle, &centerPoint);

    POINT currentPoint = {};
    GetCursorPos(&currentPoint);

    mMouseDeltaPx = {static_cast<float>(currentPoint.x - centerPoint.x),
                     static_cast<float>(currentPoint.y - centerPoint.y)};

    SetCursorPos(centerPoint.x, centerPoint.y);
}

} // namespace Kimgane::Engine
