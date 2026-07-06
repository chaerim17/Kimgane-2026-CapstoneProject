#pragma once

#include <d3dcompiler.h>
#include <wrl/client.h>

namespace Kimgane::Engine
{
class ShaderCompiler final
{
public:
    static Microsoft::WRL::ComPtr<ID3DBlob> CompileFromSource(const char* source,
                                                              const char* entryPoint,
                                                              const char* target);

    ShaderCompiler() = delete;
};

class ShaderLibrary final
{
public:
    [[nodiscard]] static const char* GetLitColorShaderSource() noexcept;

    ShaderLibrary() = delete;
};
} // namespace Kimgane::Engine
