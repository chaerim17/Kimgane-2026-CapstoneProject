#include "Pch.h"

#include "FbxModelMesh.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace Kimgane::Engine
{
namespace
{
struct ConvertedMeshData
{
    std::vector<Mesh::Vertex> vertices;
    std::vector<std::uint32_t> indices;
};

// 26.07.09 모델 파일 경로 탐색
std::filesystem::path GetModuleDirectory()
{
    wchar_t modulePath[MAX_PATH] = {}; // 유니코드로 받기 위해 wchar_t 사용
    const DWORD length = GetModuleFileNameW(
        nullptr, modulePath,
        _countof(
            modulePath)); // GetModuleFileNameW -> exe/dll의 전체 경로를 알려주는 함수, nullptr -> 지금 실행중인 exe

    if (length == 0U || length == _countof(modulePath)) // 예외처리
    {
        return {};
    }

    return std::filesystem::path(modulePath)
        .parent_path(); // parent_path()는 상위 폴더를 보내줌 C:\...\bin\x64\Debug\Kimgane.Client.exe ->
                        // C:\...\bin\x64\Debug
}

std::vector<std::filesystem::path> BuildModelSearchRoots()
{
    std::vector<std::filesystem::path> roots;
    roots.push_back(std::filesystem::current_path());

    std::filesystem::path moduleDirectory = GetModuleDirectory();
    if (!moduleDirectory.empty())
    {
        roots.push_back(moduleDirectory);
        std::filesystem::path current = moduleDirectory;
        for (int depth = 0; depth < 5 && current.has_parent_path(); ++depth)
        {
            current = current.parent_path();
            roots.push_back(current);
        }
    }

    return roots;
}
//-----------------------------------------------------------------

std::filesystem::path ResolveConvertedTextPath(const std::filesystem::path& filePath)
{
    if (filePath.extension() == ".txt" && std::filesystem::exists(filePath))
    {
        return std::filesystem::absolute(filePath);
    }

    std::vector<std::filesystem::path> candidates;
    candidates.push_back(filePath);

    std::filesystem::path fbxTextPath = filePath;
    fbxTextPath += ".txt";
    candidates.push_back(fbxTextPath);

    std::filesystem::path textPath = filePath;
    textPath.replace_extension(".txt");
    candidates.push_back(textPath);

    if (filePath.has_parent_path())
    {
        candidates.push_back(filePath.parent_path() / (filePath.stem().string() + ".fbx.txt"));
        candidates.push_back(filePath.parent_path() / (filePath.stem().string() + ".txt"));
    }

    for (const auto& candidate : candidates)
    {
        if (std::filesystem::exists(candidate))
        {
            return std::filesystem::absolute(candidate);
        }
    }

    for (const std::filesystem::path& root : BuildModelSearchRoots())
    {   
        for (const auto& candidate : candidates)
        {
            const std::filesystem::path combined = root / candidate;
            if (std::filesystem::exists(combined))
            {
                return std::filesystem::absolute(combined);
            }
        }
    }

    return {};
}

void SkipFloatValues(std::ifstream& stream, int count)
{
    float ignored = 0.0F;
    for (int index = 0; index < count; ++index)
    {
        stream >> ignored;
    }
}

void SkipConvertedBlock(std::ifstream& stream, const std::string& closingToken)
{
    std::string token;
    while (stream >> token)
    {
        if (token == closingToken)
        {
            break;
        }
    }
}

DirectX::XMMATRIX ReadConvertedTransformMatrix(std::ifstream& stream)
{
    DirectX::XMFLOAT4X4 transform = {};
    stream >> transform._11 >> transform._12 >> transform._13 >> transform._14;
    stream >> transform._21 >> transform._22 >> transform._23 >> transform._24;
    stream >> transform._31 >> transform._32 >> transform._33 >> transform._34;
    stream >> transform._41 >> transform._42 >> transform._43 >> transform._44;
    return DirectX::XMLoadFloat4x4(&transform);
}

DirectX::XMFLOAT3 TransformPosition(const DirectX::XMFLOAT3& positionM, DirectX::CXMMATRIX matrix) noexcept
{
    DirectX::XMFLOAT3 result = {};
    DirectX::XMStoreFloat3(&result,
                           DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&positionM), matrix));
    return result;
}

DirectX::XMFLOAT3 TransformNormal(const DirectX::XMFLOAT3& normal, DirectX::CXMMATRIX matrix) noexcept
{
    DirectX::XMVECTOR transformed = DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&normal), matrix);
    if (DirectX::XMVectorGetX(DirectX::XMVector3LengthSq(transformed)) <= 0.000001F)
    {
        return {0.0F, 1.0F, 0.0F};
    }

    DirectX::XMFLOAT3 result = {};
    DirectX::XMStoreFloat3(&result, DirectX::XMVector3Normalize(transformed));
    return result;
}

DirectX::XMFLOAT4 BuildFrameColor(const std::string& frameName,
                                  const FbxModelMeshLoadOptions& options) noexcept
{
    std::uint32_t hash = 2166136261U;
    for (const char character : frameName)
    {
        hash ^= static_cast<std::uint8_t>(character);
        hash *= 16777619U;
    }

    const float r = 0.55F + static_cast<float>((hash >> 0U) & 0x3FU) / 255.0F;
    const float g = 0.55F + static_cast<float>((hash >> 6U) & 0x3FU) / 255.0F;
    const float b = 0.55F + static_cast<float>((hash >> 12U) & 0x3FU) / 255.0F;
    if (frameName.empty())
    {
        return options.defaultColorLinear;
    }

    return {std::min(r, 1.0F), std::min(g, 1.0F), std::min(b, 1.0F), 1.0F};
}

void AppendConvertedMesh(std::ifstream& stream,
                         const std::string& frameName,
                         DirectX::CXMMATRIX frameWorld,
                         const FbxModelMeshLoadOptions& options,
                         ConvertedMeshData& meshData)
{
    int declaredVertexCount = 0;
    std::string meshName;
    stream >> declaredVertexCount >> meshName;
    (void)declaredVertexCount;

    std::vector<DirectX::XMFLOAT3> positionsM;
    std::vector<DirectX::XMFLOAT3> normals;
    std::vector<std::uint32_t> meshIndices;

    std::string token;
    while (stream >> token)
    {
        if (token == "<Positions>:")
        {
            int count = 0;
            stream >> count;
            positionsM.resize(static_cast<std::size_t>(std::max(count, 0)));
            for (DirectX::XMFLOAT3& positionM : positionsM)
            {
                stream >> positionM.x >> positionM.y >> positionM.z;
            }
        }
        else if (token == "<Normals>:")
        {
            int count = 0;
            stream >> count;
            normals.resize(static_cast<std::size_t>(std::max(count, 0)));
            for (DirectX::XMFLOAT3& normal : normals)
            {
                stream >> normal.x >> normal.y >> normal.z;
            }
        }
        else if (token == "<Colors>:")
        {
            int count = 0;
            stream >> count;
            SkipFloatValues(stream, std::max(count, 0) * 4);
        }
        else if (token == "<Indices>:")
        {
            int count = 0;
            stream >> count;
            meshIndices.reserve(meshIndices.size() + static_cast<std::size_t>(std::max(count, 0)));
            for (int index = 0; index < count; ++index)
            {
                std::uint32_t value = 0;
                stream >> value;
                meshIndices.push_back(value);
            }
        }
        else if (token == "<SubMeshes>:")
        {
            int subMeshCount = 0;
            stream >> subMeshCount;
            for (int subMeshIndex = 0; subMeshIndex < subMeshCount; ++subMeshIndex)
            {
                std::string subMeshToken;
                std::string subsetName;
                int indexCount = 0;
                stream >> subMeshToken >> subsetName >> indexCount;
                if (subMeshToken != "<SubMesh>:")
                {
                    continue;
                }

                meshIndices.reserve(meshIndices.size() + static_cast<std::size_t>(std::max(indexCount, 0)));
                for (int index = 0; index < indexCount; ++index)
                {
                    std::uint32_t value = 0;
                    stream >> value;
                    meshIndices.push_back(value);
                }
            }
        }
        else if (token == "</Mesh>")
        {
            break;
        }
        else if (token == "<Bounds>:")
        {
            SkipFloatValues(stream, 6);
        }
    }

    if (positionsM.empty())
    {
        return;
    }

    if (normals.size() != positionsM.size())
    {
        normals.assign(positionsM.size(), {0.0F, 1.0F, 0.0F});
    }

    if (meshIndices.empty())
    {
        meshIndices.reserve(positionsM.size());
        for (std::size_t index = 0; index < positionsM.size(); ++index)
        {
            meshIndices.push_back(static_cast<std::uint32_t>(index));
        }
    }

    const std::uint32_t baseIndex = static_cast<std::uint32_t>(meshData.vertices.size());
    const DirectX::XMFLOAT4 frameColor = BuildFrameColor(frameName.empty() ? meshName : frameName, options);
    meshData.vertices.reserve(meshData.vertices.size() + positionsM.size());
    for (std::size_t index = 0; index < positionsM.size(); ++index)
    {
        meshData.vertices.push_back({TransformPosition(positionsM[index], frameWorld),
                                     TransformNormal(normals[index], frameWorld),
                                     frameColor});
    }

    meshData.indices.reserve(meshData.indices.size() + meshIndices.size());
    for (std::uint32_t sourceIndex : meshIndices)
    {
        if (sourceIndex < positionsM.size())
        {
            meshData.indices.push_back(baseIndex + sourceIndex);
        }
    }
}

void ReadConvertedFrame(std::ifstream& stream,
                        DirectX::CXMMATRIX parentWorld,
                        const FbxModelMeshLoadOptions& options,
                        ConvertedMeshData& meshData)
{
    int frameIndex = 0;
    std::string frameName;
    stream >> frameIndex >> frameName;
    (void)frameIndex;

    DirectX::XMMATRIX frameWorld = parentWorld;
    std::string token;
    while (stream >> token)
    {
        if (token == "<Transform>:")
        {
            SkipFloatValues(stream, 13);
        }
        else if (token == "<TransformMatrix>:")
        {
            frameWorld = ReadConvertedTransformMatrix(stream) * parentWorld;
        }
        else if (token == "<Mesh>:")
        {
            AppendConvertedMesh(stream, frameName, frameWorld, options, meshData);
        }
        else if (token == "<Materials>:")
        {
            SkipConvertedBlock(stream, "</Materials>");
        }
        else if (token == "<Children>:")
        {
            int childCount = 0;
            stream >> childCount;
            for (int childIndex = 0; childIndex < childCount; ++childIndex)
            {
                std::string childToken;
                stream >> childToken;
                if (childToken == "<Frame>:")
                {
                    ReadConvertedFrame(stream, frameWorld, options, meshData);
                }
            }
        }
        else if (token == "</Frame>")
        {
            break;
        }
    }
}

ConvertedMeshData LoadConvertedTextMesh(const std::filesystem::path& convertedTextPath,
                                        const FbxModelMeshLoadOptions& options)
{
    std::ifstream stream(convertedTextPath);
    if (!stream)
    {
        throw std::runtime_error("Failed to open converted FBX text mesh: " + convertedTextPath.string());
    }

    ConvertedMeshData meshData;
    std::string token;
    while (stream >> token)
    {
        if (token == "<Frame>:")
        {
            ReadConvertedFrame(stream, DirectX::XMMatrixIdentity(), options, meshData);
        }
    }

    return meshData;
}
} // namespace

std::shared_ptr<Mesh> FbxModelMesh::Load(ID3D12Device& device,
                                         const std::filesystem::path& filePath,
                                         const FbxModelMeshLoadOptions& options)
{
    const std::filesystem::path convertedTextPath = ResolveConvertedTextPath(filePath);
    if (convertedTextPath.empty())
    {
        throw std::runtime_error("FBX mesh requires a converted .fbx.txt or .txt file: " + filePath.string());
    }

    ConvertedMeshData meshData = LoadConvertedTextMesh(convertedTextPath, options);
    if (meshData.vertices.empty() || meshData.indices.empty())
    {
        throw std::runtime_error("Converted FBX mesh has no drawable geometry: " + convertedTextPath.string());
    }

    return Mesh::Create(device, meshData.vertices, meshData.indices);
}
} // namespace Kimgane::Engine
