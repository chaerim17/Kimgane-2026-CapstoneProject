#include "Pch.h"

#include "Shader.h"

#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace Kimgane::Engine
{
namespace
{
constexpr wchar_t LIT_COLOR_SHADER_PATH[] = L"Assets/Shaders/LitColor.hlsl";

void ThrowIfFailed(HRESULT result)
{
    if (FAILED(result))
    {
        throw std::runtime_error("A shader compiler call failed.");
    }
}

std::filesystem::path GetModuleDirectory()
{
    wchar_t modulePath[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(nullptr, modulePath, _countof(modulePath));
    if (length == 0U || length == _countof(modulePath))
    {
        return {};
    }

    return std::filesystem::path(modulePath).parent_path();
}

std::vector<std::filesystem::path> BuildShaderSearchRoots()
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

std::filesystem::path ResolveShaderPath(const std::filesystem::path& filePath)
{
    if (filePath.is_absolute() && std::filesystem::exists(filePath))
    {
        return filePath;
    }

    if (std::filesystem::exists(filePath))
    {
        return std::filesystem::absolute(filePath);
    }

    for (const std::filesystem::path& root : BuildShaderSearchRoots())
    {
        const std::filesystem::path candidate = root / filePath;
        if (std::filesystem::exists(candidate))
        {
            return std::filesystem::absolute(candidate);
        }
    }

    return {};
}

UINT BuildCompileFlags() noexcept
{
    UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
    compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    return compileFlags;
}
} // namespace

Microsoft::WRL::ComPtr<ID3DBlob> ShaderCompiler::CompileFromSource(const char* source,
                                                                   const char* entryPoint,
                                                                   const char* target)
{
    Microsoft::WRL::ComPtr<ID3DBlob> shader;
    Microsoft::WRL::ComPtr<ID3DBlob> errors;
    const HRESULT result = D3DCompile(source,
                                      std::strlen(source),
                                      nullptr,
                                      nullptr,
                                      nullptr,
                                      entryPoint,
                                      target,
                                      BuildCompileFlags(),
                                      0,
                                      &shader,
                                      &errors);
    if (FAILED(result))
    {
        if (errors != nullptr)
        {
            throw std::runtime_error(static_cast<const char*>(errors->GetBufferPointer()));
        }

        ThrowIfFailed(result);
    }

    return shader;
}

Microsoft::WRL::ComPtr<ID3DBlob> ShaderCompiler::CompileFromFile(const std::filesystem::path& filePath,
                                                                 const char* entryPoint,
                                                                 const char* target)
{
    const std::filesystem::path resolvedPath = ResolveShaderPath(filePath);
    if (resolvedPath.empty())
    {
        throw std::runtime_error("Shader file not found: " + filePath.string());
    }

    Microsoft::WRL::ComPtr<ID3DBlob> shader;
    Microsoft::WRL::ComPtr<ID3DBlob> errors;
    const HRESULT result = D3DCompileFromFile(resolvedPath.c_str(),
                                             nullptr,
                                             D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                             entryPoint,
                                             target,
                                             BuildCompileFlags(),
                                             0,
                                             &shader,
                                             &errors);
    if (FAILED(result))
    {
        if (errors != nullptr)
        {
            throw std::runtime_error(static_cast<const char*>(errors->GetBufferPointer()));
        }

        ThrowIfFailed(result);
    }

    return shader;
}

std::filesystem::path ShaderLibrary::GetLitColorShaderPath()
{
    return LIT_COLOR_SHADER_PATH;
}
} // namespace Kimgane::Engine
