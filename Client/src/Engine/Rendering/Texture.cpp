#include "Pch.h"

#include "Texture.h"

#define D3DX12_NO_STATE_OBJECT_HELPERS
#define D3DX12_NO_CHECK_FEATURE_SUPPORT_CLASS
#include "ThirdParty/DirectXTex/d3dx12.h"
#include "ThirdParty/DirectXTex/DDSTextureLoader12.h"
#include "../../Shared/IO/AssetPathResolver.h"

#include <stdexcept>
#include <vector>

namespace Kimgane::Engine
{
    namespace
    {
        void ThrowIfFailed(HRESULT result)
        {
            if (FAILED(result))
            {
                throw std::runtime_error("A DirectX 12 texture resource call failed.");
            }
        }

        std::filesystem::path ResolveDdsPath(const std::filesystem::path& filePath)
        {
            std::filesystem::path ddsPath = filePath;
            if (ddsPath.extension() != ".dds")
            {
                ddsPath += ".dds";
            }

            return Kimgane::Shared::IO::ResolveAssetPath(ddsPath);
        }

        // 업로드 힙을 만들고, 복사 명령을 기록해서 제출하고, GPU가 끝낼 때까지 대기한다.
        // Load()와 CreateWhite1x1()이 공통으로 쓰는 부분을 빼낸 것.
        void UploadTextureData(ID3D12Device& device, ID3D12CommandQueue& commandQueue, ID3D12Resource& texture,
                               const std::vector<D3D12_SUBRESOURCE_DATA>& subresources)
        {
            const UINT64 uploadBufferSize =
                GetRequiredIntermediateSize(&texture, 0, static_cast<UINT>(subresources.size()));

            Microsoft::WRL::ComPtr<ID3D12Resource> uploadHeap;
            const CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
            const CD3DX12_RESOURCE_DESC uploadResourceDescription = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
            ThrowIfFailed(device.CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
                                                         &uploadResourceDescription, D3D12_RESOURCE_STATE_GENERIC_READ,
                                                         nullptr, IID_PPV_ARGS(&uploadHeap)));

            Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
            ThrowIfFailed(
                device.CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));

            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
            ThrowIfFailed(device.CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr,
                                                   IID_PPV_ARGS(&commandList)));

            UpdateSubresources(commandList.Get(), &texture, uploadHeap.Get(), 0, 0,
                               static_cast<UINT>(subresources.size()), subresources.data());

            const CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
                &texture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            commandList->ResourceBarrier(1, &barrier);
            ThrowIfFailed(commandList->Close());

            ID3D12CommandList* commandLists[] = {commandList.Get()};
            commandQueue.ExecuteCommandLists(1, commandLists);

            Microsoft::WRL::ComPtr<ID3D12Fence> fence;
            ThrowIfFailed(device.CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

            HANDLE fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
            if (fenceEvent == nullptr)
            {
                throw std::runtime_error("Failed to create fence event for texture upload.");
            }

            ThrowIfFailed(commandQueue.Signal(fence.Get(), 1));
            ThrowIfFailed(fence->SetEventOnCompletion(1, fenceEvent));
            WaitForSingleObject(fenceEvent, INFINITE);
            CloseHandle(fenceEvent);
        }
    } // namespace

    std::shared_ptr<Texture> Texture::Load(ID3D12Device& device, ID3D12CommandQueue& commandQueue,
                                           const std::filesystem::path& filePath)
    {
        const std::filesystem::path ddsPath = ResolveDdsPath(filePath);
        if (ddsPath.empty())
        {
            throw std::runtime_error("DDS texture not found: " + filePath.string());
        }

        Microsoft::WRL::ComPtr<ID3D12Resource> texture;
        std::unique_ptr<std::uint8_t[]> ddsData;
        std::vector<D3D12_SUBRESOURCE_DATA> subresources;
        ThrowIfFailed(DirectX::LoadDDSTextureFromFile(&device, ddsPath.c_str(), &texture, ddsData, subresources));

        UploadTextureData(device, commandQueue, *texture.Get(), subresources);

        auto result = std::make_shared<Texture>();
        result->mResource = std::move(texture);
        return result;
    }

    std::shared_ptr<Texture> Texture::CreateWhite1x1(ID3D12Device& device, ID3D12CommandQueue& commandQueue)
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> texture;
        const CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
        const CD3DX12_RESOURCE_DESC textureDescription =
            CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 1);
        ThrowIfFailed(device.CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &textureDescription,
                                                     D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texture)));

        static constexpr std::uint8_t WHITE_PIXEL[4] = {255, 255, 255, 255};
        D3D12_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pData = WHITE_PIXEL;
        subresourceData.RowPitch = sizeof(WHITE_PIXEL);
        subresourceData.SlicePitch = sizeof(WHITE_PIXEL);

        UploadTextureData(device, commandQueue, *texture.Get(), {subresourceData});

        auto result = std::make_shared<Texture>();
        result->mResource = std::move(texture);
        return result;
    }

    ID3D12Resource& Texture::GetResource() const noexcept
    {
        return *mResource.Get();
    }
} // namespace Kimgane::Engine
