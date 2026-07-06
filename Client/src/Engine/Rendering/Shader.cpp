#include "Pch.h"

#include "Shader.h"

#include <cstring>
#include <stdexcept>

namespace Kimgane::Engine
{
namespace
{
constexpr char kLitColorShaderSource[] = R"(
cbuffer SceneConstants : register(b0)
{
    float4x4 gViewProjection;
    float4 gLightDirectionIntensity;
    float4 gLightColorAmbient;
};

cbuffer ObjectConstants : register(b1)
{
    float4x4 gWorld;
    float4 gBaseColor;
};

struct VSInput
{
    float3 positionM : POSITION;
    float3 normal : NORMAL;
    float4 colorLinear : COLOR;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normalW : NORMAL;
    float4 colorLinear : COLOR;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    const float4 worldPosition = mul(gWorld, float4(input.positionM, 1.0f));
    output.position = mul(gViewProjection, worldPosition);
    output.normalW = normalize(mul((float3x3)gWorld, input.normal));
    output.colorLinear = input.colorLinear;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    const float3 normalW = normalize(input.normalW);
    const float3 lightDirectionW = normalize(-gLightDirectionIntensity.xyz);
    const float diffuse = saturate(dot(normalW, lightDirectionW));
    const float3 litColor =
        input.colorLinear.rgb * gBaseColor.rgb *
        (gLightColorAmbient.rgb * (gLightColorAmbient.a + diffuse * gLightDirectionIntensity.a));
    return float4(litColor, input.colorLinear.a * gBaseColor.a);
}
)";

void ThrowIfFailed(HRESULT result)
{
    if (FAILED(result))
    {
        throw std::runtime_error("A shader compiler call failed.");
    }
}
} // namespace

Microsoft::WRL::ComPtr<ID3DBlob> ShaderCompiler::CompileFromSource(const char* source,
                                                                   const char* entryPoint,
                                                                   const char* target)
{
    UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined(_DEBUG)
    compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    Microsoft::WRL::ComPtr<ID3DBlob> shader;
    Microsoft::WRL::ComPtr<ID3DBlob> errors;
    const HRESULT result = D3DCompile(source,
                                      std::strlen(source),
                                      nullptr,
                                      nullptr,
                                      nullptr,
                                      entryPoint,
                                      target,
                                      compileFlags,
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

const char* ShaderLibrary::GetLitColorShaderSource() noexcept
{
    return kLitColorShaderSource;
}
} // namespace Kimgane::Engine
