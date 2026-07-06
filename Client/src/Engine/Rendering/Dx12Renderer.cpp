#include "Pch.h"

#include "Dx12Renderer.h"

#include "../Scene/Scene.h"
#include "SceneRenderConstants.h"
#include "Shader.h"

#include <algorithm>
#include <stdexcept>

namespace Kimgane::Engine
{
namespace
{
using Microsoft::WRL::ComPtr;

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
} // namespace

Dx12Renderer::~Dx12Renderer()
{
    if (fenceEvent_ != nullptr)
    {
        CloseHandle(fenceEvent_);
        fenceEvent_ = nullptr;
    }
}

void Dx12Renderer::Initialize(HWND windowHandle, UINT widthPx, UINT heightPx)
{
    windowHandle_ = windowHandle;
    widthPx_ = widthPx;
    heightPx_ = heightPx;

    CreateDeviceResources();
    CreateSwapChain();
    CreateRenderTargetViews();
    CreateDepthStencilView();
    CreateCommandObjects();
    CreatePipelineObjects();
    CreateFenceObjects();
    DirectX::XMStoreFloat4x4(&viewProjection_, DirectX::XMMatrixIdentity());
}

void Dx12Renderer::SetViewProjection(const DirectX::XMFLOAT4X4& viewProjection) noexcept
{
    viewProjection_ = viewProjection;
}

void Dx12Renderer::Render(const Scene& scene)
{
    ThrowIfFailed(commandAllocators_[frameIndex_]->Reset());
    ThrowIfFailed(commandList_->Reset(commandAllocators_[frameIndex_].Get(), pipelineState_.Get()));

    D3D12_RESOURCE_BARRIER barrierToRenderTarget = {};
    barrierToRenderTarget.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierToRenderTarget.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrierToRenderTarget.Transition.pResource = renderTargets_[frameIndex_].Get();
    barrierToRenderTarget.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrierToRenderTarget.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrierToRenderTarget.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList_->ResourceBarrier(1, &barrierToRenderTarget);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    rtvHandle.ptr += static_cast<SIZE_T>(frameIndex_) * rtvDescriptorSize_;
    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap_->GetCPUDescriptorHandleForHeapStart();

    commandList_->RSSetViewports(1, &viewport_);
    commandList_->RSSetScissorRects(1, &scissorRect_);
    commandList_->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
    commandList_->ClearRenderTargetView(rtvHandle, RenderSettings::kClearColor.data(), 0, nullptr);
    commandList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0F, 0, 0, nullptr);

    const DirectionalLightShaderData lightShaderData = scene.GetDirectionalLight().BuildShaderData();
    SceneShaderConstants sceneConstants = {};
    sceneConstants.viewProjection = viewProjection_;
    sceneConstants.lightDirectionIntensity = lightShaderData.directionIntensity;
    sceneConstants.lightColorAmbient = lightShaderData.colorAmbient;

    commandList_->SetGraphicsRootSignature(rootSignature_.Get());
    commandList_->SetGraphicsRoot32BitConstants(RenderRootParameter::kScene,
                                                RenderRootParameter::kSceneConstants32BitCount,
                                                &sceneConstants,
                                                0);
    scene.Render(*commandList_.Get());

    D3D12_RESOURCE_BARRIER barrierToPresent = {};
    barrierToPresent.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrierToPresent.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrierToPresent.Transition.pResource = renderTargets_[frameIndex_].Get();
    barrierToPresent.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrierToPresent.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrierToPresent.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList_->ResourceBarrier(1, &barrierToPresent);

    ThrowIfFailed(commandList_->Close());

    ID3D12CommandList* commandLists[] = {commandList_.Get()};
    commandQueue_->ExecuteCommandLists(1, commandLists);

    ThrowIfFailed(swapChain_->Present(1, 0));
    MoveToNextFrame();
}

void Dx12Renderer::WaitForGpu()
{
    if (commandQueue_ == nullptr || fence_ == nullptr || fenceEvent_ == nullptr)
    {
        return;
    }

    ThrowIfFailed(commandQueue_->Signal(fence_.Get(), fenceValues_[frameIndex_]));
    ThrowIfFailed(fence_->SetEventOnCompletion(fenceValues_[frameIndex_], fenceEvent_));
    WaitForSingleObject(fenceEvent_, INFINITE);
    ++fenceValues_[frameIndex_];
}

ID3D12Device& Dx12Renderer::GetDevice() const
{
    if (device_ == nullptr)
    {
        throw std::runtime_error("DirectX 12 device is not initialized.");
    }

    return *device_.Get();
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

    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory_)));

    ComPtr<IDXGIAdapter1> hardwareAdapter;
    GetHardwareAdapter(factory_.Get(), &hardwareAdapter);

    if (hardwareAdapter != nullptr)
    {
        ThrowIfFailed(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device_)));
    }
    else
    {
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(factory_->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));
        ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device_)));
    }

    D3D12_COMMAND_QUEUE_DESC queueDescription = {};
    queueDescription.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDescription.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(device_->CreateCommandQueue(&queueDescription, IID_PPV_ARGS(&commandQueue_)));
}

void Dx12Renderer::CreateSwapChain()
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDescription = {};
    swapChainDescription.BufferCount = kFrameCount;
    swapChainDescription.Width = widthPx_;
    swapChainDescription.Height = heightPx_;
    swapChainDescription.Format = RenderSettings::kRenderTargetFormat;
    swapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDescription.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(factory_->CreateSwapChainForHwnd(commandQueue_.Get(),
                                                   windowHandle_,
                                                   &swapChainDescription,
                                                   nullptr,
                                                   nullptr,
                                                   &swapChain));

    ThrowIfFailed(factory_->MakeWindowAssociation(windowHandle_, DXGI_MWA_NO_ALT_ENTER));
    ThrowIfFailed(swapChain.As(&swapChain_));
    frameIndex_ = swapChain_->GetCurrentBackBufferIndex();
}

void Dx12Renderer::CreateRenderTargetViews()
{
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDescription = {};
    rtvHeapDescription.NumDescriptors = kFrameCount;
    rtvHeapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(device_->CreateDescriptorHeap(&rtvHeapDescription, IID_PPV_ARGS(&rtvHeap_)));

    rtvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    for (UINT frame = 0; frame < kFrameCount; ++frame)
    {
        ThrowIfFailed(swapChain_->GetBuffer(frame, IID_PPV_ARGS(&renderTargets_[frame])));
        device_->CreateRenderTargetView(renderTargets_[frame].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += rtvDescriptorSize_;
    }
}

void Dx12Renderer::CreateDepthStencilView()
{
    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDescription = {};
    dsvHeapDescription.NumDescriptors = 1;
    dsvHeapDescription.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDescription.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(device_->CreateDescriptorHeap(&dsvHeapDescription, IID_PPV_ARGS(&dsvHeap_)));

    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC resourceDescription = {};
    resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDescription.Width = widthPx_;
    resourceDescription.Height = heightPx_;
    resourceDescription.DepthOrArraySize = 1;
    resourceDescription.MipLevels = 1;
    resourceDescription.Format = RenderSettings::kDepthStencilFormat;
    resourceDescription.SampleDesc.Count = 1;
    resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    resourceDescription.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = RenderSettings::kDepthStencilFormat;
    clearValue.DepthStencil.Depth = 1.0F;
    clearValue.DepthStencil.Stencil = 0;

    ThrowIfFailed(device_->CreateCommittedResource(&heapProperties,
                                                   D3D12_HEAP_FLAG_NONE,
                                                   &resourceDescription,
                                                   D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                                   &clearValue,
                                                   IID_PPV_ARGS(&depthStencil_)));

    device_->CreateDepthStencilView(depthStencil_.Get(), nullptr, dsvHeap_->GetCPUDescriptorHandleForHeapStart());
}

void Dx12Renderer::CreateCommandObjects()
{
    for (UINT frame = 0; frame < kFrameCount; ++frame)
    {
        ThrowIfFailed(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                     IID_PPV_ARGS(&commandAllocators_[frame])));
    }

    ThrowIfFailed(device_->CreateCommandList(0,
                                            D3D12_COMMAND_LIST_TYPE_DIRECT,
                                            commandAllocators_[frameIndex_].Get(),
                                            nullptr,
                                            IID_PPV_ARGS(&commandList_)));
    ThrowIfFailed(commandList_->Close());
}

void Dx12Renderer::CreatePipelineObjects()
{
    D3D12_ROOT_PARAMETER rootParameters[2] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParameters[0].Constants.ShaderRegister = 0;
    rootParameters[0].Constants.RegisterSpace = 0;
    rootParameters[0].Constants.Num32BitValues = RenderRootParameter::kSceneConstants32BitCount;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParameters[1].Constants.ShaderRegister = 1;
    rootParameters[1].Constants.RegisterSpace = 0;
    rootParameters[1].Constants.Num32BitValues = RenderRootParameter::kObjectConstants32BitCount;
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

    ThrowIfFailed(device_->CreateRootSignature(0,
                                               signature->GetBufferPointer(),
                                               signature->GetBufferSize(),
                                               IID_PPV_ARGS(&rootSignature_)));

    ComPtr<ID3DBlob> vertexShader =
        ShaderCompiler::CompileFromSource(ShaderLibrary::GetLitColorShaderSource(), "VSMain", "vs_5_0");
    ComPtr<ID3DBlob> pixelShader =
        ShaderCompiler::CompileFromSource(ShaderLibrary::GetLitColorShaderSource(), "PSMain", "ps_5_0");

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
    pipelineStateDescription.pRootSignature = rootSignature_.Get();
    pipelineStateDescription.VS = {vertexShader->GetBufferPointer(), vertexShader->GetBufferSize()};
    pipelineStateDescription.PS = {pixelShader->GetBufferPointer(), pixelShader->GetBufferSize()};
    pipelineStateDescription.RasterizerState = rasterizerDescription;
    pipelineStateDescription.BlendState = blendDescription;
    pipelineStateDescription.DepthStencilState = depthStencilDescription;
    pipelineStateDescription.SampleMask = UINT_MAX;
    pipelineStateDescription.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pipelineStateDescription.NumRenderTargets = 1;
    pipelineStateDescription.RTVFormats[0] = RenderSettings::kRenderTargetFormat;
    pipelineStateDescription.DSVFormat = RenderSettings::kDepthStencilFormat;
    pipelineStateDescription.SampleDesc.Count = 1;

    ThrowIfFailed(device_->CreateGraphicsPipelineState(&pipelineStateDescription, IID_PPV_ARGS(&pipelineState_)));

    viewport_.TopLeftX = 0.0F;
    viewport_.TopLeftY = 0.0F;
    viewport_.Width = static_cast<float>(widthPx_);
    viewport_.Height = static_cast<float>(heightPx_);
    viewport_.MinDepth = 0.0F;
    viewport_.MaxDepth = 1.0F;

    scissorRect_ = {0, 0, static_cast<LONG>(widthPx_), static_cast<LONG>(heightPx_)};
}

void Dx12Renderer::CreateFenceObjects()
{
    ThrowIfFailed(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)));
    fenceValues_[frameIndex_] = 1;

    fenceEvent_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (fenceEvent_ == nullptr)
    {
        throw std::runtime_error("Failed to create the fence event.");
    }
}

void Dx12Renderer::MoveToNextFrame()
{
    const UINT64 currentFenceValue = fenceValues_[frameIndex_];
    ThrowIfFailed(commandQueue_->Signal(fence_.Get(), currentFenceValue));

    frameIndex_ = swapChain_->GetCurrentBackBufferIndex();

    if (fence_->GetCompletedValue() < fenceValues_[frameIndex_])
    {
        ThrowIfFailed(fence_->SetEventOnCompletion(fenceValues_[frameIndex_], fenceEvent_));
        WaitForSingleObject(fenceEvent_, INFINITE);
    }

    fenceValues_[frameIndex_] = currentFenceValue + 1;
}

} // namespace Kimgane::Engine
