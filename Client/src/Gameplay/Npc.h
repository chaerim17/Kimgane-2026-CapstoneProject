#pragma once

#include "../Core/GameObject.h"

#include <DirectXMath.h>

#include <string>

namespace Kimgane::Client
{
// 서버 Npc에 대응하는 클라 NPC.
// 서버 위치는 띄엄띄엄 오므로, 받은 목표 위치를 향해 일정 속도로 따라가며 부드럽게 보여줌.
class Npc final : public GameObject
{
public:
    Npc(std::string name, const DirectX::XMFLOAT3& spawnPositionM, float visualMoveSpeedMps);

    void Update(float deltaTimeSec) override;

    // 서버가 보낸 새 위치. 다음 프레임부터 이 위치를 향해 이동함.
    void SetTargetPositionM(const DirectX::XMFLOAT3& positionM) noexcept;

private:
    DirectX::XMFLOAT3 mTargetPositionM = {};
    float mVisualMoveSpeedMps = 0.0F;
};
} // namespace Kimgane::Client
