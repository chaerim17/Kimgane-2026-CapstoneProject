#pragma once

#include "../Core/Component.h"
#include "SpringArmCamera.h"

#include <DirectXMath.h>

namespace Kimgane::Engine
{
class InputManager;

class CameraComponent final : public Component
{
public:
    CameraComponent(GameObject& owner, const InputManager& inputManager, const DirectX::XMFLOAT3& targetOffsetM) noexcept;

    void Update(float deltaTimeSec) override;

    void SetLens(float fovYRad, float aspectRatio, float nearZM, float farZM) noexcept;
    void SetTargetOffsetM(const DirectX::XMFLOAT3& targetOffsetM) noexcept;
    void Refresh() noexcept;
    // 캐릭터의 보간된 표시 위치를 사용하며 소유 객체의 물리 Transform은 바꾸지 않습니다.
    void RefreshAtPosition(const DirectX::XMFLOAT3& ownerPositionM) noexcept;

    [[nodiscard]] Camera& GetCamera() noexcept;
    [[nodiscard]] const Camera& GetCamera() const noexcept;
    [[nodiscard]] SpringArmCamera& GetSpringArmCamera() noexcept;
    [[nodiscard]] const SpringArmCamera& GetSpringArmCamera() const noexcept;
    [[nodiscard]] const DirectX::XMFLOAT3& GetTargetOffsetM() const noexcept;

private:
    const InputManager& mInputManager;
    SpringArmCamera mCamera;
    DirectX::XMFLOAT3 mTargetOffsetM = {0.0F, 0.0F, 0.0F};
};
} // namespace Kimgane::Engine
