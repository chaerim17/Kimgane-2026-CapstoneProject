#include "Pch.h"

#include "Npc.h"

#include <algorithm>
#include <utility>

namespace Kimgane::Client
{
namespace
{
constexpr float SNAP_DISTANCE_M = 0.001F;

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

    DirectX::XMFLOAT3 result = {};
    DirectX::XMStoreFloat3(&result, DirectX::XMVectorAdd(current, DirectX::XMVectorScale(toTarget, maxDistanceM / distanceM)));
    return result;
}
} // namespace

Npc::Npc(std::string name, const DirectX::XMFLOAT3& spawnPositionM, float visualMoveSpeedMps)
    : GameObject(std::move(name)), mTargetPositionM(spawnPositionM), mVisualMoveSpeedMps(std::max(visualMoveSpeedMps, 0.0F))
{
    GetTransform().SetPositionM(spawnPositionM);
}

void Npc::Update(float deltaTimeSec)
{
    if (!IsActive())
    {
        return;
    }

    GetTransform().SetPositionM(
        MoveTowards(GetTransform().GetPositionM(), mTargetPositionM, mVisualMoveSpeedMps * deltaTimeSec));
    GameObject::Update(deltaTimeSec);
}

void Npc::SetTargetPositionM(const DirectX::XMFLOAT3& positionM) noexcept
{
    mTargetPositionM = positionM;
}
} // namespace Kimgane::Client
