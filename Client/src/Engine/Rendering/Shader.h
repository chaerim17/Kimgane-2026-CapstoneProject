#pragma once

#include <d3dcompiler.h>
#include <wrl/client.h>

#include <filesystem>

namespace Kimgane::Engine
{
class ShaderCompiler final
{
public:
    static Microsoft::WRL::ComPtr<ID3DBlob> CompileFromSource(const char* source,
                                                              const char* entryPoint,
                                                              const char* target);
    static Microsoft::WRL::ComPtr<ID3DBlob> CompileFromFile(const std::filesystem::path& filePath,
                                                            const char* entryPoint,
                                                            const char* target);

    ShaderCompiler() = delete;
};

class ShaderLibrary final
{
public:
    [[nodiscard]] static std::filesystem::path GetLitColorShaderPath();

    ShaderLibrary() = delete;
};
} // namespace Kimgane::Engine
