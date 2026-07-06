#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <array>
#include <cstdint>
#include <stdexcept>

namespace
{
using Microsoft::WRL::ComPtr;

constexpr UINT kFrameCount = 2;
constexpr UINT kClientWidth = 1280;
constexpr UINT kClientHeight = 720;
constexpr wchar_t kWindowClassName[] = L"KimganeClientWindowClass";
constexpr wchar_t kWindowTitle[] = L"Kimgane Client";
constexpr std::array<float, 4> kClearColor = {0.05F, 0.08F, 0.12F, 1.0F};

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

class Dx12ClientApp
{
public:
    int Run(HINSTANCE instance, int commandShow)
    {
        InitializeWindow(instance, commandShow);
        InitializeD3D12();
        return RunMessageLoop();
    }

private:
    void InitializeWindow(HINSTANCE instance, int commandShow)
    {
        WNDCLASSEXW windowClass = {};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.lpfnWndProc = &Dx12ClientApp::WindowProc;
        windowClass.hInstance = instance;
        windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        windowClass.lpszClassName = kWindowClassName;

        if (RegisterClassExW(&windowClass) == 0U)
        {
            throw std::runtime_error("Failed to register the window class.");
        }

        RECT windowRect = {0, 0, static_cast<LONG>(kClientWidth), static_cast<LONG>(kClientHeight)};
        if (AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE) == FALSE)
        {
            throw std::runtime_error("Failed to adjust the window rectangle.");
        }

        windowHandle_ = CreateWindowExW(0,
                                        kWindowClassName,
                                        kWindowTitle,
                                        WS_OVERLAPPEDWINDOW,
                                        CW_USEDEFAULT,
                                        CW_USEDEFAULT,
                                        windowRect.right - windowRect.left,
                                        windowRect.bottom - windowRect.top,
                                        nullptr,
                                        nullptr,
                                        instance,
                                        this);

        if (windowHandle_ == nullptr)
        {
            throw std::runtime_error("Failed to create the window.");
        }

        ShowWindow(windowHandle_, commandShow);
    }

    void InitializeD3D12()
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

        DXGI_SWAP_CHAIN_DESC1 swapChainDescription = {};
        swapChainDescription.BufferCount = kFrameCount;
        swapChainDescription.Width = kClientWidth;
        swapChainDescription.Height = kClientHeight;
        swapChainDescription.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
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

        CreateRenderTargetViews();
        CreateCommandObjects();
        CreateFenceObjects();
    }

    void CreateRenderTargetViews()
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

    void CreateCommandObjects()
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

    void CreateFenceObjects()
    {
        ThrowIfFailed(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)));
        fenceValues_[frameIndex_] = 1;

        fenceEvent_ = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (fenceEvent_ == nullptr)
        {
            throw std::runtime_error("Failed to create the fence event.");
        }
    }

    int RunMessageLoop()
    {
        MSG message = {};
        while (message.message != WM_QUIT)
        {
            if (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE) != FALSE)
            {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
            else
            {
                Render();
            }
        }

        WaitForGpu();
        CloseHandle(fenceEvent_);
        fenceEvent_ = nullptr;
        return static_cast<int>(message.wParam);
    }

    void Render()
    {
        ThrowIfFailed(commandAllocators_[frameIndex_]->Reset());
        ThrowIfFailed(commandList_->Reset(commandAllocators_[frameIndex_].Get(), nullptr));

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

        commandList_->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
        commandList_->ClearRenderTargetView(rtvHandle, kClearColor.data(), 0, nullptr);

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

    void MoveToNextFrame()
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

    void WaitForGpu()
    {
        ThrowIfFailed(commandQueue_->Signal(fence_.Get(), fenceValues_[frameIndex_]));
        ThrowIfFailed(fence_->SetEventOnCompletion(fenceValues_[frameIndex_], fenceEvent_));
        WaitForSingleObject(fenceEvent_, INFINITE);
        ++fenceValues_[frameIndex_];
    }

    static LRESULT CALLBACK WindowProc(HWND windowHandle, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (message == WM_NCCREATE)
        {
            const auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
            SetWindowLongPtrW(windowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
        }

        switch (message)
        {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            return DefWindowProcW(windowHandle, message, wParam, lParam);
        }
    }

    HWND windowHandle_ = nullptr;
    UINT frameIndex_ = 0;
    UINT rtvDescriptorSize_ = 0;
    HANDLE fenceEvent_ = nullptr;
    std::array<UINT64, kFrameCount> fenceValues_ = {};

    ComPtr<IDXGIFactory4> factory_;
    ComPtr<ID3D12Device> device_;
    ComPtr<ID3D12CommandQueue> commandQueue_;
    ComPtr<IDXGISwapChain3> swapChain_;
    ComPtr<ID3D12DescriptorHeap> rtvHeap_;
    std::array<ComPtr<ID3D12Resource>, kFrameCount> renderTargets_;
    std::array<ComPtr<ID3D12CommandAllocator>, kFrameCount> commandAllocators_;
    ComPtr<ID3D12GraphicsCommandList> commandList_;
    ComPtr<ID3D12Fence> fence_;
};
} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previousInstance, PWSTR commandLine, int commandShow)
{
    UNREFERENCED_PARAMETER(previousInstance);
    UNREFERENCED_PARAMETER(commandLine);

    try
    {
        Dx12ClientApp app;
        return app.Run(instance, commandShow);
    }
    catch (const std::exception& exception)
    {
        MessageBoxA(nullptr, exception.what(), "Kimgane.Client", MB_OK | MB_ICONERROR);
        return -1;
    }
}
