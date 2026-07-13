#pragma once

#include <DirectXMath.h>

namespace Kimgane::Engine::VectorMath
{
inline DirectX::XMFLOAT3 Store(DirectX::FXMVECTOR value) noexcept
{
    DirectX::XMFLOAT3 result = {};
    DirectX::XMStoreFloat3(&result, value);
    return result;
}

inline DirectX::XMFLOAT3 Add(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs) noexcept
{
    return Store(DirectX::XMVectorAdd(DirectX::XMLoadFloat3(&lhs), DirectX::XMLoadFloat3(&rhs)));
}

inline DirectX::XMFLOAT3 Subtract(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs) noexcept
{
    return Store(DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&lhs), DirectX::XMLoadFloat3(&rhs)));
}

inline DirectX::XMFLOAT3 Scale(const DirectX::XMFLOAT3& value, float scalar) noexcept
{
    return Store(DirectX::XMVectorScale(DirectX::XMLoadFloat3(&value), scalar));
}

inline DirectX::XMFLOAT3 Cross(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs) noexcept
{
    return Store(DirectX::XMVector3Cross(DirectX::XMLoadFloat3(&lhs), DirectX::XMLoadFloat3(&rhs)));
}

inline float Dot(const DirectX::XMFLOAT3& lhs, const DirectX::XMFLOAT3& rhs) noexcept
{
    return DirectX::XMVectorGetX(DirectX::XMVector3Dot(DirectX::XMLoadFloat3(&lhs), DirectX::XMLoadFloat3(&rhs)));
}

inline DirectX::XMFLOAT3 NormalizeOrFallback(const DirectX::XMFLOAT3& value,
                                             const DirectX::XMFLOAT3& fallback) noexcept
{
    const DirectX::XMVECTOR vector = DirectX::XMLoadFloat3(&value);
    const float lengthSq = DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(vector));
    if (lengthSq <= 0.000001F)
    {
        return fallback;
    }

    return Store(DirectX::XMVector3Normalize(vector));
}
} // namespace Kimgane::Engine::VectorMath
