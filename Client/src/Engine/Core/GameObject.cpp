#include "Pch.h"

#include "GameObject.h"

#include <utility>

namespace Kimgane::Engine
{
GameObject::GameObject(std::string name)
    : name_(std::move(name))
{
}

void GameObject::Update(float deltaTimeSec)
{
    if (!active_)
    {
        return;
    }

    for (const auto& component : components_)
    {
        component->Update(deltaTimeSec);
    }
}

const std::string& GameObject::GetName() const noexcept
{
    return name_;
}

void GameObject::SetName(std::string name)
{
    name_ = std::move(name);
}

bool GameObject::IsActive() const noexcept
{
    return active_;
}

void GameObject::SetActive(bool active) noexcept
{
    active_ = active;
}

Transform& GameObject::GetTransform() noexcept
{
    return transform_;
}

const Transform& GameObject::GetTransform() const noexcept
{
    return transform_;
}
} // namespace Kimgane::Engine
