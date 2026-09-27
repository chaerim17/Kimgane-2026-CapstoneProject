#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Windows.h>
#include <d3d12.h>
#include <wrl/client.h>

#include <filesystem>
#include <memory>

namespace Kimgane::Engine
{
    class Texture final
    {
    public:
        static std::shared_ptr<Texture> Load(ID3D12Device& device, ID3D12CommandQueue& commandQueue,
                                             const std::filesystem::path& filePath);
        static std::shared_ptr<Texture> CreateWhite1x1(ID3D12Device& device, ID3D12CommandQueue& commandQueue);

        Texture() = default;
        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        [[nodiscard]] ID3D12Resource& GetResource() const noexcept;

    private:
        Microsoft::WRL::ComPtr<ID3D12Resource> mResource;
    };
} // namespace Kimgane::Engine
