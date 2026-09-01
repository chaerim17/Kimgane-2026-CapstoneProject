#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>

#include <DirectXMath.h>

#include <array>
#include <cstddef>

namespace Kimgane::Engine
{
enum class InputKey : std::size_t
{
    MoveForward,
    MoveBackward,
    MoveLeft,
    MoveRight,
    Jump,
    MenuUp,
    MenuDown,
    Confirm,
    Cancel,
    ToggleColliderDebug,
    Count
};

struct InputState
{
    DirectX::XMFLOAT2 mMoveAxis = {0.0F, 0.0F};
    DirectX::XMFLOAT2 mMouseDeltaPx = {0.0F, 0.0F};
    bool mJumpDown = false;
    bool mJumpPressed = false;
};

class InputManager final
{
public:
    void Initialize(HWND windowHandle) noexcept;
    void BeginFrame() noexcept;
    void Update() noexcept;
    void Update(bool acceptsInput) noexcept;
    void Reset() noexcept;

    void SetKeyDown(InputKey key, bool isDown) noexcept;

    [[nodiscard]] bool IsKeyDown(InputKey key) const noexcept;
    [[nodiscard]] bool WasKeyPressed(InputKey key) const noexcept;
    [[nodiscard]] bool WasKeyReleased(InputKey key) const noexcept;
    [[nodiscard]] InputState GetState() const noexcept;

private:
    [[nodiscard]] static std::size_t ToIndex(InputKey key) noexcept;
    [[nodiscard]] static bool IsValidKey(InputKey key) noexcept;
    void UpdateMouseDelta(bool acceptsInput) noexcept;

    static constexpr std::size_t KEY_COUNT = static_cast<std::size_t>(InputKey::Count);
    
    std::array<bool, KEY_COUNT> mCurrentKeys = {};
    std::array<bool, KEY_COUNT> mPreviousKeys = {};
    HWND mWindowHandle = nullptr;
    DirectX::XMFLOAT2 mMouseDeltaPx = {0.0F, 0.0F};
    bool mCursorHidden = false;
};
} // namespace Kimgane::Engine
