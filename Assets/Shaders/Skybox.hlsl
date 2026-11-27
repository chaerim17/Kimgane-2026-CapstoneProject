cbuffer SkyboxConstants : register(b0)
{
    float4x4 CLIP_TO_WORLD;
};

TextureCube<float4> SKY_TEXTURE : register(t0);
SamplerState SKY_SAMPLER : register(s0);

struct SkyboxVertex
{
    float4 position : SV_POSITION;
    float3 directionW : TEXCOORD0;
};

SkyboxVertex VSMain(uint vertexId : SV_VertexID)
{
    // NDC의 세 점으로 화면 전체를 덮고 카메라 중심에서 나가는 광선을 만든다.
    const float2 ndc = float2((vertexId << 1) & 2u, vertexId & 2u) * 2.0F - 1.0F;
    SkyboxVertex output;
    output.position = float4(ndc, 1.0F, 1.0F);
    output.directionW = mul(CLIP_TO_WORLD, output.position).xyz;
    return output;
}

float4 PSMain(SkyboxVertex input) : SV_TARGET
{
    return float4(SKY_TEXTURE.Sample(SKY_SAMPLER, input.directionW).rgb, 1.0F);
}
