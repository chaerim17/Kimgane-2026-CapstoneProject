#include "Pch.h"

#include "NetworkSmoothingComponent.h"

#include "../Core/GameObject.h"

namespace Kimgane::Engine
{
namespace
{
constexpr float SNAP_DISTANCE_M = 0.001F;

DirectX::XMFLOAT3 Store(DirectX::FXMVECTOR value) noexcept
{
    DirectX::XMFLOAT3 result = {};
    DirectX::XMStoreFloat3(&result, value);
    return result;
}

DirectX::XMFLOAT3 MoveTowards(const DirectX::XMFLOAT3& currentM,
                              const DirectX::XMFLOAT3& targetM,
                              float maxDistanceM) noexcept
{
    const DirectX::XMVECTOR current = DirectX::XMLoadFloat3(&currentM);
    const DirectX::XMVECTOR target = DirectX::XMLoadFloat3(&targetM);
    const DirectX::XMVECTOR toTarget = DirectX::XMVectorSubtract(target, current);
    const float distanceM = DirectX::XMVectorGetX(DirectX::XMVector3Length(toTarget));

    if (distanceM <= SNAP_DISTANCE_M || distanceM <= maxDistanceM)
    {
        return targetM;
    }

    if (maxDistanceM <= 0.0F)
    {
        return currentM;
    }

    return Store(DirectX::XMVectorAdd(current, DirectX::XMVectorScale(toTarget, maxDistanceM / distanceM)));
}
} // namespace

NetworkSmoothingComponent::NetworkSmoothingComponent(GameObject& owner) noexcept
    : Component(owner), mTargetPositionM(owner.GetTransform().GetPositionM()), mHasTargetPosition(true)
{
}

void NetworkSmoothingComponent::Update(float deltaTimeSec)
{
    if (!mHasTargetPosition)
    {
        return;
    }

    const float maxStepM = mMoveSpeedMps * deltaTimeSec;
    GetOwner().GetTransform().SetPositionM(MoveTowards(GetOwner().GetTransform().GetPositionM(), mTargetPositionM, maxStepM));
}

void NetworkSmoothingComponent::SetTargetPositionM(const DirectX::XMFLOAT3& positionM) noexcept
{
    mTargetPositionM = positionM;
    mHasTargetPosition = true;
}

void NetworkSmoothingComponent::SnapToPositionM(const DirectX::XMFLOAT3& positionM) noexcept
{
    SetTargetPositionM(positionM);
    GetOwner().GetTransform().SetPositionM(positionM);
}

void NetworkSmoothingComponent::SetMoveSpeedMps(float moveSpeedMps) noexcept
{
    mMoveSpeedMps = moveSpeedMps > 0.0F ? moveSpeedMps : 0.0F;
}

const DirectX::XMFLOAT3& NetworkSmoothingComponent::GetTargetPositionM() const noexcept
{
    return mTargetPositionM;
}

float NetworkSmoothingComponent::GetMoveSpeedMps() const noexcept
{
    return mMoveSpeedMps;
}
} // namespace Kimgane::Engine
