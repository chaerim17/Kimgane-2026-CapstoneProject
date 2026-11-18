#pragma once

#include "Transform.h"
#include "../Rendering/Material.h"

#include <memory>
#include <string>
#include <type_traits>

namespace Kimgane::Client
{
class Mesh;

// 씬에 놓이는 물체의 기본 클래스. 위치(Transform)와 그릴 모양(Mesh)·재질(Material)을 직접 가짐.
// Mesh가 없으면 그리지 않음. 동작이 필요한 물체(플레이어, NPC 등)는 이 클래스를 상속해서 Update를 재정의함.
class GameObject
{
public:
    explicit GameObject(std::string name = {});
    virtual ~GameObject() = default;

    GameObject(const GameObject&) = delete;
    GameObject& operator=(const GameObject&) = delete;
    GameObject(GameObject&&) = delete;
    GameObject& operator=(GameObject&&) = delete;

    // 매 프레임 호출. 기본은 아무것도 안 함. 동작이 있는 물체가 재정의함.
    virtual void Update(float deltaTimeSec);

    [[nodiscard]] const std::string& GetName() const noexcept;
    void SetName(std::string name);

    [[nodiscard]] bool IsActive() const noexcept;
    void SetActive(bool active) noexcept;

    [[nodiscard]] Transform& GetTransform() noexcept;
    [[nodiscard]] const Transform& GetTransform() const noexcept;
    // 위에서 내려다본 회전(Y축). 서버 yaw와 같은 기준.
    [[nodiscard]] float GetYawRad() const noexcept;
    void SetYawRad(float yawRad) noexcept;

    [[nodiscard]] const std::shared_ptr<Mesh>& GetMesh() const noexcept;
    void SetMesh(std::shared_ptr<Mesh> mesh) noexcept;
    [[nodiscard]] Material& GetMaterial() noexcept;
    [[nodiscard]] const Material& GetMaterial() const noexcept;

private:
    std::string mName;
    bool mActive = true;
    Transform mTransform;
    std::shared_ptr<Mesh> mMesh;
    Material mMaterial;
};
} // namespace Kimgane::Client
