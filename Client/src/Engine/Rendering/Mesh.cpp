#include "Pch.h"

#include "Mesh.h"

#include <cstring>
#include <stdexcept>
#include <utility>
#include <vector>

namespace Kimgane::Engine
{
namespace
{
void ThrowIfFailed(HRESULT result)
{
    if (FAILED(result))
    {
        throw std::runtime_error("A DirectX 12 mesh resource call failed.");
    }
}

D3D12_HEAP_PROPERTIES CreateUploadHeapProperties() noexcept
{
    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;
    return heapProperties;
}

D3D12_RESOURCE_DESC CreateBufferDescription(UINT64 sizeBytes) noexcept
{
    D3D12_RESOURCE_DESC resourceDescription = {};
    resourceDescription.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resourceDescription.Width = sizeBytes;
    resourceDescription.Height = 1;
    resourceDescription.DepthOrArraySize = 1;
    resourceDescription.MipLevels = 1;
    resourceDescription.Format = DXGI_FORMAT_UNKNOWN;
    resourceDescription.SampleDesc.Count = 1;
    resourceDescription.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resourceDescription.Flags = D3D12_RESOURCE_FLAG_NONE;
    return resourceDescription;
}
} // namespace

std::shared_ptr<Mesh> Mesh::CreateCube(ID3D12Device& device, float sizeM)
{
    const float halfSizeM = sizeM * 0.5F;

    const DirectX::XMFLOAT3 leftDownFront = {-halfSizeM, -halfSizeM, -halfSizeM};
    const DirectX::XMFLOAT3 leftDownBack = {-halfSizeM, -halfSizeM, +halfSizeM};
    const DirectX::XMFLOAT3 leftUpFront = {-halfSizeM, +halfSizeM, -halfSizeM};
    const DirectX::XMFLOAT3 leftUpBack = {-halfSizeM, +halfSizeM, +halfSizeM};
    const DirectX::XMFLOAT3 rightDownFront = {+halfSizeM, -halfSizeM, -halfSizeM};
    const DirectX::XMFLOAT3 rightDownBack = {+halfSizeM, -halfSizeM, +halfSizeM};
    const DirectX::XMFLOAT3 rightUpFront = {+halfSizeM, +halfSizeM, -halfSizeM};
    const DirectX::XMFLOAT3 rightUpBack = {+halfSizeM, +halfSizeM, +halfSizeM};

    constexpr DirectX::XMFLOAT4 kWhiteLinear = {1.0F, 1.0F, 1.0F, 1.0F};
    std::vector<Vertex> vertices;
    vertices.reserve(24);
    std::vector<std::uint32_t> indices;
    indices.reserve(36);

    auto addFace = [&vertices, &indices, &kWhiteLinear](const DirectX::XMFLOAT3& a,
                                                        const DirectX::XMFLOAT3& b,
                                                        const DirectX::XMFLOAT3& c,
                                                        const DirectX::XMFLOAT3& d,
                                                        const DirectX::XMFLOAT3& normal)
    {
        const std::uint32_t baseIndex = static_cast<std::uint32_t>(vertices.size());
        vertices.push_back({a, normal, kWhiteLinear});
        vertices.push_back({b, normal, kWhiteLinear});
        vertices.push_back({c, normal, kWhiteLinear});
        vertices.push_back({d, normal, kWhiteLinear});
        indices.push_back(baseIndex + 0U);
        indices.push_back(baseIndex + 1U);
        indices.push_back(baseIndex + 2U);
        indices.push_back(baseIndex + 0U);
        indices.push_back(baseIndex + 2U);
        indices.push_back(baseIndex + 3U);
    };

    addFace(leftUpFront, rightUpFront, rightDownFront, leftDownFront, {0.0F, 0.0F, -1.0F});
    addFace(leftUpBack, leftUpFront, leftDownFront, leftDownBack, {-1.0F, 0.0F, 0.0F});
    addFace(rightUpFront, rightUpBack, rightDownBack, rightDownFront, {1.0F, 0.0F, 0.0F});
    addFace(leftUpBack, rightUpBack, rightUpFront, leftUpFront, {0.0F, 1.0F, 0.0F});
    addFace(leftDownFront, rightDownFront, rightDownBack, leftDownBack, {0.0F, -1.0F, 0.0F});
    addFace(rightUpBack, leftUpBack, leftDownBack, rightDownBack, {0.0F, 0.0F, 1.0F});

    return Create(device, vertices, indices);
}

std::shared_ptr<Mesh> Mesh::Create(ID3D12Device& device,
                                   const std::vector<Vertex>& vertices,
                                   const std::vector<std::uint32_t>& indices)
{
    auto mesh = std::shared_ptr<Mesh>(new Mesh());
    mesh->CreateVertexBuffer(device, vertices);
    mesh->CreateIndexBuffer(device, indices);
    mesh->CreateBounds(vertices);
    mesh->CreateTriangles(vertices, indices);
    return mesh;
}

void Mesh::Render(ID3D12GraphicsCommandList& commandList) const noexcept
{
    if (vertexCount_ == 0U)
    {
        return;
    }

    commandList.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList.IASetVertexBuffers(0, 1, &vertexBufferView_);
    if (HasIndices())
    {
        commandList.IASetIndexBuffer(&indexBufferView_);
        commandList.DrawIndexedInstanced(indexCount_, 1, 0, 0, 0);
        return;
    }

    commandList.DrawInstanced(vertexCount_, 1, 0, 0);
}

const DirectX::BoundingBox& Mesh::GetLocalAabb() const noexcept
{
    return localAabb_;
}

const DirectX::BoundingOrientedBox& Mesh::GetLocalObb() const noexcept
{
    return localObb_;
}

const std::vector<MeshTriangle>& Mesh::GetLocalTriangles() const noexcept
{
    return localTriangles_;
}

bool Mesh::HasIndices() const noexcept
{
    return indexCount_ > 0U;
}

void Mesh::CreateVertexBuffer(ID3D12Device& device, const std::vector<Vertex>& vertices)
{
    const UINT bufferSizeBytes = static_cast<UINT>(vertices.size() * sizeof(Vertex));
    if (bufferSizeBytes == 0U)
    {
        return;
    }

    D3D12_HEAP_PROPERTIES heapProperties = CreateUploadHeapProperties();
    D3D12_RESOURCE_DESC resourceDescription = CreateBufferDescription(bufferSizeBytes);

    ThrowIfFailed(device.CreateCommittedResource(&heapProperties,
                                                 D3D12_HEAP_FLAG_NONE,
                                                 &resourceDescription,
                                                 D3D12_RESOURCE_STATE_GENERIC_READ,
                                                 nullptr,
                                                 IID_PPV_ARGS(&vertexBuffer_)));

    void* mappedData = nullptr;
    const D3D12_RANGE readRange = {0, 0};
    ThrowIfFailed(vertexBuffer_->Map(0, &readRange, &mappedData));
    std::memcpy(mappedData, vertices.data(), bufferSizeBytes);
    vertexBuffer_->Unmap(0, nullptr);

    vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = bufferSizeBytes;
    vertexBufferView_.StrideInBytes = sizeof(Vertex);
    vertexCount_ = static_cast<UINT>(vertices.size());
}

void Mesh::CreateIndexBuffer(ID3D12Device& device, const std::vector<std::uint32_t>& indices)
{
    const UINT bufferSizeBytes = static_cast<UINT>(indices.size() * sizeof(std::uint32_t));
    if (bufferSizeBytes == 0U)
    {
        return;
    }

    D3D12_HEAP_PROPERTIES heapProperties = CreateUploadHeapProperties();
    D3D12_RESOURCE_DESC resourceDescription = CreateBufferDescription(bufferSizeBytes);

    ThrowIfFailed(device.CreateCommittedResource(&heapProperties,
                                                 D3D12_HEAP_FLAG_NONE,
                                                 &resourceDescription,
                                                 D3D12_RESOURCE_STATE_GENERIC_READ,
                                                 nullptr,
                                                 IID_PPV_ARGS(&indexBuffer_)));

    void* mappedData = nullptr;
    const D3D12_RANGE readRange = {0, 0};
    ThrowIfFailed(indexBuffer_->Map(0, &readRange, &mappedData));
    std::memcpy(mappedData, indices.data(), bufferSizeBytes);
    indexBuffer_->Unmap(0, nullptr);

    indexBufferView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
    indexBufferView_.SizeInBytes = bufferSizeBytes;
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
    indexCount_ = static_cast<UINT>(indices.size());
}

void Mesh::CreateBounds(const std::vector<Vertex>& vertices) noexcept
{
    std::vector<DirectX::XMFLOAT3> positions;
    positions.reserve(vertices.size());
    for (const Vertex& vertex : vertices)
    {
        positions.push_back(vertex.positionM);
    }

    if (positions.empty())
    {
        return;
    }

    DirectX::BoundingBox::CreateFromPoints(localAabb_, positions.size(), positions.data(), sizeof(DirectX::XMFLOAT3));
    localObb_.Center = localAabb_.Center;
    localObb_.Extents = localAabb_.Extents;
    localObb_.Orientation = {0.0F, 0.0F, 0.0F, 1.0F};
}

void Mesh::CreateTriangles(const std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices)
{
    localTriangles_.clear();
    if (!indices.empty())
    {
        localTriangles_.reserve(indices.size() / 3U);
        for (std::size_t index = 0; index + 2U < indices.size(); index += 3U)
        {
            const std::uint32_t a = indices[index + 0U];
            const std::uint32_t b = indices[index + 1U];
            const std::uint32_t c = indices[index + 2U];
            if (a < vertices.size() && b < vertices.size() && c < vertices.size())
            {
                localTriangles_.push_back({vertices[a].positionM, vertices[b].positionM, vertices[c].positionM});
            }
        }
        return;
    }

    localTriangles_.reserve(vertices.size() / 3U);
    for (std::size_t index = 0; index + 2U < vertices.size(); index += 3U)
    {
        localTriangles_.push_back({vertices[index + 0U].positionM,
                                   vertices[index + 1U].positionM,
                                   vertices[index + 2U].positionM});
    }
}
} // namespace Kimgane::Engine
