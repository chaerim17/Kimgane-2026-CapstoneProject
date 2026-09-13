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
        mComponents.push_back(std::move(component));
        return reference;
    }

    template <typename T>
    [[nodiscard]] T* GetComponent() noexcept
    {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component.");

        for (const auto& component : mComponents)
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

        for (const auto& component : mComponents)
        {
            if (const auto* typedComponent = dynamic_cast<const T*>(component.get()))
            {
                return typedComponent;
            }
        }

        return nullptr;
    }

    // GetComponent()는 가장 먼저 찾은 것 하나만 반환해서, TestHouse처럼 한 GameObject에
    // 같은 타입 컴포넌트가 여러 개(박스 콜라이더 30개) 붙어있는 경우엔 나머지를 놓침.
    // 그럴 때는 이 함수로 같은 타입을 전부 받아옴.
    template <typename T>
    [[nodiscard]] std::vector<T*> GetComponents() noexcept
    {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component.");

        std::vector<T*> result;
        for (const auto& component : mComponents)
        {
            if (auto* typedComponent = dynamic_cast<T*>(component.get()))
            {
                result.push_back(typedComponent);
            }
        }
        return result;
    }

    template <typename T>
    [[nodiscard]] std::vector<const T*> GetComponents() const noexcept
    {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component.");

        std::vector<const T*> result;
        for (const auto& component : mComponents)
        {
            if (const auto* typedComponent = dynamic_cast<const T*>(component.get()))
            {
                result.push_back(typedComponent);
            }
        }
        return result;
    }

    template <typename T>
    bool RemoveComponents()
    {
        static_assert(std::is_base_of_v<Component, T>, "T must derive from Component.");

        const auto oldSize = mComponents.size();
        mComponents.erase(std::remove_if(mComponents.begin(),
                                         mComponents.end(),
                                         [](const std::unique_ptr<Component>& component)
                                         {
                                             return dynamic_cast<T*>(component.get()) != nullptr;
                                         }),
                          mComponents.end());
        return mComponents.size() != oldSize;
    }

private:
    std::string mName;
    bool mActive = true;
    Transform mTransform;
    std::vector<std::unique_ptr<Component>> mComponents;
};
} // namespace Kimgane::Engine
