#pragma once

#include "../Core/Component.h"
#include "Mesh.h"

#include <memory>
#include <utility>

namespace Kimgane::Engine
{
class MeshComponent final : public Component
{
public:
    explicit MeshComponent(GameObject& owner, std::shared_ptr<Mesh> mesh) noexcept
        : Component(owner), mMesh(std::move(mesh))
    {
    }

    [[nodiscard]] const std::shared_ptr<Mesh>& GetMesh() const noexcept
    {
        return mMesh;
    }

    void SetMesh(std::shared_ptr<Mesh> mesh) noexcept
    {
        mMesh = std::move(mesh);
    }

    void Render(ID3D12GraphicsCommandList& commandList) const noexcept
    {
        if (mMesh)
        {
            mMesh->Render(commandList);
        }
    }

private:
    std::shared_ptr<Mesh> mMesh;
};
} // namespace Kimgane::Engine
