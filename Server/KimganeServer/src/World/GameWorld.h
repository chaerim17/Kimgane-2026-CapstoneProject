#pragma once

#include "GameObject.h"
#include "Player.h"
#include "../Npc/Npc.h"
#include "../Terrain/TerrainHeightMap.h"
#include "../../../../Shared/World/TestMapCollision.h"

#include <map>
#include <memory>

// 모든 접근은 Server::mWorldMutex 아래에서 수행(데이터레이스 방지)
class GameWorld
{
public:
    void LoadMap();
    Player& CreatePlayer(int id);
    Npc& CreateNpc(int id);
    bool RemoveObject(int id);

    [[nodiscard]] GameObject* FindObject(int id) noexcept;
    [[nodiscard]] Player* FindPlayer(int id) noexcept;
    [[nodiscard]] const std::map<int, std::unique_ptr<GameObject>>& GetObjects() const noexcept { return mObjects; }
    [[nodiscard]] TerrainHeightMap& GetTerrain() const noexcept { return *mTerrain; }
    [[nodiscard]] Kimgane::Shared::World::TestMapCollision& GetMapCollision() noexcept { return mMapCollision; }

private:
    GameObject& AddObject(std::unique_ptr<GameObject> object);
    std::map<int, std::unique_ptr<GameObject>> mObjects;
    std::shared_ptr<TerrainHeightMap> mTerrain;
    Kimgane::Shared::World::TestMapCollision mMapCollision;
};
