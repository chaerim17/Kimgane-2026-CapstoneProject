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
    GameObject& owner_;
};

inline Component::Component(GameObject& owner) noexcept
    : owner_(owner)
{
}

inline void Component::Update(float deltaTimeSec)
{
    (void)deltaTimeSec;
}

inline GameObject& Component::GetOwner() const noexcept
{
    return owner_;
}
} // namespace Kimgane::Engine
