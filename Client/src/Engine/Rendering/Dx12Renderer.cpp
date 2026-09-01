#include "Pch.h"

#include "Dx12Renderer.h"

#include <d2d1_1helper.h>

#include "../Scene/Scene.h"
#include "TextComponent.h"
#include "../../Shared/IO/AssetPathResolver.h"
#include "SceneRenderConstants.h"
#include "Shader.h"

#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <stdexcept>

namespace Kimgane::Engine
{
namespace
{
using Microsoft::WRL::ComPtr;

constexpr wchar_t UI_FONT_DIRECTORY_PATH[] = L"Assets/Fonts/Paperlogy-1.001";
constexpr wchar_t UI_FONT_FAMILY_NAME[] = L"Paperlogy";
constexpr float D2D_DEFAULT_DPI = 96.0F;
constexpr float MIN_PROJECTED_W = 0.0001F;

void ThrowIfFailed(HRESULT result)
{
    if (FAILED(result))
    {
        throw std::runtime_error("A DirectX 12 call failed.");
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
    CreateDepthStencilView();
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

void Dx12Renderer::Render(const Scene& scene)
{
    BeginFrame();
    RenderScene(scene);
    EndFrame();
}

void Dx12Renderer::BeginFrame()
{
    mTextDrawCommands.clear();

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
}

void Dx12Renderer::RenderScene(const Scene& scene, RenderPass pass, bool includeText)
{
    ID3D12PipelineState* pipelineState = pass == RenderPass::Overlay ? mOverlayPipelineState.Get() : mPipelineState.Get();
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
    scene.Render(*mCommandList.Get());

    if (includeText)
    {
        QueueTextCommands(scene);
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
        throw std::runtime_error("DirectX 12 device is not initialized.");
    }

    return *mDevice.Get();
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
    dsvHeapDescription.NumDescriptors = 1;
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
    resourceDescription.Format = RenderSettings::DEPTH_STENCIL_FORMAT;
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

    mDevice->CreateDepthStencilView(mDepthStencil.Get(), nullptr, mDsvHeap->GetCPUDescriptorHandleForHeapStart());
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
    D3D12_ROOT_PARAMETER rootParameters[2] = {};
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

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDescription = {};
    rootSignatureDescription.NumParameters = _countof(rootParameters);
    rootSignatureDescription.pParameters = rootParameters;
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
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

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
    for (const auto& object : scene.GetObjects())
    {
        if (!object || !object->IsActive())
        {
            continue;
        }

        const auto* textComponent = object->GetComponent<TextComponent>();
        if (textComponent == nullptr || textComponent->GetText().empty())
        {
            continue;
        }

        const DirectX::XMFLOAT4 rectPx = BuildTextRectPx(*object, *textComponent);
        if (rectPx.z <= rectPx.x || rectPx.w <= rectPx.y)
        {
            continue;
        }

        TextDrawCommand command = {};
        command.text = textComponent->GetText();
        command.rectPx = rectPx;
        command.colorLinear = textComponent->GetColorLinear();
        command.fontSizeDip = textComponent->GetFontSizeDip();
        command.horizontalAlignment = static_cast<int>(textComponent->GetHorizontalAlignment());
        command.verticalAlignment = static_cast<int>(textComponent->GetVerticalAlignment());
        mTextDrawCommands.push_back(std::move(command));
    }
}

DirectX::XMFLOAT4 Dx12Renderer::BuildTextRectPx(const GameObject& object,
                                                const TextComponent& textComponent) const noexcept
{
    const DirectX::XMFLOAT2& insetRatio = textComponent.GetInsetRatio();
    const float leftLocal = -0.5F + insetRatio.x;
    const float rightLocal = 0.5F - insetRatio.x;
    const float topLocal = 0.5F - insetRatio.y;
    const float bottomLocal = -0.5F + insetRatio.y;

    const DirectX::XMVECTOR localCorners[] = {
        DirectX::XMVectorSet(leftLocal, topLocal, 0.0F, 1.0F),
        DirectX::XMVectorSet(rightLocal, topLocal, 0.0F, 1.0F),
        DirectX::XMVectorSet(leftLocal, bottomLocal, 0.0F, 1.0F),
        DirectX::XMVectorSet(rightLocal, bottomLocal, 0.0F, 1.0F)};

    const DirectX::XMMATRIX world = object.GetTransform().GetWorldMatrix();
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
        if (std::fabs(projectedW) <= MIN_PROJECTED_W)
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
    if (!mTextDrawCommands.empty())
    {
        mD2dContext->SetTarget(mD2dRenderTargets[mFrameIndex].Get());
        mD2dContext->BeginDraw();
        mD2dContext->SetTransform(D2D1::Matrix3x2F::Identity());

        for (const TextDrawCommand& command : mTextDrawCommands)
        {
            ComPtr<ID2D1SolidColorBrush> brush;
            ThrowIfFailed(mD2dContext->CreateSolidColorBrush(ToD2DColor(command.colorLinear), &brush));

            ComPtr<IDWriteTextFormat> textFormat =
                CreateTextFormat(command.fontSizeDip, command.horizontalAlignment, command.verticalAlignment);
            const D2D1_RECT_F layoutRect =
                D2D1::RectF(command.rectPx.x, command.rectPx.y, command.rectPx.z, command.rectPx.w);

            mD2dContext->DrawText(command.text.c_str(),
                                  static_cast<UINT32>(command.text.size()),
                                  textFormat.Get(),
                                  layoutRect,
                                  brush.Get(),
                                  D2D1_DRAW_TEXT_OPTIONS_CLIP,
                                  DWRITE_MEASURING_MODE_NATURAL);
        }

        drawResult = mD2dContext->EndDraw();
    }

    mD3d11On12Device->ReleaseWrappedResources(&wrappedBackBuffer, 1);
    mD3d11Context->Flush();
    ThrowIfFailed(drawResult);
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
        throw std::runtime_error("Failed to create the fence event.");
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

} // namespace Kimgane::Engine
