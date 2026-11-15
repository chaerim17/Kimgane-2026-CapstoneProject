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
#include <wincodec.h>
#include <wrl/client.h>

#include <DirectXMath.h>

#include "Mesh.h"
#include "RenderSettings.h"

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace Kimgane::Engine
{
class GameObject;
class Scene;
class TextComponent;
class Texture;

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
    void SetCrosshairVisible(bool visible) noexcept;
    void Render(const Scene& scene);
    void BeginFrame();
    void RenderScene(const Scene& scene,
                     RenderPass pass = RenderPass::World,
                     bool includeText = true,
                     MeshPrimitiveTopology primitiveTopology = MeshPrimitiveTopology::TriangleList);
    void EndFrame();
    void WaitForGpu();

    [[nodiscard]] ID3D12Device& GetDevice() const;
    [[nodiscard]] ID3D12CommandQueue& GetCommandQueue() const;
    [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetDefaultTextureGpuHandle() const noexcept;
    [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE CreateTextureView(ID3D12Resource& textureResource);
    [[nodiscard]] UINT GetWidthPx() const noexcept { return mWidthPx; }
    [[nodiscard]] UINT GetHeightPx() const noexcept { return mHeightPx; }

    // 화면에 고정되는 2D UI(HUD)용 그리기 요청. 좌표는 화면 픽셀(left, top, right, bottom)이고,
    // 월드 텍스트·체력바·조준선 위에 요청한 순서대로 그려짐. 매 프레임 BeginFrame에서 비워짐.
    // 정렬 값은 TextHorizontalAlignment / TextVerticalAlignment를 int로 넘김.
    void QueueUiFillRect(const DirectX::XMFLOAT4& rectPx, const DirectX::XMFLOAT4& colorLinear);
    void QueueUiStrokeRect(const DirectX::XMFLOAT4& rectPx, const DirectX::XMFLOAT4& colorLinear, float thicknessPx);
    void QueueUiText(std::wstring text,
                     const DirectX::XMFLOAT4& rectPx,
                     const DirectX::XMFLOAT4& colorLinear,
                     float fontSizeDip,
                     int horizontalAlignment,
                     int verticalAlignment,
                     bool drawShadow = true);
    // PNG 등 이미지 파일을 rectPx에 맞춰 그림. 경로는 Assets 기준 상대 경로이고 처음 그릴 때 한 번 읽어 캐시함.
    // 파일이 없으면 로그만 남기고 건너뜀.
    void QueueUiImage(std::wstring imagePath, const DirectX::XMFLOAT4& rectPx, float opacity = 1.0F);

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
        bool drawShadow = false;
        float shadowOffsetPx = 0.0F; // drawShadow일 때 그림자를 오른쪽 아래로 미는 거리
    };

    enum class UiDrawKind
    {
        FillRect,
        StrokeRect,
        Text,
        Image
    };

    struct UiDrawCommand
    {
        UiDrawKind kind = UiDrawKind::FillRect;
        DirectX::XMFLOAT4 rectPx = {};
        DirectX::XMFLOAT4 colorLinear = {1.0F, 1.0F, 1.0F, 1.0F};
        float thicknessPx = 1.0F;
        TextDrawCommand text;
        std::wstring imagePath;
    };

    struct HealthBarDrawCommand
    {
        DirectX::XMFLOAT2 centerPx = {};
        float hpRatio = 0.0F;
    };

    void CreateDeviceResources();
    void CreateSwapChain();
    void CreateRenderTargetViews();
    void CreateDepthStencilView();
    void CreateCommandObjects();
    void CreatePipelineObjects();
    void CreateShaderResourceViewHeap();
    void CreateDefaultTexture();
    void CreateTextOverlayResources();
    void CreateFenceObjects();
    void LoadUiFontCollection();
    void QueueTextCommands(const Scene& scene);
    [[nodiscard]] DirectX::XMFLOAT4 BuildTextRectPx(const DirectX::XMFLOAT4X4& renderWorld,
                                                     const TextComponent& textComponent) const noexcept;
    void QueueHealthBarCommands(const Scene& scene);
    void QueueDamageNumberCommands(const Scene& scene);
    [[nodiscard]] bool TryProjectWorldToScreenPx(const DirectX::XMFLOAT3& worldPositionM,
                                                  DirectX::XMFLOAT2& outPx) const noexcept;
    [[nodiscard]] bool IsLineOfSightBlocked(const DirectX::XMFLOAT3& fromM,
                                            const DirectX::XMFLOAT3& toM,
                                            const Scene& scene) const noexcept;
    void DrawHealthBars();
    [[nodiscard]] Microsoft::WRL::ComPtr<IDWriteTextFormat> CreateTextFormat(float fontSizeDip,
                                                                              int horizontalAlignment,
                                                                              int verticalAlignment) const;
    [[nodiscard]] IDWriteTextFormat* FindOrCreateTextFormat(float fontSizeDip,
                                                            int horizontalAlignment,
                                                            int verticalAlignment);
    void DrawTextCommand(const TextDrawCommand& command);
    void DrawTextOverlay();
    void DrawUiCommands();
    [[nodiscard]] ID2D1Bitmap1* FindOrLoadUiBitmap(const std::wstring& imagePath);
    [[nodiscard]] Microsoft::WRL::ComPtr<ID2D1Bitmap1> LoadUiBitmap(const std::wstring& imagePath) const;
    void DrawCrosshair();
    void TransitionCurrentBackBufferToPresent();
    void ExecuteCurrentCommandList();
    void MoveToNextFrame();

    HWND mWindowHandle = nullptr;
    UINT mWidthPx = 0;
    UINT mHeightPx = 0;
    UINT mFrameIndex = 0;
    UINT mRtvDescriptorSize = 0;
    UINT mSrvDescriptorSize = 0;
    UINT mSrvDescriptorCount = 0;
    HANDLE mFenceEvent = nullptr;
    D3D12_VIEWPORT mViewport = {};
    D3D12_RECT mScissorRect = {};
    DirectX::XMFLOAT4X4 mViewProjection = {};
    DirectX::XMFLOAT3 mCameraPositionM = {0.0F, 0.0F, 0.0F};
    bool mCrosshairVisible = false;
    std::array<UINT64, FRAME_COUNT> mFenceValues = {};
    std::vector<TextDrawCommand> mTextDrawCommands;
    std::vector<HealthBarDrawCommand> mHealthBarDrawCommands;
    std::vector<UiDrawCommand> mUiDrawCommands;
    // 이미지 경로별 D2D 비트맵 캐시. 로드 실패도 nullptr로 저장해서 매 프레임 다시 읽지 않음.
    std::unordered_map<std::wstring, Microsoft::WRL::ComPtr<ID2D1Bitmap1>> mUiBitmaps;
    // (글자 크기, 가로 정렬, 세로 정렬)별 글꼴 서식 캐시. 글자를 그릴 때마다 새로 만들지 않도록 재사용함.
    std::map<std::tuple<float, int, int>, Microsoft::WRL::ComPtr<IDWriteTextFormat>> mTextFormats;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> mTextBrush;
    std::wstring mUiFontFamilyName = L"Malgun Gothic";

    Microsoft::WRL::ComPtr<IDXGIFactory4> mFactory;
    Microsoft::WRL::ComPtr<ID3D12Device> mDevice;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
    Microsoft::WRL::ComPtr<ID3D11Device> mD3d11Device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> mD3d11Context;
    Microsoft::WRL::ComPtr<ID3D11On12Device> mD3d11On12Device;
    Microsoft::WRL::ComPtr<IDXGISwapChain3> mSwapChain;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mSrvHeap;
    std::shared_ptr<Texture> mDefaultTexture;
    D3D12_GPU_DESCRIPTOR_HANDLE mDefaultTextureGpuHandle = {};
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencil;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> mPipelineState;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> mTerrainPipelineState;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> mLinePipelineState;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> mOverlayPipelineState;
    Microsoft::WRL::ComPtr<ID2D1Factory1> mD2dFactory;
    Microsoft::WRL::ComPtr<ID2D1Device> mD2dDevice;
    Microsoft::WRL::ComPtr<ID2D1DeviceContext> mD2dContext;
    Microsoft::WRL::ComPtr<IDWriteFactory3> mDWriteFactory;
    Microsoft::WRL::ComPtr<IDWriteFontCollection1> mUiFontCollection;
    Microsoft::WRL::ComPtr<IWICImagingFactory> mWicFactory;
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, FRAME_COUNT> mRenderTargets;
    std::array<Microsoft::WRL::ComPtr<ID3D11Resource>, FRAME_COUNT> mWrappedBackBuffers;
    std::array<Microsoft::WRL::ComPtr<ID2D1Bitmap1>, FRAME_COUNT> mD2dRenderTargets;
    std::array<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>, FRAME_COUNT> mCommandAllocators;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;
    Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
};
} // namespace Kimgane::Engine
