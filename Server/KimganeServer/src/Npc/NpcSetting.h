#pragma once

#include <vector>

class Npc;
class GameWorld;

namespace NpcSetting
{
    void Initialize(GameWorld& world);
    // 갱신한 NPC만 반환합니다. 패킷 전송은 Server가 담당합니다.
    std::vector<Npc*> Update(GameWorld& world, float deltaTimeSec);
} // namespace NpcSetting
