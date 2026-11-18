#pragma once

#include "../Core/GameObject.h"

#include <memory>
#include <string>

namespace Kimgane::Client
{
// 플레이어 외형(일반 / 중간 변이) 묶음. 변이 메쉬가 없으면 외형 전환을 하지 않음.
struct PlayerAppearance
{
    std::shared_ptr<Mesh> normalMesh;
    D3D12_GPU_DESCRIPTOR_HANDLE normalTextureGpuHandle = {};
    std::shared_ptr<Mesh> mutationMesh;
    D3D12_GPU_DESCRIPTOR_HANDLE mutationTextureGpuHandle = {};
};

// 서버 Player에 대응하는 클라 플레이어. 로컬/원격 공통으로 외형과 바라보는 방향을 다룸.
class Player : public GameObject
{
public:
    Player(std::string name, PlayerAppearance appearance);

    // 서버 변이 수치가 기준값 이상이면 중간 변이 외형, 정화로 내려가면 원래 외형으로 되돌림.
    void ApplyMutationAppearance(float mutation);

private:
    PlayerAppearance mAppearance;
};

// 다른 플레이어. 서버가 보낸 위치·방향을 그대로 반영함.
class RemotePlayer final : public Player
{
public:
    using Player::Player;
};
} // namespace Kimgane::Client
