#pragma once

#include <vector>
#include "../../../../Shared/Protocol.h"

class Npc;
class GameWorld;

namespace NpcSetting
{
    constexpr int COUNT = NPC_COUNT;
    constexpr int MOVE_INTERVAL_MS = 1000; // 이동 방향 선택 간격. 위치는 매 서버 틱 갱신.

    void Initialize(GameWorld& world);
    // 갱신한 NPC만 반환합니다. 패킷 전송은 Server가 담당합니다.
    std::vector<Npc*> Update(GameWorld& world, float deltaTimeSec);
} // namespace NpcSetting
