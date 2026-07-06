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

#include <DirectXCollision.h>
#include <DirectXMath.h>

#include <memory>
#include <vector>

namespace Kimgane::Engine
{
class Mesh final
{
public:
    struct Vertex
    {
        DirectX::XMFLOAT3 positionM;
    };

    static std::shared_ptr<Mesh> CreateCube(ID3D12Device& device, float sizeM);

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&) = delete;
    Mesh& operator=(Mesh&&) = delete;

    void Render(ID3D12GraphicsCommandList& commandList) const noexcept;

    [[nodiscard]] const DirectX::BoundingBox& GetLocalAabb() const noexcept;
    [[nodiscard]] const DirectX::BoundingOrientedBox& GetLocalObb() const noexcept;

private:
    Mesh() = default;

    void CreateVertexBuffer(ID3D12Device& device, const std::vector<Vertex>& vertices);
    void CreateBounds(const std::vector<Vertex>& vertices) noexcept;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_ = {};
    UINT vertexCount_ = 0;
    DirectX::BoundingBox localAabb_ = {};
    DirectX::BoundingOrientedBox localObb_ = {};
};
} // namespace Kimgane::Engine
