#pragma once

#include "../Core/Component.h"
#include "SpringArmCamera.h"

#include <DirectXMath.h>

namespace Kimgane::Engine
{
class CameraComponent final : public Component
{
public:
    CameraComponent(GameObject& owner, const DirectX::XMFLOAT3& targetOffsetM) noexcept;

    void Update(float deltaTimeSec) override;

    void SetLens(float fovYRad, float aspectRatio, float nearZM, float farZM) noexcept;
    void SetTargetOffsetM(const DirectX::XMFLOAT3& targetOffsetM) noexcept;
    void Refresh() noexcept;

    [[nodiscard]] Camera& GetCamera() noexcept;
    [[nodiscard]] const Camera& GetCamera() const noexcept;
    [[nodiscard]] SpringArmCamera& GetSpringArmCamera() noexcept;
    [[nodiscard]] const SpringArmCamera& GetSpringArmCamera() const noexcept;
    [[nodiscard]] const DirectX::XMFLOAT3& GetTargetOffsetM() const noexcept;

private:
    SpringArmCamera mCamera;
    DirectX::XMFLOAT3 mTargetOffsetM = {0.0F, 0.0F, 0.0F};
};
} // namespace Kimgane::Engine
