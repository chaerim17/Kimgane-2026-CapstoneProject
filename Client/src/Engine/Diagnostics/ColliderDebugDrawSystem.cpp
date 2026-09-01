#include "Pch.h"

#include "ColliderDebugDrawSystem.h"

#include "../Core/GameObject.h"
#include "../Physics/ColliderComponent.h"
#include "../Physics/TerrainColliderComponent.h"
#include "../Rendering/MaterialComponent.h"
#include "../Rendering/Mesh.h"
#include "../Rendering/MeshComponent.h"
#include "../Scene/Scene.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Kimgane::Engine
{
namespace
{
constexpr int CIRCLE_SEGMENTS = 32;
constexpr int ARC_SEGMENTS = 16;
constexpr float DEBUG_EMISSION_INTENSITY = 0.25F;
constexpr DirectX::XMFLOAT3 DEBUG_NORMAL = {0.0F, 1.0F, 0.0F};
constexpr DirectX::XMFLOAT4 DEBUG_VERTEX_COLOR = {1.0F, 1.0F, 1.0F, 1.0F};

DirectX::XMFLOAT3 StoreVector3(DirectX::FXMVECTOR value) noexcept
{
    DirectX::XMFLOAT3 result = {};
    DirectX::XMStoreFloat3(&result, value);
    return result;
}

Mesh::Vertex MakeLineVertex(const DirectX::XMFLOAT3& positionM) noexcept
{
    return {positionM, DEBUG_NORMAL, DEBUG_VERTEX_COLOR};
}

void AddLine(std::vector<Mesh::Vertex>& vertices,
             const DirectX::XMFLOAT3& fromM,
             const DirectX::XMFLOAT3& toM)
{
    vertices.push_back(MakeLineVertex(fromM));
    vertices.push_back(MakeLineVertex(toM));
}

void AddBoxLines(std::vector<Mesh::Vertex>& vertices, const DirectX::XMFLOAT3 (&cornersM)[8])
{
    constexpr std::array<int, 24> EDGE_INDICES = {
        0, 1, 1, 2, 2, 3, 3, 0,
        4, 5, 5, 6, 6, 7, 7, 4,
        0, 4, 1, 5, 2, 6, 3, 7};

    for (std::size_t index = 0; index + 1U < EDGE_INDICES.size(); index += 2U)
    {
        AddLine(vertices, cornersM[EDGE_INDICES[index]], cornersM[EDGE_INDICES[index + 1U]]);
    }
}

void AddObbLines(std::vector<Mesh::Vertex>& vertices, const DirectX::BoundingOrientedBox& box)
{
    DirectX::XMFLOAT3 cornersM[8] = {};
    box.GetCorners(cornersM);
    AddBoxLines(vertices, cornersM);
}

void AddCircleLines(std::vector<Mesh::Vertex>& vertices,
                    DirectX::FXMVECTOR centerM,
                    DirectX::FXMVECTOR right,
                    DirectX::FXMVECTOR forward,
                    float radiusM)
{
    DirectX::XMVECTOR previous = {};
    for (int segment = 0; segment <= CIRCLE_SEGMENTS; ++segment)
    {
        const float angleRad = DirectX::XM_2PI * static_cast<float>(segment) / static_cast<float>(CIRCLE_SEGMENTS);
        const DirectX::XMVECTOR offset = DirectX::XMVectorAdd(DirectX::XMVectorScale(right, std::cos(angleRad) * radiusM),
                                                             DirectX::XMVectorScale(forward, std::sin(angleRad) * radiusM));
        const DirectX::XMVECTOR current = DirectX::XMVectorAdd(centerM, offset);
        if (segment > 0)
        {
            AddLine(vertices, StoreVector3(previous), StoreVector3(current));
        }
        previous = current;
    }
}

void AddHemisphereArc(std::vector<Mesh::Vertex>& vertices,
                      DirectX::FXMVECTOR centerM,
                      DirectX::FXMVECTOR axis,
                      DirectX::FXMVECTOR radial,
                      float radiusM,
                      bool upper)
{
    DirectX::XMVECTOR previous = {};
    for (int segment = 0; segment <= ARC_SEGMENTS; ++segment)
    {
        const float angleRad = DirectX::XM_PI * static_cast<float>(segment) / static_cast<float>(ARC_SEGMENTS);
        const float axisSign = upper ? 1.0F : -1.0F;
        const DirectX::XMVECTOR offset = DirectX::XMVectorAdd(
            DirectX::XMVectorScale(radial, std::cos(angleRad) * radiusM),
            DirectX::XMVectorScale(axis, std::sin(angleRad) * radiusM * axisSign));
        const DirectX::XMVECTOR current = DirectX::XMVectorAdd(centerM, offset);
        if (segment > 0)
        {
            AddLine(vertices, StoreVector3(previous), StoreVector3(current));
        }
        previous = current;
    }
}

void AddCapsuleLines(std::vector<Mesh::Vertex>& vertices,
                     const DirectX::XMFLOAT3& centerM,
                     float radiusM,
                     float heightM)
{
    const float halfSegmentM = std::max((heightM * 0.5F) - radiusM, 0.0F);
    const DirectX::XMVECTOR center = DirectX::XMLoadFloat3(&centerM);
    const DirectX::XMVECTOR axis = DirectX::XMVectorSet(0.0F, 1.0F, 0.0F, 0.0F);
    const DirectX::XMVECTOR right = DirectX::XMVectorSet(1.0F, 0.0F, 0.0F, 0.0F);
    const DirectX::XMVECTOR forward = DirectX::XMVectorSet(0.0F, 0.0F, 1.0F, 0.0F);
    const DirectX::XMVECTOR bottom = DirectX::XMVectorAdd(center, DirectX::XMVectorScale(axis, -halfSegmentM));
    const DirectX::XMVECTOR top = DirectX::XMVectorAdd(center, DirectX::XMVectorScale(axis, halfSegmentM));

    AddCircleLines(vertices, bottom, right, forward, radiusM);
    AddCircleLines(vertices, top, right, forward, radiusM);

    const std::array<DirectX::XMVECTOR, 4> radialDirections = {
        right,
        DirectX::XMVectorScale(right, -1.0F),
        forward,
        DirectX::XMVectorScale(forward, -1.0F)};

    for (const DirectX::XMVECTOR& radial : radialDirections)
    {
        AddLine(vertices,
                StoreVector3(DirectX::XMVectorAdd(bottom, DirectX::XMVectorScale(radial, radiusM))),
                StoreVector3(DirectX::XMVectorAdd(top, DirectX::XMVectorScale(radial, radiusM))));
    }

    AddHemisphereArc(vertices, top, axis, right, radiusM, true);
    AddHemisphereArc(vertices, top, axis, forward, radiusM, true);
    AddHemisphereArc(vertices, bottom, axis, right, radiusM, false);
    AddHemisphereArc(vertices, bottom, axis, forward, radiusM, false);
}

void AddSphereLines(std::vector<Mesh::Vertex>& vertices,
                    const DirectX::XMFLOAT3& centerM,
                    float radiusM)
{
    const DirectX::XMVECTOR center = DirectX::XMLoadFloat3(&centerM);
    const DirectX::XMVECTOR right = DirectX::XMVectorSet(1.0F, 0.0F, 0.0F, 0.0F);
    const DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0F, 1.0F, 0.0F, 0.0F);
    const DirectX::XMVECTOR forward = DirectX::XMVectorSet(0.0F, 0.0F, 1.0F, 0.0F);

    AddCircleLines(vertices, center, right, forward, radiusM);
    AddCircleLines(vertices, center, right, up, radiusM);
    AddCircleLines(vertices, center, forward, up, radiusM);
}

void AddCylinderLines(std::vector<Mesh::Vertex>& vertices,
                      const DirectX::XMFLOAT3& centerM,
                      float radiusM,
                      float heightM)
{
    const DirectX::XMVECTOR center = DirectX::XMLoadFloat3(&centerM);
    const DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0F, 1.0F, 0.0F, 0.0F);
    const DirectX::XMVECTOR right = DirectX::XMVectorSet(1.0F, 0.0F, 0.0F, 0.0F);
    const DirectX::XMVECTOR forward = DirectX::XMVectorSet(0.0F, 0.0F, 1.0F, 0.0F);
    const DirectX::XMVECTOR bottom = DirectX::XMVectorAdd(center, DirectX::XMVectorScale(up, heightM * -0.5F));
    const DirectX::XMVECTOR top = DirectX::XMVectorAdd(center, DirectX::XMVectorScale(up, heightM * 0.5F));

    AddCircleLines(vertices, bottom, right, forward, radiusM);
    AddCircleLines(vertices, top, right, forward, radiusM);

    const std::array<DirectX::XMVECTOR, 4> radialDirections = {
        right,
        DirectX::XMVectorScale(right, -1.0F),
        forward,
        DirectX::XMVectorScale(forward, -1.0F)};

    for (const DirectX::XMVECTOR& radial : radialDirections)
    {
        AddLine(vertices,
                StoreVector3(DirectX::XMVectorAdd(bottom, DirectX::XMVectorScale(radial, radiusM))),
                StoreVector3(DirectX::XMVectorAdd(top, DirectX::XMVectorScale(radial, radiusM))));
    }
}

float ComputeRampSurfaceY(const DirectX::BoundingOrientedBox& bounds,
                          RampColliderComponent::RampDirection direction,
                          float x,
                          float z) noexcept
{
    const float minX = bounds.Center.x - bounds.Extents.x;
    const float maxX = bounds.Center.x + bounds.Extents.x;
    const float minZ = bounds.Center.z - bounds.Extents.z;
    const float maxZ = bounds.Center.z + bounds.Extents.z;
    const float bottomY = bounds.Center.y - bounds.Extents.y;
    const float riseM = bounds.Extents.y * 2.0F;

    float progress = 0.0F;
    switch (direction)
    {
    case RampColliderComponent::RampDirection::PositiveX:
        progress = (x - minX) / std::max(maxX - minX, 0.001F);
        break;
    case RampColliderComponent::RampDirection::NegativeX:
        progress = (maxX - x) / std::max(maxX - minX, 0.001F);
        break;
    case RampColliderComponent::RampDirection::NegativeZ:
        progress = (maxZ - z) / std::max(maxZ - minZ, 0.001F);
        break;
    case RampColliderComponent::RampDirection::PositiveZ:
    default:
        progress = (z - minZ) / std::max(maxZ - minZ, 0.001F);
        break;
    }

    return bottomY + riseM * std::clamp(progress, 0.0F, 1.0F);
}

void AddRampLines(std::vector<Mesh::Vertex>& vertices,
                  const DirectX::BoundingOrientedBox& bounds,
                  RampColliderComponent::RampDirection direction)
{
    const float minX = bounds.Center.x - bounds.Extents.x;
    const float maxX = bounds.Center.x + bounds.Extents.x;
    const float minZ = bounds.Center.z - bounds.Extents.z;
    const float maxZ = bounds.Center.z + bounds.Extents.z;
    const float bottomY = bounds.Center.y - bounds.Extents.y;

    const std::array<DirectX::XMFLOAT3, 4> bottomCorners = {
        DirectX::XMFLOAT3{minX, bottomY, minZ},
        DirectX::XMFLOAT3{maxX, bottomY, minZ},
        DirectX::XMFLOAT3{maxX, bottomY, maxZ},
        DirectX::XMFLOAT3{minX, bottomY, maxZ}};
    std::array<DirectX::XMFLOAT3, 4> surfaceCorners = bottomCorners;
    for (DirectX::XMFLOAT3& corner : surfaceCorners)
    {
        corner.y = ComputeRampSurfaceY(bounds, direction, corner.x, corner.z);
    }

    for (std::size_t index = 0; index < bottomCorners.size(); ++index)
    {
        const std::size_t next = (index + 1U) % bottomCorners.size();
        AddLine(vertices, bottomCorners[index], bottomCorners[next]);
        AddLine(vertices, surfaceCorners[index], surfaceCorners[next]);
        AddLine(vertices, bottomCorners[index], surfaceCorners[index]);
    }

    AddLine(vertices, surfaceCorners[0], surfaceCorners[2]);
}

DirectX::XMFLOAT3 GetTerrainPointM(const TerrainHeightMap& heightMap,
                                   std::uint32_t x,
                                   std::uint32_t z) noexcept
{
    const float sampleXM = static_cast<float>(x) * heightMap.GetCellSpacingM();
    const float sampleZM = static_cast<float>(z) * heightMap.GetCellSpacingM();
    return {sampleXM - heightMap.GetWorldWidthM() * 0.5F,
            heightMap.SampleHeightM(sampleXM, sampleZM),
            sampleZM - heightMap.GetWorldLengthM() * 0.5F};
}

void AddTerrainLines(std::vector<Mesh::Vertex>& vertices, const TerrainHeightMap& heightMap)
{
    const std::uint32_t width = heightMap.GetWidth();
    const std::uint32_t length = heightMap.GetLength();
    if (width < 2U || length < 2U)
    {
        return;
    }

    for (std::uint32_t z = 0; z < length; ++z)
    {
        for (std::uint32_t x = 0; x < width; ++x)
        {
            const DirectX::XMFLOAT3 current = GetTerrainPointM(heightMap, x, z);
            if (x + 1U < width)
            {
                AddLine(vertices, current, GetTerrainPointM(heightMap, x + 1U, z));
            }
            if (z + 1U < length)
            {
                AddLine(vertices, current, GetTerrainPointM(heightMap, x, z + 1U));
            }
            if (x + 1U < width && z + 1U < length)
            {
                AddLine(vertices, GetTerrainPointM(heightMap, x + 1U, z), GetTerrainPointM(heightMap, x, z + 1U));
            }
        }
    }
}

std::shared_ptr<Mesh> CreateLineMesh(ID3D12Device& device, std::vector<Mesh::Vertex> vertices)
{
    if (vertices.empty())
    {
        return nullptr;
    }

    return Mesh::CreateLineList(device, vertices);
}

std::shared_ptr<Mesh> CreateColliderDebugMesh(ID3D12Device& device, ColliderComponent& collider)
{
    std::vector<Mesh::Vertex> vertices;

    switch (collider.GetType())
    {
    case ColliderType::Box:
        if (const auto* box = dynamic_cast<const BoxColliderComponent*>(&collider))
        {
            vertices.reserve(24);
            AddObbLines(vertices, box->GetLocalBox());
        }
        break;
    case ColliderType::Sphere:
        if (const auto* sphere = dynamic_cast<const SphereColliderComponent*>(&collider))
        {
            vertices.reserve(CIRCLE_SEGMENTS * 6U);
            AddSphereLines(vertices, sphere->GetLocalSphere().Center, sphere->GetLocalSphere().Radius);
        }
        break;
    case ColliderType::Capsule:
        if (const auto* capsule = dynamic_cast<const CapsuleColliderComponent*>(&collider))
        {
            vertices.reserve(CIRCLE_SEGMENTS * 4U + ARC_SEGMENTS * 8U + 8U);
            AddCapsuleLines(vertices,
                            capsule->GetLocalCenterM(),
                            capsule->GetLocalRadiusM(),
                            capsule->GetLocalHeightM());
        }
        break;
    case ColliderType::Cylinder:
        if (const auto* cylinder = dynamic_cast<const CylinderColliderComponent*>(&collider))
        {
            vertices.reserve(CIRCLE_SEGMENTS * 4U + 8U);
            AddCylinderLines(vertices,
                             cylinder->GetLocalCenterM(),
                             cylinder->GetLocalRadiusM(),
                             cylinder->GetLocalHeightM());
        }
        break;
    case ColliderType::Ramp:
        if (const auto* ramp = dynamic_cast<const RampColliderComponent*>(&collider))
        {
            vertices.reserve(26);
            AddRampLines(vertices, ramp->GetLocalBounds(), ramp->GetLocalDirection());
        }
        break;
    case ColliderType::Terrain:
        if (const auto* terrain = dynamic_cast<const TerrainColliderComponent*>(&collider))
        {
            const std::shared_ptr<const TerrainHeightMap>& heightMap = terrain->GetHeightMap();
            if (heightMap != nullptr && heightMap->GetWidth() >= 2U && heightMap->GetLength() >= 2U)
            {
                const std::size_t width = heightMap->GetWidth();
                const std::size_t length = heightMap->GetLength();
                vertices.reserve(((width - 1U) * length + width * (length - 1U) + (width - 1U) * (length - 1U)) *
                                 2U);
                AddTerrainLines(vertices, *heightMap);
            }
        }
        break;
    }

    return CreateLineMesh(device, std::move(vertices));
}

DirectX::XMFLOAT4 GetColliderColor(ColliderType type) noexcept
{
    constexpr DirectX::XMFLOAT4 DXPROJECT_BOX_COLOR = {0.1F, 0.95F, 1.0F, 1.0F};
    constexpr DirectX::XMFLOAT4 DXPROJECT_CAPSULE_COLOR = {0.2F, 1.0F, 0.25F, 1.0F};
    constexpr DirectX::XMFLOAT4 DXPROJECT_MESH_COLOR = {1.0F, 0.85F, 0.1F, 1.0F};
    constexpr DirectX::XMFLOAT4 DXPROJECT_FALLBACK_COLOR = {1.0F, 1.0F, 1.0F, 1.0F};

    switch (type)
    {
    case ColliderType::Box:
        return DXPROJECT_BOX_COLOR;
    case ColliderType::Capsule:
        return DXPROJECT_CAPSULE_COLOR;
    case ColliderType::Sphere:
        return DXPROJECT_FALLBACK_COLOR;
    case ColliderType::Cylinder:
        return DXPROJECT_BOX_COLOR;
    case ColliderType::Ramp:
        return DXPROJECT_MESH_COLOR;
    case ColliderType::Terrain:
        return DXPROJECT_MESH_COLOR;
    default:
        return DXPROJECT_FALLBACK_COLOR;
    }
}

const char* GetColliderTypeName(ColliderType type) noexcept
{
    switch (type)
    {
    case ColliderType::Box:
        return "Box";
    case ColliderType::Capsule:
        return "Capsule";
    case ColliderType::Sphere:
        return "Sphere";
    case ColliderType::Cylinder:
        return "Cylinder";
    case ColliderType::Ramp:
        return "Ramp";
    case ColliderType::Terrain:
        return "Terrain";
    default:
        return "Unknown";
    }
}

std::string BuildDebugObjectName(const ColliderComponent& collider)
{
    return std::string("Collider Debug ") + GetColliderTypeName(collider.GetType()) + " - " +
           collider.GetOwner().GetName();
}

void CopyTransform(const GameObject& source, GameObject& target) noexcept
{
    target.GetTransform().SetPositionM(source.GetTransform().GetPositionM());
    target.GetTransform().SetRotationRad(source.GetTransform().GetRotationRad());
    target.GetTransform().SetScale(source.GetTransform().GetScale());
}

void ConfigureDebugMaterial(MaterialComponent& materialComponent, const DirectX::XMFLOAT4& colorLinear) noexcept
{
    Material& material = materialComponent.GetMaterial();
    material.SetBaseColorLinear(colorLinear);
    material.SetSurface(0.0F, 1.0F);
    material.SetEmissionLinear({colorLinear.x, colorLinear.y, colorLinear.z}, DEBUG_EMISSION_INTENSITY);
}
} // namespace

void ColliderDebugDrawSystem::Clear() noexcept
{
    mDebugColliders.clear();
    mVisible = false;
}

void ColliderDebugDrawSystem::RegisterCollider(ID3D12Device& device, Scene& scene, ColliderComponent& collider)
{
    const auto exists = std::find_if(mDebugColliders.begin(),
                                     mDebugColliders.end(),
                                     [&collider](const DebugCollider& entry)
                                     {
                                         return entry.collider == &collider;
                                     });
    if (exists != mDebugColliders.end())
    {
        return;
    }

    std::shared_ptr<Mesh> mesh = CreateColliderDebugMesh(device, collider);
    if (!mesh)
    {
        return;
    }

    GameObject& debugObject = scene.CreateObject(BuildDebugObjectName(collider));
    debugObject.AddComponent<MeshComponent>(std::move(mesh));
    ConfigureDebugMaterial(debugObject.AddComponent<MaterialComponent>(), GetColliderColor(collider.GetType()));
    CopyTransform(collider.GetOwner(), debugObject);
    debugObject.SetActive(mVisible && collider.GetOwner().IsActive());

    mDebugColliders.push_back({&collider, &debugObject});
}

void ColliderDebugDrawSystem::SetVisible(bool visible) noexcept
{
    mVisible = visible;
    Sync();
}

void ColliderDebugDrawSystem::ToggleVisible() noexcept
{
    SetVisible(!mVisible);
}

bool ColliderDebugDrawSystem::IsVisible() const noexcept
{
    return mVisible;
}

void ColliderDebugDrawSystem::Sync() noexcept
{
    for (const DebugCollider& entry : mDebugColliders)
    {
        if (entry.collider == nullptr || entry.debugObject == nullptr)
        {
            continue;
        }

        const GameObject& owner = entry.collider->GetOwner();
        CopyTransform(owner, *entry.debugObject);
        entry.debugObject->SetActive(mVisible && owner.IsActive());
    }
}
} // namespace Kimgane::Engine
