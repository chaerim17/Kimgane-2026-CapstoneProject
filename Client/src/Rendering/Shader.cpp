#include "Pch.h"

#include "Shader.h"

#include "../../Shared/IO/AssetPathResolver.h"

#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace Kimgane::Client
{
namespace
{
constexpr wchar_t LIT_COLOR_SHADER_PATH[] = L"Assets/Shaders/LitColor.hlsl";

void ThrowIfFailed(HRESULT result)
{
    if (FAILED(result))
    {
        throw std::runtime_error("셰이더 컴파일러 호출에 실패했습니다.");
    }
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
    const std::filesystem::path resolvedPath = Kimgane::Shared::IO::ResolveAssetPath(filePath);
    if (resolvedPath.empty())
    {
        throw std::runtime_error("셰이더 파일을 찾을 수 없습니다: " + filePath.string());
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
} // namespace Kimgane::Client
