#pragma once

#include <vector>

struct ID3D12Device;

namespace Kimgane::Engine
{
class ColliderComponent;
class GameObject;
class Scene;

class ColliderDebugDrawSystem final
{
public:
    void Clear() noexcept;
    void RegisterCollider(ID3D12Device& device, Scene& scene, ColliderComponent& collider);
    void SetVisible(bool visible) noexcept;
    void ToggleVisible() noexcept;
    [[nodiscard]] bool IsVisible() const noexcept;
    void Sync() noexcept;

private:
    struct DebugCollider
    {
        ColliderComponent* collider = nullptr;
        GameObject* debugObject = nullptr;
    };

    std::vector<DebugCollider> mDebugColliders;
    bool mVisible = false;
};
} // namespace Kimgane::Engine
