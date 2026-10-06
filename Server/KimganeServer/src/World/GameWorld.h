#pragma once

#include "GameObject.h"
#include "Player.h"
#include "ItemBox.h"
#include "../Npc/Npc.h"
#include "../Terrain/TerrainHeightMap.h"
#include "../../../../Shared/World/TestMapCollision.h"
#include "../../../../Shared/Geometry/ObjGeometry.h"

#include <map>
#include <memory>

// 모든 접근은 Server::mWorldMutex 아래에서 수행(데이터레이스 방지)
class GameWorld
{
public:
    void LoadMap();
    Player& CreatePlayer(int id);
    Npc& CreateNpc(int id);
    // 서버 내부 생성과 충돌 등록만 수행
    ItemBox& CreateItemBox(int id, const GameObject::Vec3& positionM);
    // 초기 배치용, 실제 생성 수 return
    int SpawnItemBoxes(int count);
    bool RemoveObject(int id);

    [[nodiscard]] GameObject* FindObject(int id) noexcept;
    [[nodiscard]] Player* FindPlayer(int id) noexcept;
    [[nodiscard]] const std::map<int, std::unique_ptr<GameObject>>& GetObjects() const noexcept { return mObjects; }
    [[nodiscard]] TerrainHeightMap& GetTerrain() const noexcept { return *mTerrain; }
    [[nodiscard]] Kimgane::Shared::World::TestMapCollision& GetMapCollision() noexcept { return mMapCollision; }
    // OBJ 로더의 로컬 좌표(미터) 데이터 월드 배치는 TestMapSettings::HOUSE_POSITION_M
    [[nodiscard]] const Kimgane::Shared::Geometry::ObjGeometryData& GetHouseGeometry() const noexcept { return mHouseGeometry; }
    [[nodiscard]] const Kimgane::Shared::Geometry::ObjGeometryData& GetItemBoxBodyGeometry() const noexcept { return mItemBoxBodyGeometry; }
    [[nodiscard]] const Kimgane::Shared::Geometry::ObjGeometryData& GetItemBoxLidGeometry() const noexcept { return mItemBoxLidGeometry; }

private:
    GameObject& AddObject(std::unique_ptr<GameObject> object);
    std::map<int, std::unique_ptr<GameObject>> mObjects;
    std::shared_ptr<TerrainHeightMap> mTerrain;
    Kimgane::Shared::World::TestMapCollision mMapCollision;
    Kimgane::Shared::Geometry::ObjGeometryData mHouseGeometry;
    Kimgane::Shared::Geometry::ObjGeometryData mItemBoxBodyGeometry;
    Kimgane::Shared::Geometry::ObjGeometryData mItemBoxLidGeometry;
    Kimgane::Shared::Physics::Box mItemBoxLocalCollision;
};
