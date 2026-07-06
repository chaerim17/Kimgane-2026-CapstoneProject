#pragma once

#include <DirectXMath.h>

namespace Kimgane::Engine
{
class Transform final
{
public:
    Transform() noexcept = default;

    [[nodiscard]] const DirectX::XMFLOAT3& GetPositionM() const noexcept
    {
        return positionM_;
    }

    [[nodiscard]] const DirectX::XMFLOAT3& GetRotationRad() const noexcept
    {
        return rotationRad_;
    }

    [[nodiscard]] const DirectX::XMFLOAT3& GetScale() const noexcept
    {
        return scale_;
    }

    void SetPositionM(const DirectX::XMFLOAT3& positionM) noexcept
    {
        positionM_ = positionM;
    }

    void SetRotationRad(const DirectX::XMFLOAT3& rotationRad) noexcept
    {
        rotationRad_ = rotationRad;
    }

    void SetScale(const DirectX::XMFLOAT3& scale) noexcept
    {
        scale_ = scale;
    }

    void TranslateM(const DirectX::XMFLOAT3& deltaM) noexcept
    {
        positionM_.x += deltaM.x;
        positionM_.y += deltaM.y;
        positionM_.z += deltaM.z;
    }

    [[nodiscard]] DirectX::XMMATRIX GetWorldMatrix() const noexcept
    {
        const DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(scale_.x, scale_.y, scale_.z);
        const DirectX::XMMATRIX rotation =
            DirectX::XMMatrixRotationRollPitchYaw(rotationRad_.x, rotationRad_.y, rotationRad_.z);
        const DirectX::XMMATRIX translation =
            DirectX::XMMatrixTranslation(positionM_.x, positionM_.y, positionM_.z);

        return scale * rotation * translation;
    }

    [[nodiscard]] DirectX::XMFLOAT4X4 GetWorldMatrix4x4() const noexcept
    {
        DirectX::XMFLOAT4X4 worldMatrix = {};
        DirectX::XMStoreFloat4x4(&worldMatrix, GetWorldMatrix());
        return worldMatrix;
    }

private:
    DirectX::XMFLOAT3 positionM_ = {0.0F, 0.0F, 0.0F};
    DirectX::XMFLOAT3 rotationRad_ = {0.0F, 0.0F, 0.0F};
    DirectX::XMFLOAT3 scale_ = {1.0F, 1.0F, 1.0F};
};
} // namespace Kimgane::Engine
