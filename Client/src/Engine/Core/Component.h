#pragma once

namespace Kimgane::Engine
{
class GameObject;

class Component
{
public:
    explicit Component(GameObject& owner) noexcept;
    virtual ~Component() = default;

    Component(const Component&) = delete;
    Component& operator=(const Component&) = delete;
    Component(Component&&) = delete;
    Component& operator=(Component&&) = delete;

    virtual void Update(float deltaTimeSec);

    [[nodiscard]] GameObject& GetOwner() const noexcept;

private:
    GameObject& mOwner;
};

inline Component::Component(GameObject& owner) noexcept
    : mOwner(owner)
{
}

inline void Component::Update(float deltaTimeSec)
{
    (void)deltaTimeSec;
}

inline GameObject& Component::GetOwner() const noexcept
{
    return mOwner;
}
} // namespace Kimgane::Engine
