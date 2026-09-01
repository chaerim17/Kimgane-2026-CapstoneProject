#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <d2d1_1.h>
#include <d3d11on12.h>
#include <d3d12.h>
#include <dwrite_3.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

#include <DirectXMath.h>

#include "Mesh.h"
#include "RenderSettings.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace Kimgane::Engine
{
class GameObject;
class Scene;
class TextComponent;

enum class RenderPass
{
    World,
    Overlay
};

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
    void BeginFrame();
    void RenderScene(const Scene& scene,
                     RenderPass pass = RenderPass::World,
                     bool includeText = true,
                     MeshPrimitiveTopology primitiveTopology = MeshPrimitiveTopology::TriangleList);
    void EndFrame();
    void WaitForGpu();

    [[nodiscard]] ID3D12Device& GetDevice() const;

private:
    static constexpr UINT FRAME_COUNT = RenderSettings::FRAME_COUNT;

    struct TextDrawCommand
    {
        std::wstring text;
        DirectX::XMFLOAT4 rectPx = {};
        DirectX::XMFLOAT4 colorLinear = {1.0F, 1.0F, 1.0F, 1.0F};
        float fontSizeDip = 28.0F;
        int horizontalAlignment = 1;
        int verticalAlignment = 1;
    };

    void CreateDeviceResources();
    void CreateSwapChain();
    void CreateRenderTargetViews();
    void CreateDepthStencilView();
    void CreateCommandObjects();
    void CreatePipelineObjects();
    void CreateTextOverlayResources();
    void CreateFenceObjects();
    void LoadUiFontCollection();
    void QueueTextCommands(const Scene& scene);
    [[nodiscard]] DirectX::XMFLOAT4 BuildTextRectPx(const GameObject& object,
                                                     const TextComponent& textComponent) const noexcept;
    [[nodiscard]] Microsoft::WRL::ComPtr<IDWriteTextFormat> CreateTextFormat(float fontSizeDip,
                                                                              int horizontalAlignment,
                                                                              int verticalAlignment) const;
    void DrawTextOverlay();
    void TransitionCurrentBackBufferToPresent();
    void ExecuteCurrentCommandList();
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
    std::vector<TextDrawCommand> mTextDrawCommands;
    std::wstring mUiFontFamilyName = L"Malgun Gothic";

    Microsoft::WRL::ComPtr<IDXGIFactory4> mFactory;
    Microsoft::WRL::ComPtr<ID3D12Device> mDevice;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
    Microsoft::WRL::ComPtr<ID3D11Device> mD3d11Device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> mD3d11Context;
    Microsoft::WRL::ComPtr<ID3D11On12Device> mD3d11On12Device;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> mSwapChain;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencil;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> mPipelineState;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> mLinePipelineState;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> mOverlayPipelineState;
    Microsoft::WRL::ComPtr<ID2D1Factory1> mD2dFactory;
    Microsoft::WRL::ComPtr<ID2D1Device> mD2dDevice;
    Microsoft::WRL::ComPtr<ID2D1DeviceContext> mD2dContext;
    Microsoft::WRL::ComPtr<IDWriteFactory3> mDWriteFactory;
    Microsoft::WRL::ComPtr<IDWriteFontCollection1> mUiFontCollection;
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, FRAME_COUNT> mRenderTargets;
    std::array<Microsoft::WRL::ComPtr<ID3D11Resource>, FRAME_COUNT> mWrappedBackBuffers;
    std::array<Microsoft::WRL::ComPtr<ID2D1Bitmap1>, FRAME_COUNT> mD2dRenderTargets;
    std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, FRAME_COUNT> mCommandAllocators;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;
    Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
};
} // namespace Kimgane::Engine
