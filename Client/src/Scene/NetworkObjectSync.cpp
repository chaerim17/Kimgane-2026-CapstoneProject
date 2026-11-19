#include "Pch.h"

#include "NetworkObjectSync.h"

#include "Scene.h"
#include "TestSceneSettings.h"
#include "../Core/SharedVec3Conversion.h"
#include "../Network/NetworkManager.h"

#include <iostream>
#include <string>
#include <utility>

namespace Kimgane::Client
{
namespace
{
namespace SharedPhysics = Kimgane::Shared::Physics;
using Kimgane::Shared::World::ObjectType;
} // namespace

void NetworkObjectSync::Reset(Scene& scene,
                              NetworkManager& networkManager,
                              SharedPhysics::CollisionWorld& collisionWorld,
                              NetworkObjectAssets assets)
{
    mScene = &scene;
    mNetworkManager = &networkManager;
    mCollisionWorld = &collisionWorld;
    mAssets = std::move(assets);
    mRemotePlayers.clear();
    mNpcs.clear();
    mItemBoxes.clear();
}

void NetworkObjectSync::ApplyServerTransform(int objectId, const DirectX::XMFLOAT3& positionM, float yawRad)
{
    if (const auto npc = mNpcs.find(objectId); npc != mNpcs.end())
    {
        npc->second->SetTargetPositionM(positionM);
        npc->second->SetYawRad(yawRad);
        return;
    }

    if (const auto remotePlayer = mRemotePlayers.find(objectId); remotePlayer != mRemotePlayers.end())
    {
        remotePlayer->second->GetTransform().SetPositionM(positionM);
        remotePlayer->second->SetYawRad(yawRad);
        return;
    }

    CreateNetworkObject(objectId, positionM, yawRad);
}

void NetworkObjectSync::Remove(int objectId)
{
    std::cout << "[SCENE REMOVE] " << objectId << '\n';
    if (const auto npc = mNpcs.find(objectId); npc != mNpcs.end())
    {
        npc->second->SetActive(false);
        mNpcs.erase(npc);
    }
    else if (const auto remotePlayer = mRemotePlayers.find(objectId); remotePlayer != mRemotePlayers.end())
    {
        remotePlayer->second->SetActive(false);
        mRemotePlayers.erase(remotePlayer);
    }
    else
    {
        return;
    }

    std::cout << "[Scene] Network Player " << objectId << " Removed\n";
}

void NetworkObjectSync::Update()
{
    if (mNetworkManager == nullptr)
    {
        return;
    }

    RefreshHealthBars();
    RefreshRemoteMutationAppearances();
    SyncItemBoxes();
}

bool NetworkObjectSync::IsNpc(int objectId) const noexcept
{
    const ObjectState* state = mNetworkManager != nullptr ? mNetworkManager->FindObject(objectId) : nullptr;
    return state != nullptr && state->mType == ObjectType::Npc;
}

const Npc* NetworkObjectSync::FindNpc(int objectId) const noexcept
{
    const auto npc = mNpcs.find(objectId);
    return npc != mNpcs.end() ? npc->second : nullptr;
}

const std::unordered_map<int, Npc*>& NetworkObjectSync::GetNpcs() const noexcept
{
    return mNpcs;
}

const std::unordered_map<int, ItemBox*>& NetworkObjectSync::GetItemBoxes() const noexcept
{
    return mItemBoxes;
}

void NetworkObjectSync::CreateNetworkObject(int objectId, const DirectX::XMFLOAT3& positionM, float yawRad)
{
    if (mScene == nullptr)
    {
        return;
    }

    if (IsNpc(objectId))
    {
        Npc& npc = mScene->CreateObject<Npc>("NPC " + std::to_string(objectId), positionM,
                                             TestSceneSettings::NPC_VISUAL_MOVE_SPEED_MPS);
        npc.SetYawRad(yawRad);
        npc.SetMesh(mAssets.npcMesh);
        Material& material = npc.GetMaterial();
        material.SetBaseColorLinear(TestSceneSettings::NPC_MODEL_BASE_COLOR_LINEAR);
        material.SetSurface(0.0F, 0.48F);
        material.SetTextureGpuHandle(mAssets.npcTextureGpuHandle);
        // TODO: NPC 머리/몸통 히트박스(Shared Npc::HUMANOID_HIT_BOXES) 정렬 확인이 필요하면
        // ColliderDebugDrawSystem에 NPC 히트박스 표시 기능을 추가. (예전 콜라이더 컴포넌트 방식은 제거됨)
        mNpcs[objectId] = &npc;
        return;
    }

    RemotePlayer& remotePlayer =
        mScene->CreateObject<RemotePlayer>("Network Player " + std::to_string(objectId), mAssets.playerAppearance);
    remotePlayer.GetTransform().SetPositionM(positionM);
    remotePlayer.SetYawRad(yawRad);
    Material& material = remotePlayer.GetMaterial();
    material.SetBaseColorLinear(TestSceneSettings::NETWORK_PLAYER_BASE_COLOR_LINEAR);
    material.SetSurface(0.0F, 0.48F);
    mRemotePlayers[objectId] = &remotePlayer;
}

void NetworkObjectSync::CreateItemBox(int itemBoxId, const DirectX::XMFLOAT3& positionM)
{
    if (mScene == nullptr)
    {
        return;
    }

    ItemBox& itemBox = mScene->CreateObject<ItemBox>("Item Box " + std::to_string(itemBoxId) + " Body");
    itemBox.GetTransform().SetPositionM(positionM);
    itemBox.SetMesh(mAssets.itemBoxBodyMesh);
    itemBox.GetMaterial().SetBaseColorLinear(TestSceneSettings::ITEM_BOX_BASE_COLOR_LINEAR);
    itemBox.GetMaterial().SetSurface(0.1F, 0.6F);
    itemBox.GetMaterial().SetTextureGpuHandle(mAssets.itemBoxTextureGpuHandle);

    // House/Terrain과 같은 공유 물리 월드에 등록. 이동 충돌·레이캐스트·디버그 표시가 모두 이 body를 씀.
    // 서버 GameWorld::CreateItemBox도 같은 id/박스로 이 월드에 등록하므로 그대로 맞춰줌.
    // S07_Supply_Crate_collision.txt 박스는 본체 기준 로컬 좌표이고, 본체+닫힌 뚜껑을 감싸는 박스 하나뿐임.
    if (mCollisionWorld != nullptr && !mAssets.itemBoxCollisionBoxes.empty())
    {
        const auto& localBox = mAssets.itemBoxCollisionBoxes.front().box;
        const SharedPhysics::Vec3 centerWorldM = SharedPhysics::Add(localBox.centerM, ToSharedVec3(positionM));
        mCollisionWorld->AddOrUpdateBody({itemBoxId, SharedPhysics::Box{centerWorldM, localBox.halfExtentsM},
                                          SharedPhysics::CollisionLayer::STATIC_WORLD,
                                          SharedPhysics::CollisionLayer::ALL, false});
    }

    GameObject& lid = mScene->CreateObject("Item Box " + std::to_string(itemBoxId) + " Lid");
    lid.SetMesh(mAssets.itemBoxLidMesh);
    lid.GetMaterial().SetBaseColorLinear(TestSceneSettings::ITEM_BOX_BASE_COLOR_LINEAR);
    lid.GetMaterial().SetSurface(0.1F, 0.6F);
    lid.GetMaterial().SetTextureGpuHandle(mAssets.itemBoxTextureGpuHandle);
    itemBox.AttachLid(lid, TestSceneSettings::ITEM_BOX_LID_LOCAL_OFFSET_M);

    mItemBoxes[itemBoxId] = &itemBox;
}

void NetworkObjectSync::RefreshHealthBars()
{
    for (auto& [npcId, npc] : mNpcs)
    {
        if (npc != nullptr)
        {
            npc->GetHealthBar().SetHp(mNetworkManager->GetCurrentHp(npcId), mNetworkManager->GetMaxHp(npcId));
        }
    }
}

void NetworkObjectSync::RefreshRemoteMutationAppearances()
{
    for (auto& [playerId, remotePlayer] : mRemotePlayers)
    {
        if (remotePlayer == nullptr)
        {
            continue;
        }

        if (const auto* objectState = mNetworkManager->FindObject(playerId))
        {
            remotePlayer->ApplyMutationAppearance(objectState->mMutation);
        }
    }
}

void NetworkObjectSync::SyncItemBoxes()
{
    for (const auto& [objectId, state] : mNetworkManager->GetObjects())
    {
        if (state.mType != ObjectType::ItemBox || !state.mIsActive)
        {
            continue;
        }

        if (mItemBoxes.find(objectId) == mItemBoxes.end())
        {
            CreateItemBox(objectId, {state.mX, state.mY, state.mZ});
        }
    }

    for (auto iter = mItemBoxes.begin(); iter != mItemBoxes.end();)
    {
        const auto* state = mNetworkManager->FindObject(iter->first);
        const bool stillPresent = state != nullptr && state->mIsActive && state->mType == ObjectType::ItemBox;
        if (stillPresent)
        {
            ++iter;
            continue;
        }

        if (iter->second != nullptr)
        {
            iter->second->Despawn();
        }
        if (mCollisionWorld != nullptr)
        {
            mCollisionWorld->RemoveBody(iter->first);
        }
        iter = mItemBoxes.erase(iter);
    }
}
} // namespace Kimgane::Client
