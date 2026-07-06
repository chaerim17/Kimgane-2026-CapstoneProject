#include "Pch.h"

#include "Mesh.h"

#include <cstring>
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

    std::vector<Vertex> vertices;
    vertices.reserve(36);

    auto addFace = [&vertices](const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b,
                               const DirectX::XMFLOAT3& c, const DirectX::XMFLOAT3& d)
    {
        vertices.push_back({a});
        vertices.push_back({b});
        vertices.push_back({c});
        vertices.push_back({a});
        vertices.push_back({c});
        vertices.push_back({d});
    };

    addFace(leftUpFront, rightUpFront, rightDownFront, leftDownFront);
    addFace(leftUpBack, rightUpBack, rightUpFront, leftUpFront);
    addFace(leftDownBack, rightDownBack, rightUpBack, leftUpBack);
    addFace(leftDownFront, rightDownFront, rightDownBack, leftDownBack);
    addFace(leftUpBack, leftUpFront, leftDownFront, leftDownBack);
    addFace(rightUpFront, rightUpBack, rightDownBack, rightDownFront);

    auto mesh = std::shared_ptr<Mesh>(new Mesh());
    mesh->CreateVertexBuffer(device, vertices);
    mesh->CreateBounds(vertices);
    return mesh;
}

void Mesh::Render(ID3D12GraphicsCommandList& commandList) const noexcept
{
    if (vertexCount_ == 0)
    {
        return;
    }

    commandList.IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    commandList.IASetVertexBuffers(0, 1, &vertexBufferView_);
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

void Mesh::CreateVertexBuffer(ID3D12Device& device, const std::vector<Vertex>& vertices)
{
    const UINT bufferSizeBytes = static_cast<UINT>(vertices.size() * sizeof(Vertex));
    if (bufferSizeBytes == 0)
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
} // namespace Kimgane::Engine
