#include "Pch.h"

#include "Player.h"

#include "../Scene/TestSceneSettings.h"

#include <utility>

namespace Kimgane::Client
{
Player::Player(std::string name, PlayerAppearance appearance)
    : GameObject(std::move(name)), mAppearance(std::move(appearance))
{
    SetMesh(mAppearance.normalMesh);
    GetMaterial().SetTextureGpuHandle(mAppearance.normalTextureGpuHandle);
}

void Player::ApplyMutationAppearance(float mutation)
{
    if (mAppearance.mutationMesh == nullptr)
    {
        return;
    }

    const bool isMutated = mutation >= TestSceneSettings::MUTATION_APPEARANCE_THRESHOLD;
    const auto& targetMesh = isMutated ? mAppearance.mutationMesh : mAppearance.normalMesh;
    if (GetMesh() == targetMesh)
    {
        return;
    }

    SetMesh(targetMesh);
    GetMaterial().SetTextureGpuHandle(isMutated ? mAppearance.mutationTextureGpuHandle
                                                : mAppearance.normalTextureGpuHandle);
}
} // namespace Kimgane::Client
