#pragma once

#include "../../Shared/Physics/CollisionTypes.h"

#include <DirectXMath.h>

namespace Kimgane::Client
{
// 클라(DirectXMath)와 Shared(Vec3) 사이 3D 벡터 변환. 값은 그대로 복사함.
[[nodiscard]] inline Kimgane::Shared::Physics::Vec3 ToSharedVec3(const DirectX::XMFLOAT3& value) noexcept
{
    return {value.x, value.y, value.z};
}

[[nodiscard]] inline DirectX::XMFLOAT3 ToXMFloat3(const Kimgane::Shared::Physics::Vec3& value) noexcept
{
    return {value.x, value.y, value.z};
}
} // namespace Kimgane::Client
