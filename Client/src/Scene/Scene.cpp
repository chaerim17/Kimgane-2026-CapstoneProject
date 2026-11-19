#include "Pch.h"

#include "Scene.h"

#include "../Rendering/SceneRenderConstants.h"
#include "../Terrain/LunarTerrainRenderer.h"

#include <utility>

namespace Kimgane::Client
{
void Scene::Clear() noexcept
{
    mTerrainRenderer.reset();
    mTextLabels.clear();
    mObjects.clear();
}

void Scene::Update(float deltaTimeSec)
{
    for (const auto& object : mObjects)
    {
        if (object)
        {
            object->Update(deltaTimeSec);
        }
    }
}

void Scene::Render(ID3D12GraphicsCommandList& commandList,
                   D3D12_GPU_DESCRIPTOR_HANDLE defaultTextureGpuHandle,
                   MeshPrimitiveTopology primitiveTopology) const
{
    for (const auto& object : mObjects)
    {
        if (!object || !object->IsActive())
        {
            continue;
        }

        const std::shared_ptr<Mesh>& mesh = object->GetMesh();
        if (!mesh || mesh->GetPrimitiveTopology() != primitiveTopology)
        {
            continue;
        }

        const Material& material = object->GetMaterial();
        ObjectShaderConstants objectConstants = {};
        objectConstants.world = GetRenderWorldMatrix(*object);
        objectConstants.baseColor = material.GetBaseColorLinear();
        objectConstants.surface = {material.GetMetallic(), material.GetRoughness(), 0.0F, 0.0F};
        objectConstants.emission = material.GetEmissionLinear();
        commandList.SetGraphicsRoot32BitConstants(RenderRootParameter::OBJECT,
                                                  RenderRootParameter::OBJECT_CONSTANTS_32BIT_COUNT,
                                                  &objectConstants,
                                                  0);

        // 텍스처가 없는 머티리얼은 기본 흰색 텍스처를 대신 바인딩 (셰이더가 항상 t0을 읽을 수 있어야 함)
        D3D12_GPU_DESCRIPTOR_HANDLE textureHandle = material.GetTextureGpuHandle();
        if (textureHandle.ptr == 0)
        {
            textureHandle = defaultTextureGpuHandle;
        }
        commandList.SetGraphicsRootDescriptorTable(RenderRootParameter::TEXTURE, textureHandle);

        mesh->Render(commandList);
    }
}

DirectX::XMFLOAT4X4 Scene::GetRenderWorldMatrix(const GameObject& object) const noexcept
{
    return object.GetTransform().GetWorldMatrix4x4();
}

const std::vector<std::unique_ptr<GameObject>>& Scene::GetObjects() const noexcept
{
    return mObjects;
}

TextLabel& Scene::CreateTextLabel(const GameObject& panel, std::wstring text, float fontSizeDip)
{
    mTextLabels.push_back(std::make_unique<TextLabel>(panel, std::move(text), fontSizeDip));
    return *mTextLabels.back();
}

const std::vector<std::unique_ptr<TextLabel>>& Scene::GetTextLabels() const noexcept
{
    return mTextLabels;
}

void Scene::CollectWorldHealthBars(std::vector<WorldHealthBarView>& outHealthBars) const
{
    (void)outHealthBars;
}

std::span<const DamageNumber> Scene::GetDamageNumbers() const noexcept
{
    return {};
}

const DirectionalLight& Scene::GetDirectionalLight() const noexcept
{
    return mDirectionalLight;
}

DirectionalLight& Scene::GetDirectionalLight() noexcept
{
    return mDirectionalLight;
}

} // namespace Kimgane::Client
