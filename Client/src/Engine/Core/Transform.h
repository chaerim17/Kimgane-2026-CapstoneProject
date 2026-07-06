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
        return mPositionM;
    }

    [[nodiscard]] const DirectX::XMFLOAT3& GetRotationRad() const noexcept
    {
        return mRotationRad;
    }

    [[nodiscard]] const DirectX::XMFLOAT3& GetScale() const noexcept
    {
        return mScale;
    }

    void SetPositionM(const DirectX::XMFLOAT3& positionM) noexcept
    {
        mPositionM = positionM;
    }

    void SetRotationRad(const DirectX::XMFLOAT3& rotationRad) noexcept
    {
        mRotationRad = rotationRad;
    }

    void SetScale(const DirectX::XMFLOAT3& scale) noexcept
    {
        mScale = scale;
    }

    void TranslateM(const DirectX::XMFLOAT3& deltaM) noexcept
    {
        mPositionM.x += deltaM.x;
        mPositionM.y += deltaM.y;
        mPositionM.z += deltaM.z;
    }

    [[nodiscard]] DirectX::XMMATRIX GetWorldMatrix() const noexcept
    {
        const DirectX::XMMATRIX scale = DirectX::XMMatrixScaling(mScale.x, mScale.y, mScale.z);
        const DirectX::XMMATRIX rotation =
            DirectX::XMMatrixRotationRollPitchYaw(mRotationRad.x, mRotationRad.y, mRotationRad.z);
        const DirectX::XMMATRIX translation =
            DirectX::XMMatrixTranslation(mPositionM.x, mPositionM.y, mPositionM.z);

        return scale * rotation * translation;
    }

    [[nodiscard]] DirectX::XMFLOAT4X4 GetWorldMatrix4x4() const noexcept
    {
        DirectX::XMFLOAT4X4 worldMatrix = {};
        DirectX::XMStoreFloat4x4(&worldMatrix, GetWorldMatrix());
        return worldMatrix;
    }

private:
    DirectX::XMFLOAT3 mPositionM = {0.0F, 0.0F, 0.0F};
    DirectX::XMFLOAT3 mRotationRad = {0.0F, 0.0F, 0.0F};
    DirectX::XMFLOAT3 mScale = {1.0F, 1.0F, 1.0F};
};
} // namespace Kimgane::Engine
