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

#include <cstdint>
#include <memory>
#include <vector>

namespace Kimgane::Engine
{
struct MeshTriangle
{
    DirectX::XMFLOAT3 aM;
    DirectX::XMFLOAT3 bM;
    DirectX::XMFLOAT3 cM;
};

class Mesh
{
public:
    struct Vertex
    {
        DirectX::XMFLOAT3 positionM;
        DirectX::XMFLOAT3 normal;
        DirectX::XMFLOAT4 colorLinear;
    };

    static std::shared_ptr<Mesh> CreateCube(ID3D12Device& device, float sizeM);
    static std::shared_ptr<Mesh> Create(ID3D12Device& device,
                                        const std::vector<Vertex>& vertices,
                                        const std::vector<std::uint32_t>& indices = {});

    virtual ~Mesh() = default;

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&&) = delete;
    Mesh& operator=(Mesh&&) = delete;

    void Render(ID3D12GraphicsCommandList& commandList) const noexcept;

    [[nodiscard]] const DirectX::BoundingBox& GetLocalAabb() const noexcept;
    [[nodiscard]] const DirectX::BoundingOrientedBox& GetLocalObb() const noexcept;
    [[nodiscard]] const std::vector<MeshTriangle>& GetLocalTriangles() const noexcept;
    [[nodiscard]] bool HasIndices() const noexcept;

protected:
    Mesh() = default;

    void CreateVertexBuffer(ID3D12Device& device, const std::vector<Vertex>& vertices);
    void CreateIndexBuffer(ID3D12Device& device, const std::vector<std::uint32_t>& indices);
    void CreateBounds(const std::vector<Vertex>& vertices) noexcept;
    void CreateTriangles(const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices);

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
    Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_ = {};
    D3D12_INDEX_BUFFER_VIEW indexBufferView_ = {};
    UINT vertexCount_ = 0;
    UINT indexCount_ = 0;
    DirectX::BoundingBox localAabb_ = {};
    DirectX::BoundingOrientedBox localObb_ = {};
    std::vector<MeshTriangle> localTriangles_;
};
} // namespace Kimgane::Engine
