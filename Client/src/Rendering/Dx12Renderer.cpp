#include "Pch.h"

#include "Dx12Renderer.h"

#include <d2d1_1helper.h>

#include "../Camera/Camera.h"
#include "../Scene/Scene.h"
#include "../Terrain/LunarTerrainRenderer.h"
#include "../UI/DamageNumber.h"
#include "../UI/TextLabel.h"
#include "../UI/WorldHealthBar.h"
#include "../../Shared/Physics/RaycastQueries.h"
#include "../../Shared/IO/AssetPathResolver.h"
#include "SceneRenderConstants.h"
#include "Shader.h"
#include "Texture.h"

#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <stdexcept>

namespace Kimgane::Client
{
namespace
{
using Microsoft::WRL::ComPtr;

constexpr wchar_t UI_FONT_DIRECTORY_PATH[] = L"Assets/Fonts/Paperlogy-1.001";
constexpr wchar_t UI_FONT_FAMILY_NAME[] = L"Paperlogy";
constexpr float D2D_DEFAULT_DPI = 96.0F;
// UI 이미지는 화면에 작게(수십 px) 그려지므로, 큰 원본은 로드할 때 이 크기로 줄여서 메모리와 축소 품질을 챙김
constexpr UINT UI_IMAGE_MAX_SIZE_PX = 128;
constexpr float MIN_PROJECTED_W = 0.0001F;
// 카메라~체력바 사이에 벽/지형이 있는지 볼 때, 부동소수점 오차로 자기 발밑 지형에
// 스스로 막힌 것처럼 오판하지 않도록 살짝 여유를 둠.
constexpr float HEALTH_BAR_OCCLUSION_MARGIN_M = 0.1F;

void ThrowIfFailed(HRESULT result)
{
    if (FAILED(result))
    {
        throw std::runtime_error("DirectX 12 호출에 실패했습니다.");
    }
}

void GetHardwareAdapter(IDXGIFactory1* factory, IDXGIAdapter1** adapter)
{
    *adapter = nullptr;

    for (UINT adapterIndex = 0;; ++adapterIndex)
    {
        ComPtr<IDXGIAdapter1> candidate;
        if (factory->EnumAdapters1(adapterIndex, &candidate) == DXGI_ERROR_NOT_FOUND)
        {
            break;
        }

        DXGI_ADAPTER_DESC1 description = {};
        ThrowIfFailed(candidate->GetDesc1(&description));

        if ((description.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0U)
        {
            continue;
        }

        if (SUCCEEDED(D3D12CreateDevice(candidate.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
        {
            *adapter = candidate.Detach();
            return;
        }
    }
}

bool IsTrueTypeFontFile(const std::filesystem::path& filePath)
{
    std::wstring extension = filePath.extension().wstring();
    std::transform(extension.begin(),
                   extension.end(),
                   extension.begin(),
                   [](wchar_t character)
                   {
                       return static_cast<wchar_t>(std::towlower(character));
                   });
    return extension == L".ttf";
}

D2D1_COLOR_F ToD2DColor(const DirectX::XMFLOAT4& colorLinear) noexcept
{
    return {std::clamp(colorLinear.x, 0.0F, 1.0F),
            std::clamp(colorLinear.y, 0.0F, 1.0F),
            std::clamp(colorLinear.z, 0.0F, 1.0F),
            std::clamp(colorLinear.w, 0.0F, 1.0F)};
}

DWRITE_TEXT_ALIGNMENT ToDWriteTextAlignment(int alignment) noexcept
{
    switch (static_cast<TextHorizontalAlignment>(alignment))
    {
    case TextHorizontalAlignment::Left:
        return DWRITE_TEXT_ALIGNMENT_LEADING;
    case TextHorizontalAlignment::Right:
        return DWRITE_TEXT_ALIGNMENT_TRAILING;
    case TextHorizontalAlignment::Center:
    default:
        return DWRITE_TEXT_ALIGNMENT_CENTER;
    }
}

DWRITE_PARAGRAPH_ALIGNMENT ToDWriteParagraphAlignment(int alignment) noexcept
{
    switch (static_cast<TextVerticalAlignment>(alignment))
    {
    case TextVerticalAlignment::Top:
        return DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
    case TextVerticalAlignment::Bottom:
        return DWRITE_PARAGRAPH_ALIGNMENT_FAR;
    case TextVerticalAlignment::Center:
    default:
        return DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
    }
}
} // namespace

Dx12Renderer::~Dx12Renderer()
{
    if (mFenceEvent != nullptr)
    {
        CloseHandle(mFenceEvent);
        mFenceEvent = nullptr;
    }
}

void Dx12Renderer::Initialize(HWND windowHandle, UINT widthPx, UINT heightPx)
{
    mWindowHandle = windowHandle;
    mWidthPx = widthPx;
    mHeightPx = heightPx;

    CreateDeviceResources();
    CreateSwapChain();
    CreateRenderTargetViews();
    CreateShaderResourceViewHeap();
    CreateDepthStencilView();
    CreateDefaultTexture();
    CreateCommandObjects();
    CreatePipelineObjects();
    CreateTextOverlayResources();
    CreateFenceObjects();
    DirectX::XMStoreFloat4x4(&mViewProjection, DirectX::XMMatrixIdentity());
}

void Dx12Renderer::SetViewProjection(const DirectX::XMFLOAT4X4& viewProjection) noexcept
{
    mViewProjection = viewProjection;
}

void Dx12Renderer::SetCameraPositionM(const DirectX::XMFLOAT3& cameraPositionM) noexcept
{
    mCameraPositionM = cameraPositionM;
}

void Dx12Renderer::SetCrosshairVisible(bool visible) noexcept
{
    mCrosshairVisible = visible;
}

void Dx12Renderer::QueueUiFillRect(const DirectX::XMFLOAT4& rectPx, const DirectX::XMFLOAT4& colorLinear)
{
    UiDrawCommand command;
    command.kind = UiDrawKind::FillRect;
    command.rectPx = rectPx;
    command.colorLinear = colorLinear;
    mUiDrawCommands.push_back(std::move(command));
}

void Dx12Renderer::QueueUiStrokeRect(const DirectX::XMFLOAT4& rectPx,
                                     const DirectX::XMFLOAT4& colorLinear,
                                     float thicknessPx)
{
    UiDrawCommand command;
    command.kind = UiDrawKind::StrokeRect;
    command.rectPx = rectPx;
    command.colorLinear = colorLinear;
    command.thicknessPx = thicknessPx;
    mUiDrawCommands.push_back(std::move(command));
}

void Dx12Renderer::QueueUiText(std::wstring text,
                               const DirectX::XMFLOAT4& rectPx,
                               const DirectX::XMFLOAT4& colorLinear,
                               float fontSizeDip,
                               int horizontalAlignment,
                               int verticalAlignment,
                               bool drawShadow)
{
    UiDrawCommand command;
    command.kind = UiDrawKind::Text;
    command.text.text = std::move(text);
    command.text.rectPx = rectPx;
    command.text.colorLinear = colorLinear;
    command.text.fontSizeDip = fontSizeDip;
    command.text.horizontalAlignment = horizontalAlignment;
    command.text.verticalAlignment = verticalAlignment;
    command.text.drawShadow = drawShadow;
    command.text.shadowOffsetPx = RenderSettings::UI_TEXT_SHADOW_OFFSET_PX;
    mUiDrawCommands.push_back(std::move(command));
}

void Dx12Renderer::QueueUiImage(std::wstring imagePath, const DirectX::XMFLOAT4& rectPx, float opacity)
{
    UiDrawCommand command;
    command.kind = UiDrawKind::Image;
    command.rectPx = rectPx;
    command.colorLinear = {1.0F, 1.0F, 1.0F, opacity};
    command.imagePath = std::move(imagePath);
    mUiDrawCommands.push_back(std::move(command));
}

void Dx12Renderer::Render(const Scene& scene)
{
    BeginFrame();
    RenderScene(scene);
    EndFrame();
}

void Dx12Renderer::BeginFrame()
{
    mTextDrawCommands.clear();
    mHealthBarDrawCommands.clear();
    mUiDrawCommands.clear();

    ThrowIfFailed(mCommandAllocators[mFrameIndex]->Reset());
    ThrowIfFailed(mCommandList->Reset(mCommandAllocators[mFrameIndex].Get(), mPipelineState.Get()));

    D3D12_RESOURCE_BARRIER barrierToRenderTarget = {};
    barrierToRenderTarget.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierToRenderTarget.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrierToRenderTarget.Transition.pResource = mRenderTargets[mFrameIndex].Get();
    barrierToRenderTarget.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrierToRenderTarget.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrierToRenderTarget.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    mCommandList->ResourceBarrier(1, &barrierToRenderTarget);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += static_cast<SIZE_T>(mFrameIndex) * mRtvDescriptorSize;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = mDsvHeap->GetCPUDescriptorHandleForHeapStart();

    mCommandList->RSSetViewports(1, &mViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);
    mCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
    mCommandList->ClearRenderTargetView(rtvHandle, RenderSettings::CLEAR_COLOR.data(), 0, nullptr);
    mCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0F, 0, 0, nullptr);
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
    ID3D12DescriptorHeap* descriptorHeaps[] = {mSrvHeap.Get()};
    mCommandList->SetDescriptorHeaps(1, descriptorHeaps);
}

void Dx12Renderer::RenderSkybox(const Camera& camera, D3D12_GPU_DESCRIPTOR_HANDLE cubeTexture)
{
    // 시점의 이동을 빼고 회전과 렌즈만 사용해서 픽셀의 월드 방향을 복원한다.
    DirectX::XMMATRIX viewRotation = camera.GetViewMatrix();
    viewRotation.r[3] = DirectX::XMVectorSet(0.0F, 0.0F, 0.0F, 1.0F);
    const DirectX::XMMATRIX clipToWorld =
        DirectX::XMMatrixInverse(nullptr, viewRotation * camera.GetProjectionMatrix());
    DirectX::XMFLOAT4X4 clipToWorldConstants;
    DirectX::XMStoreFloat4x4(&clipToWorldConstants, clipToWorld);

    mCommandList->SetPipelineState(mSkyboxPipelineState.Get());
    mCommandList->SetGraphicsRoot32BitConstants(RenderRootParameter::SCENE,
                                                RenderRootParameter::MATRIX_32BIT_COUNT,
                                                &clipToWorldConstants,
                                                0);
    mCommandList->SetGraphicsRootDescriptorTable(RenderRootParameter::TEXTURE, cubeTexture);
    mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mCommandList->DrawInstanced(3, 1, 0, 0);
}

void Dx12Renderer::RenderDetectionPulse(const Scene& scene,
                                        std::span<const GameObject* const> targets, float opacity)
{
    if (targets.empty() || opacity <= 0.0F)
        return;

    mCommandList->OMSetRenderTargets(0, nullptr, FALSE, &mDetectionTargetDsv);
    mCommandList->ClearDepthStencilView(mDetectionTargetDsv, D3D12_CLEAR_FLAG_DEPTH, 1.0F, 0, 0, nullptr);
    mCommandList->SetPipelineState(mDetectionDepthPipeline.Get());
    mCommandList->SetGraphicsRoot32BitConstants(RenderRootParameter::SCENE,
                                                RenderRootParameter::MATRIX_32BIT_COUNT, &mViewProjection, 0);
    for (const GameObject* target : targets)
    {
        const auto& mesh = target->GetMesh();
        if (!mesh || mesh->GetPrimitiveTopology() != MeshPrimitiveTopology::TriangleList)
            continue;
        const auto world = scene.GetRenderWorldMatrix(*target);
        mCommandList->SetGraphicsRoot32BitConstants(RenderRootParameter::OBJECT,
                                                    RenderRootParameter::MATRIX_32BIT_COUNT, &world, 0);
        mesh->Render(*mCommandList.Get());
    }

    auto rtvHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += static_cast<SIZE_T>(mFrameIndex) * mRtvDescriptorSize;
    mCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    D3D12_RESOURCE_BARRIER barriers[2]{};
    ID3D12Resource* depthResources[] = {mDepthStencil.Get(), mDetectionTargetDepth.Get()};
    for (UINT i = 0; i < _countof(barriers); ++i)
    {
        barriers[i].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barriers[i].Transition.pResource = depthResources[i];
        barriers[i].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barriers[i].Transition.StateBefore = D3D12_RESOURCE_STATE_DEPTH_WRITE;
        barriers[i].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }
    mCommandList->ResourceBarrier(_countof(barriers), barriers);

    const DirectX::XMFLOAT4 pulseParams{opacity, 0.0F, 0.0F, 0.0F};
    mCommandList->SetGraphicsRootSignature(mDetectionCompositeRootSignature.Get());
    mCommandList->SetPipelineState(mDetectionCompositePipeline.Get());
    mCommandList->SetGraphicsRootDescriptorTable(0, mDetectionDepthViews);
    mCommandList->SetGraphicsRoot32BitConstants(1, 4, &pulseParams, 0);
    mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    mCommandList->DrawInstanced(3, 1, 0, 0);

    for (auto& barrier : barriers)
        std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
    mCommandList->ResourceBarrier(_countof(barriers), barriers);
    const auto dsvHandle = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
    mCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
}

void Dx12Renderer::RenderScene(const Scene& scene,
                               RenderPass pass,
                               bool includeText,
                               MeshPrimitiveTopology primitiveTopology)
{
    ID3D12PipelineState* pipelineState = primitiveTopology == MeshPrimitiveTopology::LineList
                                             ? mLinePipelineState.Get()
                                             : (pass == RenderPass::Overlay ? mOverlayPipelineState.Get() : mPipelineState.Get());
    if (pipelineState != nullptr)
    {
        mCommandList->SetPipelineState(pipelineState);
    }

    const DirectionalLightShaderData lightShaderData = scene.GetDirectionalLight().BuildShaderData();
    SceneShaderConstants sceneConstants = {};
    sceneConstants.viewProjection = mViewProjection;
    sceneConstants.lightDirectionIntensity = lightShaderData.directionIntensity;
    sceneConstants.lightColorAmbient = lightShaderData.colorAmbient;
    sceneConstants.cameraPositionSpecularPower = {mCameraPositionM.x, mCameraPositionM.y, mCameraPositionM.z, 32.0F};

    mCommandList->SetGraphicsRoot32BitConstants(RenderRootParameter::SCENE,
                                                RenderRootParameter::SCENE_CONSTANTS_32BIT_COUNT,
                                                &sceneConstants,
                                                0);
    if (pass == RenderPass::World && primitiveTopology == MeshPrimitiveTopology::TriangleList && scene.GetTerrain())
    {
        mCommandList->SetPipelineState(mTerrainPipelineState.Get());
        scene.GetTerrain()->Render(*mCommandList.Get(), mCameraPositionM, mViewProjection);
        mCommandList->SetPipelineState(pipelineState);
    }
    scene.Render(*mCommandList.Get(), mDefaultTextureGpuHandle, primitiveTopology);

    if (includeText && primitiveTopology == MeshPrimitiveTopology::TriangleList)
    {
        QueueTextCommands(scene);
        QueueHealthBarCommands(scene);
        QueueDamageNumberCommands(scene);
    }
}

void Dx12Renderer::EndFrame()
{
    if (mD3d11On12Device == nullptr)
    {
        TransitionCurrentBackBufferToPresent();
        ThrowIfFailed(mCommandList->Close());
        ExecuteCurrentCommandList();
    }
    else
    {
        ThrowIfFailed(mCommandList->Close());
        ExecuteCurrentCommandList();
        DrawTextOverlay();
    }

    ThrowIfFailed(mSwapChain->Present(1, 0));
    MoveToNextFrame();
}

void Dx12Renderer::WaitForGpu()
{
    if (mCommandQueue == nullptr || mFence == nullptr || mFenceEvent == nullptr)
    {
        return;
    }

    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mFenceValues[mFrameIndex]));
    ThrowIfFailed(mFence->SetEventOnCompletion(mFenceValues[mFrameIndex], mFenceEvent));
    WaitForSingleObject(mFenceEvent, INFINITE);
    ++mFenceValues[mFrameIndex];
}

ID3D12Device& Dx12Renderer::GetDevice() const
{
    if (mDevice == nullptr)
    {
        throw std::runtime_error("DirectX 12 디바이스가 초기화되지 않았습니다.");
    }

    return *mDevice.Get();
}

ID3D12CommandQueue& Dx12Renderer::GetCommandQueue() const
{
    if (mCommandQueue == nullptr)
    {
        throw std::runtime_error("DirectX 12 커맨드 큐가 초기화되지 않았습니다.");
    }

    return *mCommandQueue.Get();
}

void Dx12Renderer::CreateDeviceResources()
{
    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
#endif

    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&mFactory)));

    ComPtr<IDXGIAdapter1> hardwareAdapter;
    GetHardwareAdapter(mFactory.Get(), &hardwareAdapter);

    if (hardwareAdapter != nullptr)
    {
        ThrowIfFailed(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice)));
    }
    else
    {
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(mFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
        ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice)));
    }

    D3D12_COMMAND_QUEUE_DESC queueDescription = {};
    queueDescription.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDescription.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(mDevice->CreateCommandQueue(&queueDescription, IID_PPV_ARGS(&mCommandQueue)));
}

void Dx12Renderer::CreateSwapChain()
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDescription = {};
    swapChainDescription.BufferCount = FRAME_COUNT;
    swapChainDescription.Width = mWidthPx;
    swapChainDescription.Height = mHeightPx;
    swapChainDescription.Format = RenderSettings::RENDER_TARGET_FORMAT;
    swapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDescription.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(mFactory->CreateSwapChainForHwnd(mCommandQueue.Get(),
                                                   mWindowHandle,
                                                   &swapChainDescription,
                                                   nullptr,
                                                   nullptr,
                                                   &swapChain));

    ThrowIfFailed(mFactory->MakeWindowAssociation(mWindowHandle, DXGI_MWA_NO_ALT_ENTER));
    ThrowIfFailed(swapChain.As(&mSwapChain));
    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
}

void Dx12Renderer::CreateRenderTargetViews()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDescription = {};
    rtvHeapDescription.NumDescriptors = FRAME_COUNT;
    rtvHeapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(mDevice->CreateDescriptorHeap(&rtvHeapDescription, IID_PPV_ARGS(&mRtvHeap)));

    mRtvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = mRtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT frame = 0; frame < FRAME_COUNT; ++frame)
    {
        ThrowIfFailed(mSwapChain->GetBuffer(frame, IID_PPV_ARGS(&mRenderTargets[frame])));
        mDevice->CreateRenderTargetView(mRenderTargets[frame].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += mRtvDescriptorSize;
    }
}

void Dx12Renderer::CreateDepthStencilView()
{
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDescription = {};
    dsvHeapDescription.NumDescriptors = 2;
    dsvHeapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(mDevice->CreateDescriptorHeap(&dsvHeapDescription, IID_PPV_ARGS(&mDsvHeap)));

    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resourceDescription = {};
    resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDescription.Width = mWidthPx;
    resourceDescription.Height = mHeightPx;
    resourceDescription.DepthOrArraySize = 1;
    resourceDescription.MipLevels = 1;
    resourceDescription.Format = DXGI_FORMAT_R32_TYPELESS;
    resourceDescription.SampleDesc.Count = 1;
    resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDescription.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = RenderSettings::DEPTH_STENCIL_FORMAT;
    clearValue.DepthStencil.Depth = 1.0F;
    clearValue.DepthStencil.Stencil = 0;

    ThrowIfFailed(mDevice->CreateCommittedResource(&heapProperties,
                                                   D3D12_HEAP_FLAG_NONE,
                                                   &resourceDescription,
                                                   D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                                   &clearValue,
                                                   IID_PPV_ARGS(&mDepthStencil)));

    ThrowIfFailed(mDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE,
                                                   &resourceDescription, D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                                   &clearValue, IID_PPV_ARGS(&mDetectionTargetDepth)));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDescription{};
    dsvDescription.Format = RenderSettings::DEPTH_STENCIL_FORMAT;
    dsvDescription.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    const auto worldDsv = mDsvHeap->GetCPUDescriptorHandleForHeapStart();
    mDetectionTargetDsv.ptr = worldDsv.ptr + mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    mDevice->CreateDepthStencilView(mDepthStencil.Get(), &dsvDescription, worldDsv);
    mDevice->CreateDepthStencilView(mDetectionTargetDepth.Get(), &dsvDescription, mDetectionTargetDsv);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDescription{};
    srvDescription.Format = DXGI_FORMAT_R32_FLOAT;
    srvDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDescription.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescription.Texture2D.MipLevels = 1;
    // 두 깊이 SRV를 연속 배치해 합성 패스에서 한 테이블로 바인딩한다.
    mDetectionDepthViews = CreateShaderResourceView(*mDepthStencil.Get(), srvDescription);
    (void)CreateShaderResourceView(*mDetectionTargetDepth.Get(), srvDescription);
}

void Dx12Renderer::CreateShaderResourceViewHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDescription = {};
    srvHeapDescription.NumDescriptors = RenderSettings::MAX_TEXTURE_DESCRIPTORS;
    srvHeapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(mDevice->CreateDescriptorHeap(&srvHeapDescription, IID_PPV_ARGS(&mSrvHeap)));

    mSrvDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void Dx12Renderer::CreateDefaultTexture()
{
    mDefaultTexture = Texture::CreateWhite1x1(*mDevice.Get(), *mCommandQueue.Get());
    mDefaultTextureGpuHandle = CreateTextureView(*mDefaultTexture);
}

D3D12_GPU_DESCRIPTOR_HANDLE Dx12Renderer::GetDefaultTextureGpuHandle() const noexcept
{
    return mDefaultTextureGpuHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE Dx12Renderer::CreateTextureView(const Texture& texture)
{
    ID3D12Resource& textureResource = texture.GetResource();
    const D3D12_RESOURCE_DESC resourceDescription = textureResource.GetDesc();
    const bool isCubeMap = texture.IsCubeMap();
    if (resourceDescription.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D ||
        resourceDescription.DepthOrArraySize != (isCubeMap ? 6 : 1) ||
        (isCubeMap && resourceDescription.Width != resourceDescription.Height))
    {
        throw std::runtime_error("SRV는 2D 텍스처 또는 단일 큐브맵만 지원합니다.");
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDescription = {};
    srvDescription.Format = resourceDescription.Format;
    srvDescription.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    if (isCubeMap)
    {
        srvDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
        srvDescription.TextureCube.MipLevels = resourceDescription.MipLevels;
    }
    else
    {
        srvDescription.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDescription.Texture2D.MipLevels = resourceDescription.MipLevels;
    }

    return CreateShaderResourceView(textureResource, srvDescription);
}

D3D12_GPU_DESCRIPTOR_HANDLE Dx12Renderer::CreateShaderResourceView(
    ID3D12Resource& resource, const D3D12_SHADER_RESOURCE_VIEW_DESC& description)
{
    if (mSrvDescriptorCount >= RenderSettings::MAX_TEXTURE_DESCRIPTORS)
    {
        throw std::runtime_error("SRV 디스크립터 힙이 가득 찼습니다.");
    }

    const UINT slotIndex = mSrvDescriptorCount++;
    auto cpuHandle = mSrvHeap->GetCPUDescriptorHandleForHeapStart();
    cpuHandle.ptr += static_cast<SIZE_T>(slotIndex) * mSrvDescriptorSize;
    mDevice->CreateShaderResourceView(&resource, &description, cpuHandle);

    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = mSrvHeap->GetGPUDescriptorHandleForHeapStart();
    gpuHandle.ptr += static_cast<UINT64>(slotIndex) * mSrvDescriptorSize;
    return gpuHandle;
}

void Dx12Renderer::CreateCommandObjects()
{
    for (UINT frame = 0; frame < FRAME_COUNT; ++frame)
    {
        ThrowIfFailed(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                     IID_PPV_ARGS(&mCommandAllocators[frame])));
    }

    ThrowIfFailed(mDevice->CreateCommandList(0,
                                            D3D12_COMMAND_LIST_TYPE_DIRECT,
                                            mCommandAllocators[mFrameIndex].Get(),
                                            nullptr,
                                            IID_PPV_ARGS(&mCommandList)));
    ThrowIfFailed(mCommandList->Close());
}

void Dx12Renderer::CreatePipelineObjects()
{
    // 씬 상수 28 + 오브젝트 상수 28 + 디스크립터 테이블 2 = 58 DWORD (한도 64).
    static_assert(RenderRootParameter::SCENE_CONSTANTS_32BIT_COUNT +
                  RenderRootParameter::OBJECT_CONSTANTS_32BIT_COUNT + 2 <= 64);
    D3D12_ROOT_PARAMETER rootParameters[4] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParameters[0].Constants.ShaderRegister = 0;
    rootParameters[0].Constants.RegisterSpace = 0;
    rootParameters[0].Constants.Num32BitValues = RenderRootParameter::SCENE_CONSTANTS_32BIT_COUNT;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParameters[1].Constants.ShaderRegister = 1;
    rootParameters[1].Constants.RegisterSpace = 0;
    rootParameters[1].Constants.Num32BitValues = RenderRootParameter::OBJECT_CONSTANTS_32BIT_COUNT;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_DESCRIPTOR_RANGE srvRanges[2] = {};
    for (unsigned int i = 0; i < 2; ++i)
    {
        srvRanges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        srvRanges[i].NumDescriptors = i == 0 ? 1 : LunarTerrainRenderer::TEXTURE_COUNT;
        srvRanges[i].BaseShaderRegister = i;
        srvRanges[i].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        rootParameters[2 + i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[2 + i].DescriptorTable.NumDescriptorRanges = 1;
        rootParameters[2 + i].DescriptorTable.pDescriptorRanges = &srvRanges[i];
        rootParameters[2 + i].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    }

    D3D12_STATIC_SAMPLER_DESC samplerDescription = {};
    samplerDescription.Filter = D3D12_FILTER_ANISOTROPIC;
    samplerDescription.MaxAnisotropy = 8;
    samplerDescription.MinLOD = 0.0F;
    samplerDescription.MaxLOD = D3D12_FLOAT32_MAX;
    samplerDescription.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDescription.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDescription.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDescription.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    samplerDescription.ShaderRegister = 0;
    samplerDescription.RegisterSpace = 0;
    samplerDescription.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC samplerDescriptions[2] = {samplerDescription, samplerDescription};
    samplerDescriptions[1].ShaderRegister = 1;
    samplerDescriptions[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
    samplerDescriptions[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
    samplerDescriptions[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_MIRROR;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDescription = {};
    rootSignatureDescription.NumParameters = _countof(rootParameters);
    rootSignatureDescription.pParameters = rootParameters;
    rootSignatureDescription.NumStaticSamplers = 2;
    rootSignatureDescription.pStaticSamplers = samplerDescriptions;
    rootSignatureDescription.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> errors;
    const HRESULT serializeResult = D3D12SerializeRootSignature(&rootSignatureDescription,
                                                                D3D_ROOT_SIGNATURE_VERSION_1,
                                                                &signature,
                                                                &errors);
    if (FAILED(serializeResult))
    {
        if (errors != nullptr)
        {
            throw std::runtime_error(static_cast<const char*>(errors->GetBufferPointer()));
        }

        ThrowIfFailed(serializeResult);
    }

    ThrowIfFailed(mDevice->CreateRootSignature(0,
                                               signature->GetBufferPointer(),
                                               signature->GetBufferSize(),
                                               IID_PPV_ARGS(&mRootSignature)));

    const std::filesystem::path litColorShaderPath = ShaderLibrary::GetLitColorShaderPath();
    ComPtr<ID3DBlob> vertexShader = ShaderCompiler::CompileFromFile(litColorShaderPath, "VSMain", "vs_5_0");
    ComPtr<ID3DBlob> pixelShader = ShaderCompiler::CompileFromFile(litColorShaderPath, "PSMain", "ps_5_0");

    D3D12_INPUT_ELEMENT_DESC inputElementDescriptions[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

    D3D12_RASTERIZER_DESC rasterizerDescription = {};
    rasterizerDescription.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizerDescription.CullMode = D3D12_CULL_MODE_NONE;
    rasterizerDescription.FrontCounterClockwise = FALSE;
    rasterizerDescription.DepthClipEnable = TRUE;

    D3D12_BLEND_DESC blendDescription = {};
    blendDescription.RenderTarget[0].BlendEnable = FALSE;
    blendDescription.RenderTarget[0].LogicOpEnable = FALSE;
    blendDescription.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
    blendDescription.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
    blendDescription.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blendDescription.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blendDescription.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    blendDescription.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blendDescription.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
    blendDescription.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_DEPTH_STENCIL_DESC depthStencilDescription = {};
    depthStencilDescription.DepthEnable = TRUE;
    depthStencilDescription.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencilDescription.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depthStencilDescription.StencilEnable = FALSE;
    depthStencilDescription.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    depthStencilDescription.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    depthStencilDescription.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
    depthStencilDescription.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    depthStencilDescription.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
    depthStencilDescription.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    depthStencilDescription.BackFace = depthStencilDescription.FrontFace;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDescription = {};
    pipelineStateDescription.InputLayout = {inputElementDescriptions, _countof(inputElementDescriptions)};
    pipelineStateDescription.pRootSignature = mRootSignature.Get();
    pipelineStateDescription.VS = {vertexShader->GetBufferPointer(), vertexShader->GetBufferSize()};
    pipelineStateDescription.PS = {pixelShader->GetBufferPointer(), pixelShader->GetBufferSize()};
    pipelineStateDescription.RasterizerState = rasterizerDescription;
    pipelineStateDescription.BlendState = blendDescription;
    pipelineStateDescription.DepthStencilState = depthStencilDescription;
    pipelineStateDescription.SampleMask = UINT_MAX;
    pipelineStateDescription.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateDescription.NumRenderTargets = 1;
    pipelineStateDescription.RTVFormats[0] = RenderSettings::RENDER_TARGET_FORMAT;
    pipelineStateDescription.DSVFormat = RenderSettings::DEPTH_STENCIL_FORMAT;
    pipelineStateDescription.SampleDesc.Count = 1;

    ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&pipelineStateDescription, IID_PPV_ARGS(&mPipelineState)));

    constexpr wchar_t skyboxShaderPath[] = L"Assets/Shaders/Skybox.hlsl";
    const auto skyboxVertexShader = ShaderCompiler::CompileFromFile(skyboxShaderPath, "VSMain", "vs_5_0");
    const auto skyboxPixelShader = ShaderCompiler::CompileFromFile(skyboxShaderPath, "PSMain", "ps_5_0");
    auto skyboxPipelineDescription = pipelineStateDescription;
    skyboxPipelineDescription.InputLayout = {};
    skyboxPipelineDescription.VS = {skyboxVertexShader->GetBufferPointer(), skyboxVertexShader->GetBufferSize()};
    skyboxPipelineDescription.PS = {skyboxPixelShader->GetBufferPointer(), skyboxPixelShader->GetBufferSize()};
    skyboxPipelineDescription.DepthStencilState.DepthEnable = FALSE;
    skyboxPipelineDescription.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&skyboxPipelineDescription, IID_PPV_ARGS(&mSkyboxPipelineState)));

    const auto terrainPixelShader = ShaderCompiler::CompileFromFile(L"Assets/Shaders/LunarTerrain.hlsl", "PSTerrain", "ps_5_0");
    CreateDetectionPulsePipelines(pipelineStateDescription);
    auto terrainPipelineDescription = pipelineStateDescription;
    terrainPipelineDescription.PS = {terrainPixelShader->GetBufferPointer(), terrainPixelShader->GetBufferSize()};
    ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&terrainPipelineDescription, IID_PPV_ARGS(&mTerrainPipelineState)));

    D3D12_DEPTH_STENCIL_DESC lineDepthStencilDescription = depthStencilDescription;
    lineDepthStencilDescription.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    lineDepthStencilDescription.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

    D3D12_RASTERIZER_DESC lineRasterizerDescription = rasterizerDescription;
    lineRasterizerDescription.AntialiasedLineEnable = TRUE;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC linePipelineStateDescription = pipelineStateDescription;
    linePipelineStateDescription.RasterizerState = lineRasterizerDescription;
    linePipelineStateDescription.DepthStencilState = lineDepthStencilDescription;
    linePipelineStateDescription.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&linePipelineStateDescription, IID_PPV_ARGS(&mLinePipelineState)));

    D3D12_BLEND_DESC overlayBlendDescription = blendDescription;
    overlayBlendDescription.RenderTarget[0].BlendEnable = TRUE;
    overlayBlendDescription.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    overlayBlendDescription.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    overlayBlendDescription.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    overlayBlendDescription.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    overlayBlendDescription.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    overlayBlendDescription.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

    D3D12_DEPTH_STENCIL_DESC overlayDepthStencilDescription = depthStencilDescription;
    overlayDepthStencilDescription.DepthEnable = FALSE;
    overlayDepthStencilDescription.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

    pipelineStateDescription.BlendState = overlayBlendDescription;
    pipelineStateDescription.DepthStencilState = overlayDepthStencilDescription;
    ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&pipelineStateDescription, IID_PPV_ARGS(&mOverlayPipelineState)));

    mViewport.TopLeftX = 0.0F;
    mViewport.TopLeftY = 0.0F;
    mViewport.Width = static_cast<float>(mWidthPx);
    mViewport.Height = static_cast<float>(mHeightPx);
    mViewport.MinDepth = 0.0F;
    mViewport.MaxDepth = 1.0F;

    mScissorRect = {0, 0, static_cast<LONG>(mWidthPx), static_cast<LONG>(mHeightPx)};
}

void Dx12Renderer::CreateDetectionPulsePipelines(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& worldPipeline)
{
    auto depthPipeline = worldPipeline;
    depthPipeline.NumRenderTargets = 0;
    depthPipeline.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
    depthPipeline.PS = {};
    ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&depthPipeline, IID_PPV_ARGS(&mDetectionDepthPipeline)));

    D3D12_DESCRIPTOR_RANGE depthRange{};
    depthRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    depthRange.NumDescriptors = 2;
    D3D12_ROOT_PARAMETER parameters[2]{};
    parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    parameters[0].DescriptorTable = {1, &depthRange};
    parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    parameters[1].Constants.Num32BitValues = 4;
    parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    D3D12_ROOT_SIGNATURE_DESC signatureDescription{};
    signatureDescription.NumParameters = _countof(parameters);
    signatureDescription.pParameters = parameters;

    ComPtr<ID3DBlob> signature;
    ThrowIfFailed(D3D12SerializeRootSignature(&signatureDescription, D3D_ROOT_SIGNATURE_VERSION_1,
                                             &signature, nullptr));
    ThrowIfFailed(mDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                               IID_PPV_ARGS(&mDetectionCompositeRootSignature)));

    constexpr wchar_t SHADER_PATH[] = L"Assets/Shaders/DetectionPulse.hlsl";
    const auto vertexShader = ShaderCompiler::CompileFromFile(SHADER_PATH, "VSMain", "vs_5_0");
    const auto pixelShader = ShaderCompiler::CompileFromFile(SHADER_PATH, "PSMain", "ps_5_0");
    auto compositePipeline = worldPipeline;
    compositePipeline.pRootSignature = mDetectionCompositeRootSignature.Get();
    compositePipeline.InputLayout = {};
    compositePipeline.VS = {vertexShader->GetBufferPointer(), vertexShader->GetBufferSize()};
    compositePipeline.PS = {pixelShader->GetBufferPointer(), pixelShader->GetBufferSize()};
    compositePipeline.DSVFormat = DXGI_FORMAT_UNKNOWN;
    compositePipeline.DepthStencilState.DepthEnable = FALSE;
    compositePipeline.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    auto& blend = compositePipeline.BlendState.RenderTarget[0];
    blend.BlendEnable = TRUE;
    blend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blend.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blend.SrcBlendAlpha = D3D12_BLEND_ZERO;
    blend.DestBlendAlpha = D3D12_BLEND_ONE;
    ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&compositePipeline,
                                                       IID_PPV_ARGS(&mDetectionCompositePipeline)));
}

void Dx12Renderer::CreateTextOverlayResources()
{
    constexpr UINT d3d11DeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    constexpr D3D_FEATURE_LEVEL featureLevels[] = {D3D_FEATURE_LEVEL_11_0};
    IUnknown* commandQueues[] = {mCommandQueue.Get()};

    ThrowIfFailed(D3D11On12CreateDevice(mDevice.Get(),
                                        d3d11DeviceFlags,
                                        featureLevels,
                                        _countof(featureLevels),
                                        commandQueues,
                                        _countof(commandQueues),
                                        0,
                                        &mD3d11Device,
                                        &mD3d11Context,
                                        nullptr));
    ThrowIfFailed(mD3d11Device.As(&mD3d11On12Device));

    D2D1_FACTORY_OPTIONS factoryOptions = {};
    ThrowIfFailed(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                                    __uuidof(ID2D1Factory1),
                                    &factoryOptions,
                                    reinterpret_cast<void**>(mD2dFactory.GetAddressOf())));

    ComPtr<IDXGIDevice> dxgiDevice;
    ThrowIfFailed(mD3d11Device.As(&dxgiDevice));
    ThrowIfFailed(mD2dFactory->CreateDevice(dxgiDevice.Get(), &mD2dDevice));
    ThrowIfFailed(mD2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &mD2dContext));

    ThrowIfFailed(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                                      __uuidof(IDWriteFactory3),
                                      reinterpret_cast<IUnknown**>(mDWriteFactory.GetAddressOf())));
    LoadUiFontCollection();

    // HUD 이미지(PNG) 디코딩용 WIC. 실패해도 게임은 돌아가고 이미지만 안 그려짐.
    // COM 초기화는 wWinMain이 앱 전체 수명 동안 맡음(Main.cpp).
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&mWicFactory))))
    {
        mWicFactory.Reset();
        OutputDebugStringW(L"[UI] WIC 팩토리 생성 실패: HUD 이미지를 그리지 않습니다.\n");
    }

    D3D11_RESOURCE_FLAGS d3d11Flags = {};
    d3d11Flags.BindFlags = D3D11_BIND_RENDER_TARGET;

    const D2D1_BITMAP_PROPERTIES1 bitmapProperties =
        D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
                                D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
                                D2D_DEFAULT_DPI,
                                D2D_DEFAULT_DPI);

    for (UINT frame = 0; frame < FRAME_COUNT; ++frame)
    {
        ThrowIfFailed(mD3d11On12Device->CreateWrappedResource(mRenderTargets[frame].Get(),
                                                              &d3d11Flags,
                                                              D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                              D3D12_RESOURCE_STATE_PRESENT,
                                                              IID_PPV_ARGS(&mWrappedBackBuffers[frame])));

        ComPtr<IDXGISurface> surface;
        ThrowIfFailed(mWrappedBackBuffers[frame].As(&surface));
        ThrowIfFailed(mD2dContext->CreateBitmapFromDxgiSurface(surface.Get(),
                                                               &bitmapProperties,
                                                               &mD2dRenderTargets[frame]));
    }
}

void Dx12Renderer::LoadUiFontCollection()
{
    if (mDWriteFactory == nullptr)
    {
        return;
    }

    const std::filesystem::path fontDirectory = Kimgane::Shared::IO::ResolveAssetPath(UI_FONT_DIRECTORY_PATH);
    if (fontDirectory.empty() || !std::filesystem::is_directory(fontDirectory))
    {
        return;
    }

    ComPtr<IDWriteFontSetBuilder> fontSetBuilder;
    if (FAILED(mDWriteFactory->CreateFontSetBuilder(&fontSetBuilder)))
    {
        return;
    }

    bool addedFont = false;
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(fontDirectory))
    {
        if (!entry.is_regular_file() || !IsTrueTypeFontFile(entry.path()))
        {
            continue;
        }

        ComPtr<IDWriteFontFaceReference> fontFaceReference;
        if (SUCCEEDED(mDWriteFactory->CreateFontFaceReference(entry.path().c_str(),
                                                             nullptr,
                                                             0,
                                                             DWRITE_FONT_SIMULATIONS_NONE,
                                                             &fontFaceReference)) &&
            SUCCEEDED(fontSetBuilder->AddFontFaceReference(fontFaceReference.Get())))
        {
            addedFont = true;
        }
    }

    if (!addedFont)
    {
        return;
    }

    ComPtr<IDWriteFontSet> fontSet;
    ComPtr<IDWriteFontCollection1> fontCollection;
    if (SUCCEEDED(fontSetBuilder->CreateFontSet(&fontSet)) &&
        SUCCEEDED(mDWriteFactory->CreateFontCollectionFromFontSet(fontSet.Get(), &fontCollection)))
    {
        mUiFontCollection = fontCollection;
        mUiFontFamilyName = UI_FONT_FAMILY_NAME;
    }
}

void Dx12Renderer::QueueTextCommands(const Scene& scene)
{
    for (const auto& textLabel : scene.GetTextLabels())
    {
        if (!textLabel || textLabel->GetText().empty() || !textLabel->GetPanel().IsActive())
        {
            continue;
        }

        const DirectX::XMFLOAT4 rectPx = BuildTextRectPx(scene.GetRenderWorldMatrix(textLabel->GetPanel()), *textLabel);
        if (rectPx.z <= rectPx.x || rectPx.w <= rectPx.y)
        {
            continue;
        }

        TextDrawCommand command = {};
        command.text = textLabel->GetText();
        command.rectPx = rectPx;
        command.colorLinear = textLabel->GetColorLinear();
        command.fontSizeDip = textLabel->GetFontSizeDip();
        command.horizontalAlignment = static_cast<int>(textLabel->GetHorizontalAlignment());
        command.verticalAlignment = static_cast<int>(textLabel->GetVerticalAlignment());
        mTextDrawCommands.push_back(std::move(command));
    }
}

DirectX::XMFLOAT4 Dx12Renderer::BuildTextRectPx(const DirectX::XMFLOAT4X4& renderWorld,
                                                const TextLabel& textLabel) const noexcept
{
    const DirectX::XMFLOAT2& insetRatio = textLabel.GetInsetRatio();
    const float leftLocal = -0.5F + insetRatio.x;
    const float rightLocal = 0.5F - insetRatio.x;
    const float topLocal = 0.5F - insetRatio.y;
    const float bottomLocal = -0.5F + insetRatio.y;

    const DirectX::XMVECTOR localCorners[] = {
        DirectX::XMVectorSet(leftLocal, topLocal, 0.0F, 1.0F),
        DirectX::XMVectorSet(rightLocal, topLocal, 0.0F, 1.0F),
        DirectX::XMVectorSet(leftLocal, bottomLocal, 0.0F, 1.0F),
        DirectX::XMVectorSet(rightLocal, bottomLocal, 0.0F, 1.0F)};

    const DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&renderWorld);
    const DirectX::XMMATRIX viewProjection = DirectX::XMLoadFloat4x4(&mViewProjection);

    float leftPx = static_cast<float>(mWidthPx);
    float topPx = static_cast<float>(mHeightPx);
    float rightPx = 0.0F;
    float bottomPx = 0.0F;

    for (const DirectX::XMVECTOR& localCorner : localCorners)
    {
        const DirectX::XMVECTOR worldCorner = DirectX::XMVector3Transform(localCorner, world);
        const DirectX::XMVECTOR clipCorner = DirectX::XMVector4Transform(worldCorner, viewProjection);
        const float projectedW = DirectX::XMVectorGetW(clipCorner);
        // w가 음수면 그 모서리는 카메라 뒤에 있다는 뜻. fabs()만 보면 음수인 채로 통과해서
        // 음수로 나눌 때 부호가 뒤집혀 카메라 뒤 오브젝트가 화면 안으로 잘못 투영될 수 있음.
        if (projectedW <= MIN_PROJECTED_W)
        {
            return {};
        }

        const float ndcX = DirectX::XMVectorGetX(clipCorner) / projectedW;
        const float ndcY = DirectX::XMVectorGetY(clipCorner) / projectedW;
        const float pixelX = (ndcX * 0.5F + 0.5F) * static_cast<float>(mWidthPx);
        const float pixelY = (0.5F - ndcY * 0.5F) * static_cast<float>(mHeightPx);

        leftPx = std::min(leftPx, pixelX);
        topPx = std::min(topPx, pixelY);
        rightPx = std::max(rightPx, pixelX);
        bottomPx = std::max(bottomPx, pixelY);
    }

    return {leftPx, topPx, rightPx, bottomPx};
}

bool Dx12Renderer::TryProjectWorldToScreenPx(const DirectX::XMFLOAT3& worldPositionM,
                                             DirectX::XMFLOAT2& outPx) const noexcept
{
    const DirectX::XMVECTOR worldPoint = DirectX::XMVectorSet(worldPositionM.x, worldPositionM.y, worldPositionM.z, 1.0F);
    const DirectX::XMMATRIX viewProjection = DirectX::XMLoadFloat4x4(&mViewProjection);
    const DirectX::XMVECTOR clipPoint = DirectX::XMVector4Transform(worldPoint, viewProjection);

    const float projectedW = DirectX::XMVectorGetW(clipPoint);
    // w가 음수면(또는 0에 가까우면) 점이 카메라 뒤에 있다는 뜻. 이걸 걸러내지 않으면
    // 음수로 나누면서 부호가 뒤집혀, 카메라 뒤에 있는 NPC가 엉뚱하게 화면 안 좌표로
    // 투영돼서 "허공에 체력바만 떠 있는" 것처럼 보이는 버그가 생김.
    if (projectedW <= MIN_PROJECTED_W)
    {
        return false;
    }

    const float ndcZ = DirectX::XMVectorGetZ(clipPoint) / projectedW;
    if (ndcZ < 0.0F || ndcZ > 1.0F)
    {
        return false;
    }

    const float ndcX = DirectX::XMVectorGetX(clipPoint) / projectedW;
    const float ndcY = DirectX::XMVectorGetY(clipPoint) / projectedW;
    outPx.x = (ndcX * 0.5F + 0.5F) * static_cast<float>(mWidthPx);
    outPx.y = (0.5F - ndcY * 0.5F) * static_cast<float>(mHeightPx);
    return true;
}

bool Dx12Renderer::IsLineOfSightBlocked(const DirectX::XMFLOAT3& fromM,
                                        const DirectX::XMFLOAT3& toM,
                                        const Scene& scene) const noexcept
{
    const DirectX::XMVECTOR deltaVec = DirectX::XMVectorSubtract(DirectX::XMLoadFloat3(&toM), DirectX::XMLoadFloat3(&fromM));
    const float distanceM = DirectX::XMVectorGetX(DirectX::XMVector3Length(deltaVec));
    if (distanceM <= MIN_PROJECTED_W)
    {
        return false;
    }

    const auto* world = scene.GetCollisionWorld();
    if (world == nullptr)
    {
        return false;
    }

    DirectX::XMFLOAT3 directionM = {};
    DirectX::XMStoreFloat3(&directionM, DirectX::XMVectorScale(deltaVec, 1.0F / distanceM));

    // 지형·집·정화기·상자 등 Shared 충돌 월드의 정적 body 중 하나라도 시야를 가리면 숨김.
    namespace SharedPhysics = Kimgane::Shared::Physics;
    namespace SharedRaycast = Kimgane::Shared::Physics::RaycastQueries;
    const float maxDistanceM = distanceM - HEALTH_BAR_OCCLUSION_MARGIN_M;
    if (maxDistanceM <= 0.0F)
    {
        return false;
    }
    const SharedRaycast::Ray ray{{fromM.x, fromM.y, fromM.z}, {directionM.x, directionM.y, directionM.z}, maxDistanceM};
    SharedRaycast::RaycastHit hit{};
    return SharedRaycast::RaycastClosest(ray, world->GetBodies(), SharedPhysics::INVALID_OBJECT_ID, hit) &&
           hit.distanceM < maxDistanceM;
}

void Dx12Renderer::QueueHealthBarCommands(const Scene& scene)
{
    std::vector<WorldHealthBarView> healthBars;
    scene.CollectWorldHealthBars(healthBars);
    for (const WorldHealthBarView& view : healthBars)
    {
        const WorldHealthBar* healthBar = view.healthBar;
        if (healthBar == nullptr)
        {
            continue;
        }

        const DirectX::XMFLOAT3 baseWorldM = view.footPositionM;

        DirectX::XMFLOAT3 visibilityWorldM = baseWorldM;
        visibilityWorldM.y += healthBar->GetVisibilityHeightOffsetM();

        // 몸통 높이 기준으로 가려짐을 판단함. 바를 그리는 위치(머리 위)로 판단하면
        // 언덕이 몸은 가려도 둥실 뜬 바 위치까지는 못 가려서 몸은 안 보이는데 바만 보이게 됨.
        if (IsLineOfSightBlocked(mCameraPositionM, visibilityWorldM, scene))
        {
            continue;
        }

        DirectX::XMFLOAT3 anchorWorldM = baseWorldM;
        anchorWorldM.y += healthBar->GetDisplayHeightOffsetM();

        DirectX::XMFLOAT2 anchorPx = {};
        if (!TryProjectWorldToScreenPx(anchorWorldM, anchorPx))
        {
            continue;
        }

        HealthBarDrawCommand command = {};
        command.centerPx = anchorPx;
        command.hpRatio = healthBar->GetHpRatio();
        mHealthBarDrawCommands.push_back(command);
    }
}

void Dx12Renderer::QueueDamageNumberCommands(const Scene& scene)
{
    for (const DamageNumber& damageNumber : scene.GetDamageNumbers())
    {
        if (!damageNumber.IsActive() || damageNumber.GetText().empty())
        {
            continue;
        }
        const DamageNumber* number = &damageNumber;

        const DirectX::XMFLOAT3 baseWorldM = number->GetFootPositionM();
        DirectX::XMFLOAT3 anchorWorldM = baseWorldM;
        anchorWorldM.y += number->GetDisplayHeightOffsetM();
        DirectX::XMFLOAT2 anchorPx = {};
        if (!TryProjectWorldToScreenPx(anchorWorldM, anchorPx))
        {
            continue;
        }
        anchorPx.x += number->GetHorizontalOffsetPx();

        // 위치만 월드에서 투영하고 글자 크기는 픽셀 기준으로 유지해 거리/카메라 회전에도 읽히게 합니다.
        TextDrawCommand command = {};
        command.text = number->GetText();
        const float halfWidthPx = RenderSettings::DAMAGE_NUMBER_WIDTH_PX * 0.5F;
        const float halfHeightPx = RenderSettings::DAMAGE_NUMBER_HEIGHT_PX * 0.5F;
        command.rectPx = {anchorPx.x - halfWidthPx, anchorPx.y - halfHeightPx,
                          anchorPx.x + halfWidthPx, anchorPx.y + halfHeightPx};
        if (command.rectPx.z <= 0.0F || command.rectPx.w <= 0.0F ||
            command.rectPx.x >= static_cast<float>(mWidthPx) || command.rectPx.y >= static_cast<float>(mHeightPx))
        {
            continue;
        }

        DirectX::XMFLOAT3 visibilityWorldM = baseWorldM;
        visibilityWorldM.y += number->GetVisibilityHeightOffsetM();
        if (IsLineOfSightBlocked(mCameraPositionM, visibilityWorldM, scene))
        {
            continue;
        }

        command.fontSizeDip = RenderSettings::DAMAGE_NUMBER_FONT_SIZE_DIP;
        command.colorLinear = RenderSettings::DAMAGE_NUMBER_COLOR;
        command.colorLinear.w *= number->GetOpacity();
        command.drawShadow = true;
        command.shadowOffsetPx = RenderSettings::DAMAGE_NUMBER_SHADOW_OFFSET_PX;
        mTextDrawCommands.push_back(std::move(command));
    }
}

ComPtr<IDWriteTextFormat> Dx12Renderer::CreateTextFormat(float fontSizeDip,
                                                         int horizontalAlignment,
                                                         int verticalAlignment) const
{
    ComPtr<IDWriteTextFormat> textFormat;
    HRESULT result = mDWriteFactory->CreateTextFormat(mUiFontFamilyName.c_str(),
                                                      mUiFontCollection.Get(),
                                                      DWRITE_FONT_WEIGHT_SEMI_BOLD,
                                                      DWRITE_FONT_STYLE_NORMAL,
                                                      DWRITE_FONT_STRETCH_NORMAL,
                                                      std::max(fontSizeDip, 1.0F),
                                                      L"ko-KR",
                                                      &textFormat);

    if (FAILED(result))
    {
        result = mDWriteFactory->CreateTextFormat(L"Malgun Gothic",
                                                  nullptr,
                                                  DWRITE_FONT_WEIGHT_SEMI_BOLD,
                                                  DWRITE_FONT_STYLE_NORMAL,
                                                  DWRITE_FONT_STRETCH_NORMAL,
                                                  std::max(fontSizeDip, 1.0F),
                                                  L"ko-KR",
                                                  &textFormat);
    }

    ThrowIfFailed(result);
    ThrowIfFailed(textFormat->SetTextAlignment(ToDWriteTextAlignment(horizontalAlignment)));
    ThrowIfFailed(textFormat->SetParagraphAlignment(ToDWriteParagraphAlignment(verticalAlignment)));
    ThrowIfFailed(textFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP));
    return textFormat;
}

void Dx12Renderer::DrawTextOverlay()
{
    ID3D11Resource* wrappedBackBuffer = mWrappedBackBuffers[mFrameIndex].Get();
    mD3d11On12Device->AcquireWrappedResources(&wrappedBackBuffer, 1);

    HRESULT drawResult = S_OK;
    if (!mTextDrawCommands.empty() || !mHealthBarDrawCommands.empty() || !mUiDrawCommands.empty() ||
        mCrosshairVisible)
    {
        mD2dContext->SetTarget(mD2dRenderTargets[mFrameIndex].Get());
        mD2dContext->BeginDraw();
        mD2dContext->SetTransform(D2D1::Matrix3x2F::Identity());

        DrawHealthBars();

        for (const TextDrawCommand& command : mTextDrawCommands)
        {
            DrawTextCommand(command);
        }

        if (mCrosshairVisible)
        {
            DrawCrosshair();
        }

        // HUD는 월드 위 표시(체력바·데미지 숫자·조준선)보다 위에 그림
        DrawUiCommands();

        drawResult = mD2dContext->EndDraw();
    }

    mD3d11On12Device->ReleaseWrappedResources(&wrappedBackBuffer, 1);
    mD3d11Context->Flush();
    ThrowIfFailed(drawResult);
}

IDWriteTextFormat* Dx12Renderer::FindOrCreateTextFormat(float fontSizeDip,
                                                        int horizontalAlignment,
                                                        int verticalAlignment)
{
    const auto key = std::make_tuple(fontSizeDip, horizontalAlignment, verticalAlignment);
    auto found = mTextFormats.find(key);
    if (found == mTextFormats.end())
    {
        found = mTextFormats.emplace(key, CreateTextFormat(fontSizeDip, horizontalAlignment, verticalAlignment)).first;
    }
    return found->second.Get();
}

void Dx12Renderer::DrawTextCommand(const TextDrawCommand& command)
{
    // 브러시는 하나를 만들어 두고 색만 바꿔 씀
    if (mTextBrush == nullptr)
    {
        ThrowIfFailed(mD2dContext->CreateSolidColorBrush(D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F), &mTextBrush));
    }

    IDWriteTextFormat* textFormat =
        FindOrCreateTextFormat(command.fontSizeDip, command.horizontalAlignment, command.verticalAlignment);
    const D2D1_RECT_F layoutRect = D2D1::RectF(command.rectPx.x, command.rectPx.y, command.rectPx.z, command.rectPx.w);

    if (command.drawShadow)
    {
        mTextBrush->SetColor(D2D1::ColorF(0.0F, 0.0F, 0.0F, command.colorLinear.w));
        const float offsetPx = command.shadowOffsetPx;
        const D2D1_RECT_F shadowRect = D2D1::RectF(command.rectPx.x + offsetPx, command.rectPx.y + offsetPx,
                                                  command.rectPx.z + offsetPx, command.rectPx.w + offsetPx);
        mD2dContext->DrawText(command.text.c_str(), static_cast<UINT32>(command.text.size()), textFormat,
                              shadowRect, mTextBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP, DWRITE_MEASURING_MODE_NATURAL);
    }

    mTextBrush->SetColor(ToD2DColor(command.colorLinear));
    mD2dContext->DrawText(command.text.c_str(),
                          static_cast<UINT32>(command.text.size()),
                          textFormat,
                          layoutRect,
                          mTextBrush.Get(),
                          D2D1_DRAW_TEXT_OPTIONS_CLIP,
                          DWRITE_MEASURING_MODE_NATURAL);
}

void Dx12Renderer::DrawUiCommands()
{
    if (mUiDrawCommands.empty())
    {
        return;
    }

    // 색만 바꿔가며 쓰는 브러시 하나로 사각형을 그림
    ComPtr<ID2D1SolidColorBrush> brush;
    ThrowIfFailed(mD2dContext->CreateSolidColorBrush(D2D1::ColorF(1.0F, 1.0F, 1.0F, 1.0F), &brush));

    for (const UiDrawCommand& command : mUiDrawCommands)
    {
        const D2D1_RECT_F rect = D2D1::RectF(command.rectPx.x, command.rectPx.y, command.rectPx.z, command.rectPx.w);
        switch (command.kind)
        {
        case UiDrawKind::FillRect:
            brush->SetColor(ToD2DColor(command.colorLinear));
            mD2dContext->FillRectangle(rect, brush.Get());
            break;
        case UiDrawKind::StrokeRect:
            brush->SetColor(ToD2DColor(command.colorLinear));
            mD2dContext->DrawRectangle(rect, brush.Get(), command.thicknessPx);
            break;
        case UiDrawKind::Text:
            DrawTextCommand(command.text);
            break;
        case UiDrawKind::Image:
            if (ID2D1Bitmap1* bitmap = FindOrLoadUiBitmap(command.imagePath))
            {
                mD2dContext->DrawBitmap(bitmap, &rect, std::clamp(command.colorLinear.w, 0.0F, 1.0F),
                                        D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC, nullptr, nullptr);
            }
            break;
        }
    }
}

void Dx12Renderer::DrawCrosshair()
{
    const float centerX = static_cast<float>(mWidthPx) * 0.5F;
    const float centerY = static_cast<float>(mHeightPx) * 0.5F;

    ComPtr<ID2D1SolidColorBrush> brush;
    ThrowIfFailed(mD2dContext->CreateSolidColorBrush(ToD2DColor(RenderSettings::CROSSHAIR_COLOR), &brush));

    mD2dContext->DrawLine({centerX - RenderSettings::CROSSHAIR_SIZE_PX, centerY},
                          {centerX + RenderSettings::CROSSHAIR_SIZE_PX, centerY}, brush.Get(),
                          RenderSettings::CROSSHAIR_THICKNESS_PX);
    mD2dContext->DrawLine({centerX, centerY - RenderSettings::CROSSHAIR_SIZE_PX},
                          {centerX, centerY + RenderSettings::CROSSHAIR_SIZE_PX}, brush.Get(),
                          RenderSettings::CROSSHAIR_THICKNESS_PX);
}

void Dx12Renderer::DrawHealthBars()
{
    if (mHealthBarDrawCommands.empty())
    {
        return;
    }

    ComPtr<ID2D1SolidColorBrush> backgroundBrush;
    ThrowIfFailed(mD2dContext->CreateSolidColorBrush(ToD2DColor(RenderSettings::HEALTH_BAR_BACKGROUND_COLOR),
                                                     &backgroundBrush));
    ComPtr<ID2D1SolidColorBrush> fillBrush;
    ThrowIfFailed(mD2dContext->CreateSolidColorBrush(ToD2DColor(RenderSettings::HEALTH_BAR_FILL_COLOR), &fillBrush));

    const float halfWidthPx = RenderSettings::HEALTH_BAR_WIDTH_PX * 0.5F;
    const float halfHeightPx = RenderSettings::HEALTH_BAR_HEIGHT_PX * 0.5F;

    for (const HealthBarDrawCommand& command : mHealthBarDrawCommands)
    {
        const D2D1_RECT_F backgroundRect = D2D1::RectF(command.centerPx.x - halfWidthPx,
                                                       command.centerPx.y - halfHeightPx,
                                                       command.centerPx.x + halfWidthPx,
                                                       command.centerPx.y + halfHeightPx);
        mD2dContext->FillRectangle(backgroundRect, backgroundBrush.Get());

        if (command.hpRatio > 0.0F)
        {
            const float fillWidthPx = RenderSettings::HEALTH_BAR_WIDTH_PX * command.hpRatio;
            const D2D1_RECT_F fillRect = D2D1::RectF(command.centerPx.x - halfWidthPx,
                                                     command.centerPx.y - halfHeightPx,
                                                     command.centerPx.x - halfWidthPx + fillWidthPx,
                                                     command.centerPx.y + halfHeightPx);
            mD2dContext->FillRectangle(fillRect, fillBrush.Get());
        }
    }
}

ID2D1Bitmap1* Dx12Renderer::FindOrLoadUiBitmap(const std::wstring& imagePath)
{
    auto it = mUiBitmaps.find(imagePath);
    if (it == mUiBitmaps.end())
    {
        it = mUiBitmaps.emplace(imagePath, LoadUiBitmap(imagePath)).first;
    }
    return it->second.Get();
}

ComPtr<ID2D1Bitmap1> Dx12Renderer::LoadUiBitmap(const std::wstring& imagePath) const
{
    if (mWicFactory == nullptr || mD2dContext == nullptr)
    {
        return nullptr;
    }

    const std::filesystem::path resolvedPath = Kimgane::Shared::IO::ResolveAssetPath(imagePath);
    if (resolvedPath.empty())
    {
        OutputDebugStringW((L"[UI] 이미지 파일을 찾지 못함: " + imagePath + L"\n").c_str());
        return nullptr;
    }

    // 파일 → 디코더 → 첫 프레임 → (필요하면 축소) → D2D가 쓰는 premultiplied BGRA로 변환 → D2D 비트맵
    ComPtr<IWICBitmapDecoder> decoder;
    ComPtr<IWICBitmapFrameDecode> frame;
    if (FAILED(mWicFactory->CreateDecoderFromFilename(resolvedPath.c_str(), nullptr, GENERIC_READ,
                                                      WICDecodeMetadataCacheOnLoad, &decoder)) ||
        FAILED(decoder->GetFrame(0, &frame)))
    {
        OutputDebugStringW((L"[UI] 이미지 디코딩 실패: " + imagePath + L"\n").c_str());
        return nullptr;
    }

    ComPtr<IWICBitmapSource> source = frame;
    UINT widthPx = 0;
    UINT heightPx = 0;
    if (SUCCEEDED(frame->GetSize(&widthPx, &heightPx)) && std::max(widthPx, heightPx) > UI_IMAGE_MAX_SIZE_PX)
    {
        const float scale = static_cast<float>(UI_IMAGE_MAX_SIZE_PX) / static_cast<float>(std::max(widthPx, heightPx));
        const UINT scaledWidthPx = std::max(1U, static_cast<UINT>(static_cast<float>(widthPx) * scale));
        const UINT scaledHeightPx = std::max(1U, static_cast<UINT>(static_cast<float>(heightPx) * scale));

        ComPtr<IWICBitmapScaler> scaler;
        // Fant: 여러 픽셀을 평균 내는 축소 방식이라 크게 줄여도 지글거림이 적음
        if (SUCCEEDED(mWicFactory->CreateBitmapScaler(&scaler)) &&
            SUCCEEDED(scaler->Initialize(frame.Get(), scaledWidthPx, scaledHeightPx, WICBitmapInterpolationModeFant)))
        {
            source = scaler;
        }
    }

    ComPtr<IWICFormatConverter> converter;
    if (FAILED(mWicFactory->CreateFormatConverter(&converter)) ||
        FAILED(converter->Initialize(source.Get(), GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr,
                                     0.0, WICBitmapPaletteTypeMedianCut)))
    {
        OutputDebugStringW((L"[UI] 이미지 픽셀 포맷 변환 실패: " + imagePath + L"\n").c_str());
        return nullptr;
    }

    ComPtr<ID2D1Bitmap1> bitmap;
    if (FAILED(mD2dContext->CreateBitmapFromWicBitmap(converter.Get(), nullptr, &bitmap)))
    {
        OutputDebugStringW((L"[UI] D2D 비트맵 생성 실패: " + imagePath + L"\n").c_str());
        return nullptr;
    }
    return bitmap;
}

void Dx12Renderer::TransitionCurrentBackBufferToPresent()
{
    D3D12_RESOURCE_BARRIER barrierToPresent = {};
    barrierToPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierToPresent.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrierToPresent.Transition.pResource = mRenderTargets[mFrameIndex].Get();
    barrierToPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrierToPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrierToPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    mCommandList->ResourceBarrier(1, &barrierToPresent);
}

void Dx12Renderer::ExecuteCurrentCommandList()
{
    ID3D12CommandList* commandLists[] = {mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(1, commandLists);
}

void Dx12Renderer::CreateFenceObjects()
{
    ThrowIfFailed(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
    mFenceValues[mFrameIndex] = 1;

    mFenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (mFenceEvent == nullptr)
    {
        throw std::runtime_error("펜스 이벤트 생성에 실패했습니다.");
    }
}

void Dx12Renderer::MoveToNextFrame()
{
    const UINT64 currentFenceValue = mFenceValues[mFrameIndex];
    ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), currentFenceValue));

    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

    if (mFence->GetCompletedValue() < mFenceValues[mFrameIndex])
    {
        ThrowIfFailed(mFence->SetEventOnCompletion(mFenceValues[mFrameIndex], mFenceEvent));
        WaitForSingleObject(mFenceEvent, INFINITE);
    }

    mFenceValues[mFrameIndex] = currentFenceValue + 1;
}

} // namespace Kimgane::Client
