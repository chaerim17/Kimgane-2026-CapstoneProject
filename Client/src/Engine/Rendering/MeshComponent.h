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
        : Component(owner), mesh_(std::move(mesh))
    {
    }

    [[nodiscard]] const std::shared_ptr<Mesh>& GetMesh() const noexcept
    {
        return mesh_;
    }

    void SetMesh(std::shared_ptr<Mesh> mesh) noexcept
    {
        mesh_ = std::move(mesh);
    }

    void Render(ID3D12GraphicsCommandList& commandList) const noexcept
    {
        if (mesh_)
        {
            mesh_->Render(commandList);
        }
    }

private:
    std::shared_ptr<Mesh> mesh_;
};
} // namespace Kimgane::Engine
