#include "Pch.h"

#include "ColliderDebugDrawSystem.h"

#include "../Core/GameObject.h"
#include "../Core/SharedVec3Conversion.h"
#include "../Rendering/Mesh.h"
#include "../Scene/Scene.h"
#include "../../Shared/Physics/CharacterMovement.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace Kimgane::Client
{
namespace
{
namespace SharedPhysics = Kimgane::Shared::Physics;

constexpr int CIRCLE_SEGMENTS = 32;
constexpr int ARC_SEGMENTS = 16;
constexpr float DEBUG_EMISSION_INTENSITY = 0.25F;
constexpr DirectX::XMFLOAT3 DEBUG_NORMAL = {0.0F, 1.0F, 0.0F};
constexpr DirectX::XMFLOAT4 DEBUG_VERTEX_COLOR = {1.0F, 1.0F, 1.0F, 1.0F};

constexpr DirectX::XMFLOAT4 BOX_COLOR = {0.1F, 0.95F, 1.0F, 1.0F};
constexpr DirectX::XMFLOAT4 CAPSULE_COLOR = {0.2F, 1.0F, 0.25F, 1.0F};
constexpr DirectX::XMFLOAT4 SURFACE_COLOR = {1.0F, 0.85F, 0.1F, 1.0F};
constexpr DirectX::XMFLOAT4 FALLBACK_COLOR = {1.0F, 1.0F, 1.0F, 1.0F};

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

void AddLine(std::vector<Mesh::Vertex>& vertices, const DirectX::XMFLOAT3& fromM, const DirectX::XMFLOAT3& toM)
{
    vertices.push_back(MakeLineVertex(fromM));
    vertices.push_back(MakeLineVertex(toM));
}

void AddAabbLines(std::vector<Mesh::Vertex>& vertices, const DirectX::XMFLOAT3& centerM, const DirectX::XMFLOAT3& halfM)
{
    const float minX = centerM.x - halfM.x;
    const float maxX = centerM.x + halfM.x;
    const float minY = centerM.y - halfM.y;
    const float maxY = centerM.y + halfM.y;
    const float minZ = centerM.z - halfM.z;
    const float maxZ = centerM.z + halfM.z;

    const std::array<DirectX::XMFLOAT3, 8> cornersM = {
        DirectX::XMFLOAT3{minX, minY, minZ}, DirectX::XMFLOAT3{maxX, minY, minZ},
        DirectX::XMFLOAT3{maxX, minY, maxZ}, DirectX::XMFLOAT3{minX, minY, maxZ},
        DirectX::XMFLOAT3{minX, maxY, minZ}, DirectX::XMFLOAT3{maxX, maxY, minZ},
        DirectX::XMFLOAT3{maxX, maxY, maxZ}, DirectX::XMFLOAT3{minX, maxY, maxZ}};
    constexpr std::array<int, 24> EDGE_INDICES = {
        0, 1, 1, 2, 2, 3, 3, 0,
        4, 5, 5, 6, 6, 7, 7, 4,
        0, 4, 1, 5, 2, 6, 3, 7};

    for (std::size_t index = 0; index + 1U < EDGE_INDICES.size(); index += 2U)
    {
        AddLine(vertices, cornersM[EDGE_INDICES[index]], cornersM[EDGE_INDICES[index + 1U]]);
    }
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

// 세로 원기둥 옆면(위/아래 원 + 세로선 4개). 캡슐과 원기둥이 같이 씀.
void AddVerticalTubeLines(std::vector<Mesh::Vertex>& vertices,
                          DirectX::FXMVECTOR bottom,
                          DirectX::FXMVECTOR top,
                          float radiusM)
{
    const DirectX::XMVECTOR right = DirectX::XMVectorSet(1.0F, 0.0F, 0.0F, 0.0F);
    const DirectX::XMVECTOR forward = DirectX::XMVectorSet(0.0F, 0.0F, 1.0F, 0.0F);

    AddCircleLines(vertices, bottom, right, forward, radiusM);
    AddCircleLines(vertices, top, right, forward, radiusM);

    const std::array<DirectX::XMVECTOR, 4> radialDirections = {
        right, DirectX::XMVectorScale(right, -1.0F), forward, DirectX::XMVectorScale(forward, -1.0F)};
    for (const DirectX::XMVECTOR& radial : radialDirections)
    {
        AddLine(vertices,
                StoreVector3(DirectX::XMVectorAdd(bottom, DirectX::XMVectorScale(radial, radiusM))),
                StoreVector3(DirectX::XMVectorAdd(top, DirectX::XMVectorScale(radial, radiusM))));
    }
}

void AddCapsuleLines(std::vector<Mesh::Vertex>& vertices, const DirectX::XMFLOAT3& centerM, float radiusM, float heightM)
{
    const float halfSegmentM = std::max((heightM * 0.5F) - radiusM, 0.0F);
    const DirectX::XMVECTOR center = DirectX::XMLoadFloat3(&centerM);
    const DirectX::XMVECTOR axis = DirectX::XMVectorSet(0.0F, 1.0F, 0.0F, 0.0F);
    const DirectX::XMVECTOR right = DirectX::XMVectorSet(1.0F, 0.0F, 0.0F, 0.0F);
    const DirectX::XMVECTOR forward = DirectX::XMVectorSet(0.0F, 0.0F, 1.0F, 0.0F);
    const DirectX::XMVECTOR bottom = DirectX::XMVectorAdd(center, DirectX::XMVectorScale(axis, -halfSegmentM));
    const DirectX::XMVECTOR top = DirectX::XMVectorAdd(center, DirectX::XMVectorScale(axis, halfSegmentM));

    AddVerticalTubeLines(vertices, bottom, top, radiusM);
    AddHemisphereArc(vertices, top, axis, right, radiusM, true);
    AddHemisphereArc(vertices, top, axis, forward, radiusM, true);
    AddHemisphereArc(vertices, bottom, axis, right, radiusM, false);
    AddHemisphereArc(vertices, bottom, axis, forward, radiusM, false);
}

void AddSphereLines(std::vector<Mesh::Vertex>& vertices, const DirectX::XMFLOAT3& centerM, float radiusM)
{
    const DirectX::XMVECTOR center = DirectX::XMLoadFloat3(&centerM);
    const DirectX::XMVECTOR right = DirectX::XMVectorSet(1.0F, 0.0F, 0.0F, 0.0F);
    const DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0F, 1.0F, 0.0F, 0.0F);
    const DirectX::XMVECTOR forward = DirectX::XMVectorSet(0.0F, 0.0F, 1.0F, 0.0F);

    AddCircleLines(vertices, center, right, forward, radiusM);
    AddCircleLines(vertices, center, right, up, radiusM);
    AddCircleLines(vertices, center, forward, up, radiusM);
}

void AddCylinderLines(std::vector<Mesh::Vertex>& vertices, const DirectX::XMFLOAT3& centerM, float radiusM, float heightM)
{
    const DirectX::XMVECTOR center = DirectX::XMLoadFloat3(&centerM);
    const DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0F, 1.0F, 0.0F, 0.0F);
    AddVerticalTubeLines(vertices,
                         DirectX::XMVectorAdd(center, DirectX::XMVectorScale(up, heightM * -0.5F)),
                         DirectX::XMVectorAdd(center, DirectX::XMVectorScale(up, heightM * 0.5F)),
                         radiusM);
}

float ComputeRampSurfaceY(const SharedPhysics::Ramp& ramp, float x, float z) noexcept
{
    const float halfX = ramp.sizeM.x * 0.5F;
    const float halfZ = ramp.sizeM.z * 0.5F;
    const float minX = ramp.centerM.x - halfX;
    const float maxX = ramp.centerM.x + halfX;
    const float minZ = ramp.centerM.z - halfZ;
    const float maxZ = ramp.centerM.z + halfZ;
    const float bottomY = ramp.centerM.y - ramp.sizeM.y * 0.5F;

    float progress = 0.0F;
    switch (ramp.direction)
    {
    case SharedPhysics::RampDirection::PositiveX:
        progress = (x - minX) / std::max(maxX - minX, 0.001F);
        break;
    case SharedPhysics::RampDirection::NegativeX:
        progress = (maxX - x) / std::max(maxX - minX, 0.001F);
        break;
    case SharedPhysics::RampDirection::NegativeZ:
        progress = (maxZ - z) / std::max(maxZ - minZ, 0.001F);
        break;
    case SharedPhysics::RampDirection::PositiveZ:
    default:
        progress = (z - minZ) / std::max(maxZ - minZ, 0.001F);
        break;
    }

    return bottomY + ramp.sizeM.y * std::clamp(progress, 0.0F, 1.0F);
}

void AddRampLines(std::vector<Mesh::Vertex>& vertices, const SharedPhysics::Ramp& ramp)
{
    const float halfX = ramp.sizeM.x * 0.5F;
    const float halfZ = ramp.sizeM.z * 0.5F;
    const float minX = ramp.centerM.x - halfX;
    const float maxX = ramp.centerM.x + halfX;
    const float minZ = ramp.centerM.z - halfZ;
    const float maxZ = ramp.centerM.z + halfZ;
    const float bottomY = ramp.centerM.y - ramp.sizeM.y * 0.5F;

    const std::array<DirectX::XMFLOAT3, 4> bottomCorners = {
        DirectX::XMFLOAT3{minX, bottomY, minZ},
        DirectX::XMFLOAT3{maxX, bottomY, minZ},
        DirectX::XMFLOAT3{maxX, bottomY, maxZ},
        DirectX::XMFLOAT3{minX, bottomY, maxZ}};
    std::array<DirectX::XMFLOAT3, 4> surfaceCorners = bottomCorners;
    for (DirectX::XMFLOAT3& corner : surfaceCorners)
    {
        corner.y = ComputeRampSurfaceY(ramp, corner.x, corner.z);
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
                                   const DirectX::XMFLOAT3& terrainPositionM,
                                   std::uint32_t x,
                                   std::uint32_t z) noexcept
{
    const float sampleXM = static_cast<float>(x) * heightMap.GetCellSpacingM();
    const float sampleZM = static_cast<float>(z) * heightMap.GetCellSpacingM();
    return {terrainPositionM.x + sampleXM - heightMap.GetWorldWidthM() * 0.5F,
            terrainPositionM.y + heightMap.SampleHeightM(sampleXM, sampleZM),
            terrainPositionM.z + sampleZM - heightMap.GetWorldLengthM() * 0.5F};
}

void AddTerrainLines(std::vector<Mesh::Vertex>& vertices,
                     const TerrainHeightMap& heightMap,
                     const DirectX::XMFLOAT3& terrainPositionM)
{
    const std::uint32_t width = heightMap.GetWidth();
    const std::uint32_t length = heightMap.GetLength();
    if (width < 2U || length < 2U)
    {
        return;
    }

    vertices.reserve(vertices.size() +
                     ((static_cast<std::size_t>(width) - 1U) * length + width * (static_cast<std::size_t>(length) - 1U) +
                      (static_cast<std::size_t>(width) - 1U) * (length - 1U)) * 2U);
    for (std::uint32_t z = 0; z < length; ++z)
    {
        for (std::uint32_t x = 0; x < width; ++x)
        {
            const DirectX::XMFLOAT3 current = GetTerrainPointM(heightMap, terrainPositionM, x, z);
            if (x + 1U < width)
            {
                AddLine(vertices, current, GetTerrainPointM(heightMap, terrainPositionM, x + 1U, z));
            }
            if (z + 1U < length)
            {
                AddLine(vertices, current, GetTerrainPointM(heightMap, terrainPositionM, x, z + 1U));
            }
            if (x + 1U < width && z + 1U < length)
            {
                AddLine(vertices,
                        GetTerrainPointM(heightMap, terrainPositionM, x + 1U, z),
                        GetTerrainPointM(heightMap, terrainPositionM, x, z + 1U));
            }
        }
    }
}

struct DebugShape
{
    std::vector<Mesh::Vertex> vertices;
    DirectX::XMFLOAT4 colorLinear = FALLBACK_COLOR;
    const char* typeName = "Unknown";
};

// 월드 좌표 그대로 선을 만듦. 디버그 오브젝트의 Transform은 원점에 둠.
DebugShape BuildBodyShape(const SharedPhysics::CollisionBody& body,
                          const TerrainHeightMap* terrainHeightMap,
                          const DirectX::XMFLOAT3& terrainPositionM)
{
    DebugShape shape;
    std::visit(
        [&](const auto& value)
        {
            using Shape = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Shape, SharedPhysics::Box>)
            {
                AddAabbLines(shape.vertices, ToXMFloat3(value.centerM), ToXMFloat3(value.halfExtentsM));
                shape.colorLinear = BOX_COLOR;
                shape.typeName = "Box";
            }
            else if constexpr (std::is_same_v<Shape, SharedPhysics::Sphere>)
            {
                AddSphereLines(shape.vertices, ToXMFloat3(value.centerM), value.radiusM);
                shape.typeName = "Sphere";
            }
            else if constexpr (std::is_same_v<Shape, SharedPhysics::Capsule>)
            {
                AddCapsuleLines(shape.vertices, ToXMFloat3(value.centerM), value.radiusM, value.heightM);
                shape.colorLinear = CAPSULE_COLOR;
                shape.typeName = "Capsule";
            }
            else if constexpr (std::is_same_v<Shape, SharedPhysics::Cylinder>)
            {
                AddCylinderLines(shape.vertices, ToXMFloat3(value.centerM), value.radiusM, value.heightM);
                shape.colorLinear = BOX_COLOR;
                shape.typeName = "Cylinder";
            }
            else if constexpr (std::is_same_v<Shape, SharedPhysics::Ramp>)
            {
                AddRampLines(shape.vertices, value);
                shape.colorLinear = SURFACE_COLOR;
                shape.typeName = "Ramp";
            }
            else if constexpr (std::is_same_v<Shape, SharedPhysics::TerrainSurface>)
            {
                // 샘플러에는 격자 정보가 없어서, 같은 높이맵을 쓰는 클라 지형 데이터로 그림.
                if (terrainHeightMap != nullptr)
                {
                    AddTerrainLines(shape.vertices, *terrainHeightMap, terrainPositionM);
                }
                shape.colorLinear = SURFACE_COLOR;
                shape.typeName = "Terrain";
            }
        },
        body.shape);
    return shape;
}

GameObject* CreateDebugObject(ID3D12Device& device, Scene& scene, DebugShape shape, const std::string& name)
{
    if (shape.vertices.empty())
    {
        return nullptr;
    }

    std::shared_ptr<Mesh> mesh = Mesh::CreateLineList(device, shape.vertices);
    if (!mesh)
    {
        return nullptr;
    }

    GameObject& debugObject = scene.CreateObject(name);
    debugObject.SetMesh(std::move(mesh));
    Material& material = debugObject.GetMaterial();
    material.SetBaseColorLinear(shape.colorLinear);
    material.SetSurface(0.0F, 1.0F);
    material.SetEmissionLinear({shape.colorLinear.x, shape.colorLinear.y, shape.colorLinear.z}, DEBUG_EMISSION_INTENSITY);
    return &debugObject;
}
} // namespace

void ColliderDebugDrawSystem::Initialize(ID3D12Device& device,
                                         std::shared_ptr<const TerrainHeightMap> terrainHeightMap,
                                         const DirectX::XMFLOAT3& terrainPositionM)
{
    Clear();
    mDevice = &device;
    mTerrainHeightMap = std::move(terrainHeightMap);
    mTerrainPositionM = terrainPositionM;
}

void ColliderDebugDrawSystem::Clear() noexcept
{
    // 디버그 오브젝트는 Scene이 소유하므로 Scene::Clear()와 함께 정리됨. 여기서는 참조만 버림.
    mBodyObjects.clear();
    mLocalPlayerCapsuleObject = nullptr;
    mVisible = false;
}

void ColliderDebugDrawSystem::SetVisible(bool visible) noexcept
{
    mVisible = visible;
    if (!mVisible)
    {
        HideAll();
    }
}

void ColliderDebugDrawSystem::ToggleVisible() noexcept
{
    SetVisible(!mVisible);
}

bool ColliderDebugDrawSystem::IsVisible() const noexcept
{
    return mVisible;
}

void ColliderDebugDrawSystem::Sync(Scene& scene,
                                   const SharedPhysics::CollisionWorld& world,
                                   const DirectX::XMFLOAT3& localPlayerFootM)
{
    if (!mVisible || mDevice == nullptr)
    {
        return;
    }

    // 월드에서 빠진 body(열린 상자 등)의 표시를 끄고 목록에서 뺌.
    for (auto iter = mBodyObjects.begin(); iter != mBodyObjects.end();)
    {
        if (world.ContainsBody(iter->first))
        {
            ++iter;
            continue;
        }
        if (iter->second != nullptr)
        {
            iter->second->SetActive(false);
        }
        iter = mBodyObjects.erase(iter);
    }

    for (const SharedPhysics::CollisionBody& body : world.GetBodies())
    {
        auto [iter, inserted] = mBodyObjects.try_emplace(body.objectId, nullptr);
        if (inserted)
        {
            DebugShape shape = BuildBodyShape(body, mTerrainHeightMap.get(), mTerrainPositionM);
            const std::string name =
                std::string("Collider Debug ") + shape.typeName + " " + std::to_string(body.objectId);
            iter->second = CreateDebugObject(*mDevice, scene, std::move(shape), name);
        }
        if (iter->second != nullptr)
        {
            iter->second->SetActive(true);
        }
    }

    if (mLocalPlayerCapsuleObject == nullptr)
    {
        // 발 위치 원점 기준 캡슐을 만들어 두고 Transform만 옮김. 크기는 이동 계산과 같은 Shared 값.
        const SharedPhysics::Capsule localCapsule = SharedPhysics::MakeCapsuleFromFootPosition(
            {}, SharedPhysics::Settings::PLAYER_CAPSULE_RADIUS_M, SharedPhysics::Settings::PLAYER_CAPSULE_HEIGHT_M);
        DebugShape shape;
        AddCapsuleLines(shape.vertices, ToXMFloat3(localCapsule.centerM), localCapsule.radiusM, localCapsule.heightM);
        shape.colorLinear = CAPSULE_COLOR;
        mLocalPlayerCapsuleObject = CreateDebugObject(*mDevice, scene, std::move(shape), "Collider Debug Local Player");
    }
    if (mLocalPlayerCapsuleObject != nullptr)
    {
        mLocalPlayerCapsuleObject->GetTransform().SetPositionM(localPlayerFootM);
        mLocalPlayerCapsuleObject->SetActive(true);
    }
}

void ColliderDebugDrawSystem::HideAll() noexcept
{
    for (const auto& [objectId, debugObject] : mBodyObjects)
    {
        if (debugObject != nullptr)
        {
            debugObject->SetActive(false);
        }
    }
    if (mLocalPlayerCapsuleObject != nullptr)
    {
        mLocalPlayerCapsuleObject->SetActive(false);
    }
}
} // namespace Kimgane::Client
