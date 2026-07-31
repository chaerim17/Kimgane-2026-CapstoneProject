#include "ObjLoader.h"

#include "../IO/AssetPathResolver.h"

#include <cmath>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace Kimgane::Shared::Geometry
{
namespace
{

struct ObjFaceVertex
{
    int positionIndex = -1;
    int normalIndex = -1;
};

struct ParsedObjData
{
    std::vector<Vec3> positionsM;
    std::vector<Vec3> normals;
    std::vector<std::vector<ObjFaceVertex>> faces;
};

struct PositionNormal
{
    Vec3 positionM;
    Vec3 normal;
};

std::filesystem::path ResolveObjPath(const std::filesystem::path& filePath) // obj 파일 경로 확인
{
    if (filePath.extension() == ".obj")
    {
        return Kimgane::Shared::IO::ResolveAssetPath(filePath);
    }

    std::filesystem::path objPath = filePath;
    objPath += ".obj";
    return Kimgane::Shared::IO::ResolveAssetPath(objPath);
}

ObjFaceVertex ParseFaceVertexToken(const std::string& token) // f v/vt/vn 형식의 토큰을 파싱하여 ObjFaceVertex 구조체로 변환
{
    ObjFaceVertex vertex;
    const std::size_t firstSlash = token.find('/');
    vertex.positionIndex = std::stoi(token.substr(0, firstSlash)) - 1;

    if (firstSlash == std::string::npos)
    {
        return vertex;
    }

    const std::size_t secondSlash = token.find('/', firstSlash + 1);
    if (secondSlash == std::string::npos)
    {
        return vertex;
    }

    const std::string normalPart = token.substr(secondSlash + 1);
    if (!normalPart.empty())
    {
        vertex.normalIndex = std::stoi(normalPart) - 1;
    }

    return vertex;
}

ParsedObjData ParseObjFile(const std::filesystem::path& objPath) // obj 파일을 파싱하여 ParsedObjData 구조체로 반환
{
    std::ifstream stream(objPath);
    if (!stream)
    {
        throw std::runtime_error("Failed to open OBJ file : " + objPath.string());
    }

    ParsedObjData data;
    std::string line;
    while (std::getline(stream, line))
    {
        std::istringstream lineStream(line);
        std::string keyword;
        lineStream >> keyword;

        if (keyword == "v")
        {
            Vec3 positionM = {};
            lineStream >> positionM.x >> positionM.y >> positionM.z;
            data.positionsM.push_back(positionM);
        }
        else if (keyword == "vn")
        {
            Vec3 normal = {};
            lineStream >> normal.x >> normal.y >> normal.z;
            data.normals.push_back(normal);
        }
        else if (keyword == "f")
        {
            std::vector<ObjFaceVertex> faceVertices;
            std::string token;
            while (lineStream >> token)
            {
                faceVertices.push_back(ParseFaceVertexToken(token));
            }
            data.faces.push_back(std::move(faceVertices));
        }
    }

    return data;
}

Vec3 ConvertToEngineAxis(const Vec3& value) noexcept // obj 좌표계를 엔진 좌표계로 변환
{
    return {value.x, value.y, -value.z};
}

Vec3 Subtract(const Vec3& lhs, const Vec3& rhs) noexcept
{
    return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

Vec3 Cross(const Vec3& lhs, const Vec3& rhs) noexcept
{
    return {lhs.y * rhs.z - lhs.z * rhs.y,
            lhs.z * rhs.x - lhs.x * rhs.z,
            lhs.x * rhs.y - lhs.y * rhs.x};
}

Vec3 Normalize(const Vec3& value) noexcept
{
    const float length = std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
    if (length <= 0.0F)
    {
        return value;
    }

    return {value.x / length, value.y / length, value.z / length};
}

Vec3 ComputeFaceNormal(const Vec3& a, const Vec3& b, const Vec3& c) noexcept // 노멀이 없는 경우 삼각형의 법선 벡터를 계산
{
    return Normalize(Cross(Subtract(b, a), Subtract(c, a)));
}

PositionNormal BuildPositionNormal(const ParsedObjData& data,
                                   const ObjFaceVertex& faceVertex,
                                   const Vec3& fallbackNormal) // obj 파일의 faceVertex를 position/normal 쌍으로 변환
{
    const Vec3 positionM = ConvertToEngineAxis(data.positionsM[static_cast<std::size_t>(faceVertex.positionIndex)]);

    const Vec3 normal = faceVertex.normalIndex >= 0 ? ConvertToEngineAxis(data.normals[static_cast<std::size_t>(faceVertex.normalIndex)]) : fallbackNormal;

    return {positionM, normal};
}

ObjGeometryData BuildMeshData(const ParsedObjData& data) // ParsedObjData를 기반으로 ObjGeometryData를 생성,
                                                         // 정점 중복 제거, 삼각분할, 좌표계 반전에 맞춘 winding 순서 뒤집기
{
    ObjGeometryData meshData;
    std::unordered_map<std::int64_t, std::uint32_t> vertexCache;

    for (const std::vector<ObjFaceVertex>& face : data.faces)
    {
        if (face.size() < 3)
        {
            continue;
        }

        Vec3 fallbackNormal = {0.0F, 1.0F, 0.0F};
        if (face[0].normalIndex < 0)
        {
            fallbackNormal = ComputeFaceNormal(data.positionsM[static_cast<std::size_t>(face[0].positionIndex)],
                                               data.positionsM[static_cast<std::size_t>(face[1].positionIndex)],
                                               data.positionsM[static_cast<std::size_t>(face[2].positionIndex)]);
        }

        std::vector<std::uint32_t> faceIndices;
        faceIndices.reserve(face.size());
        for (const ObjFaceVertex& faceVertex : face)
        {
            if (faceVertex.normalIndex < 0)
            {
                faceIndices.push_back(static_cast<std::uint32_t>(meshData.positionsM.size()));
                const PositionNormal positionNormal = BuildPositionNormal(data, faceVertex, fallbackNormal);
                meshData.positionsM.push_back(positionNormal.positionM);
                meshData.normals.push_back(positionNormal.normal);
                continue;
            }

            const std::int64_t key = (static_cast<std::int64_t>(faceVertex.positionIndex) << 32) |
                                     static_cast<std::uint32_t>(faceVertex.normalIndex);
            const auto cached = vertexCache.find(key);
            if (cached != vertexCache.end())
            {
                faceIndices.push_back(cached->second);
                continue;
            }

            const std::uint32_t newIndex = static_cast<std::uint32_t>(meshData.positionsM.size());
            const PositionNormal positionNormal = BuildPositionNormal(data, faceVertex, fallbackNormal);
            meshData.positionsM.push_back(positionNormal.positionM);
            meshData.normals.push_back(positionNormal.normal);
            vertexCache.emplace(key, newIndex);
            faceIndices.push_back(newIndex);
        }

        for (std::size_t i = 1; i + 1 < faceIndices.size(); ++i)
        {
            meshData.indices.push_back(faceIndices[0]);
            meshData.indices.push_back(faceIndices[i + 1]);
            meshData.indices.push_back(faceIndices[i]);
        }
    }

    return meshData;
}

} // namespace

ObjGeometryData ObjLoader::Load(const std::filesystem::path& filePath) // obj 파일을 로드하여 순수 지오메트리 데이터를 생성
{
    const std::filesystem::path objPath = ResolveObjPath(filePath);
    if (objPath.empty())
    {
        throw std::runtime_error("OBJ file not found: " + filePath.string());
    }

    const ParsedObjData parsedData = ParseObjFile(objPath);
    if (parsedData.positionsM.empty() || parsedData.faces.empty())
    {
        throw std::runtime_error("OBJ file has no drawable geometry: " + objPath.string());
    }

    ObjGeometryData meshData = BuildMeshData(parsedData);
    if (meshData.positionsM.empty() || meshData.indices.empty())
    {
        throw std::runtime_error("OBJ file produced no drawable geometry: " + objPath.string());
    }

    return meshData;
}

} // namespace Kimgane::Shared::Geometry
