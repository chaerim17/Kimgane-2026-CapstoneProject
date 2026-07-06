#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <DirectXMath.h>

#include "RenderSettings.h"

#include <array>
#include <cstdint>

namespace Kimgane::Engine
{
class Scene;

class Dx12Renderer final
{
public:
    Dx12Renderer() = default;
    ~Dx12Renderer();

    Dx12Renderer(const Dx12Renderer&) = delete;
    Dx12Renderer& operator=(const Dx12Renderer&) = delete;
    Dx12Renderer(Dx12Renderer&&) = delete;
    Dx12Renderer& operator=(Dx12Renderer&&) = delete;

    void Initialize(HWND windowHandle, UINT widthPx, UINT heightPx);
    void SetViewProjection(const DirectX::XMFLOAT4X4& viewProjection) noexcept;
    void SetCameraPositionM(const DirectX::XMFLOAT3& cameraPositionM) noexcept;
    void Render(const Scene& scene);
    void WaitForGpu();

    [[nodiscard]] ID3D12Device& GetDevice() const;

private:
    static constexpr UINT kFrameCount = RenderSettings::kFrameCount;

    void CreateDeviceResources();
    void CreateSwapChain();
    void CreateRenderTargetViews();
    void CreateDepthStencilView();
    void CreateCommandObjects();
    void CreatePipelineObjects();
    void CreateFenceObjects();
    void MoveToNextFrame();

    HWND windowHandle_ = nullptr;
    UINT widthPx_ = 0;
    UINT heightPx_ = 0;
    UINT frameIndex_ = 0;
    UINT rtvDescriptorSize_ = 0;
    HANDLE fenceEvent_ = nullptr;
    D3D12_VIEWPORT viewport_ = {};
    D3D12_RECT scissorRect_ = {};
    DirectX::XMFLOAT4X4 viewProjection_ = {};
    DirectX::XMFLOAT3 mCameraPositionM = {0.0F, 0.0F, 0.0F};
    std::array<UINT64, kFrameCount> fenceValues_ = {};

    Microsoft::WRL::ComPtr<IDXGIFactory4> factory_;
    Microsoft::WRL::ComPtr<ID3D12Device> device_;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> swapChain_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvHeap_;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvHeap_;
    Microsoft::WRL::ComPtr<ID3D12Resource> depthStencil_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, kFrameCount> renderTargets_;
    std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, kFrameCount> commandAllocators_;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList_;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
};
} // namespace Kimgane::Engine
