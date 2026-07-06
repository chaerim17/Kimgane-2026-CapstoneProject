#include "Pch.h"

#include "GameObject.h"

#include <utility>

namespace Kimgane::Engine
{
GameObject::GameObject(std::string name)
    : mName(std::move(name))
{
}

void GameObject::Update(float deltaTimeSec)
{
    if (!mActive)
    {
        return;
    }

    for (const auto& component : mComponents)
    {
        component->Update(deltaTimeSec);
    }
}

const std::string& GameObject::GetName() const noexcept
{
    return mName;
}

void GameObject::SetName(std::string name)
{
    mName = std::move(name);
}

bool GameObject::IsActive() const noexcept
{
    return mActive;
}

void GameObject::SetActive(bool active) noexcept
{
    mActive = active;
}

Transform& GameObject::GetTransform() noexcept
{
    return mTransform;
}

const Transform& GameObject::GetTransform() const noexcept
{
    return mTransform;
}
} // namespace Kimgane::Engine
