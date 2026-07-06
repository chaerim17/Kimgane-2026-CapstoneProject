#pragma once

#include "Component.h"
#include "Transform.h"

#include <algorithm>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace Kimgane::Engine
{
class GameObject
{
public:
    explicit GameObject(std::string name = {});
    virtual ~GameObject() = default;

    GameObject(const GameObject&) = delete;
    GameObject& operator=(const GameObject&) = delete;
    GameObject(GameObject&&) = delete;
    GameObject& operator=(GameObject&&) = delete;

    virtual void Update(float deltaTimeSec);

    [[nodiscard]] const std::string& GetName() const noexcept;
    void SetName(std::string name);

    [[nodiscard]] bool IsActive() const noexcept;
    void SetActive(bool active) noexcept;

    [[nodiscard]] Transform& GetTransform() noexcept;
    [[nodiscard]] const Transform& GetTransform() const noexcept;

    template <typename T, typename... Args>
    T& AddComponent(Args&&... args)
    {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component.");

        auto component = std::make_unique<T>(*this, std::forward<Args>(args)...);
        T& reference = *component;
        components_.push_back(std::move(component));
        return reference;
    }

    template <typename T>
    [[nodiscard]] T* GetComponent() noexcept
    {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component.");

        for (const auto& component : components_)
        {
            if (auto* typedComponent = dynamic_cast<T*>(component.get()))
            {
                return typedComponent;
            }
        }

        return nullptr;
    }

    template <typename T>
    [[nodiscard]] const T* GetComponent() const noexcept
    {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component.");

        for (const auto& component : components_)
        {
            if (const auto* typedComponent = dynamic_cast<const T*>(component.get()))
            {
                return typedComponent;
            }
        }

        return nullptr;
    }

    template <typename T>
    bool RemoveComponents()
    {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component.");

        const auto oldSize = components_.size();
        components_.erase(std::remove_if(components_.begin(),
                                         components_.end(),
                                         [](const std::unique_ptr<Component>& component)
                                         {
                                             return dynamic_cast<T*>(component.get()) != nullptr;
                                         }),
                          components_.end());
        return components_.size() != oldSize;
    }

private:
    std::string name_;
    bool active_ = true;
    Transform transform_;
    std::vector<std::unique_ptr<Component>> components_;
};
} // namespace Kimgane::Engine
