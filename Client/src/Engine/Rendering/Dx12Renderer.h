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
    static constexpr UINT FRAME_COUNT = RenderSettings::FRAME_COUNT;

    void CreateDeviceResources();
    void CreateSwapChain();
    void CreateRenderTargetViews();
    void CreateDepthStencilView();
    void CreateCommandObjects();
    void CreatePipelineObjects();
    void CreateFenceObjects();
    void MoveToNextFrame();

    HWND mWindowHandle = nullptr;
    UINT mWidthPx = 0;
    UINT mHeightPx = 0;
    UINT mFrameIndex = 0;
    UINT mRtvDescriptorSize = 0;
    HANDLE mFenceEvent = nullptr;
    D3D12_VIEWPORT mViewport = {};
    D3D12_RECT mScissorRect = {};
    DirectX::XMFLOAT4X4 mViewProjection = {};
    DirectX::XMFLOAT3 mCameraPositionM = {0.0F, 0.0F, 0.0F};
    std::array<UINT64, FRAME_COUNT> mFenceValues = {};

    Microsoft::WRL::ComPtr<IDXGIFactory4> mFactory;
    Microsoft::WRL::ComPtr<ID3D12Device> mDevice;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> mSwapChain;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencil;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> mPipelineState;
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, FRAME_COUNT> mRenderTargets;
    std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, FRAME_COUNT> mCommandAllocators;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;
    Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
};
} // namespace Kimgane::Engine
