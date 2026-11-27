Texture2D<float> WORLD_DEPTH : register(t0);
Texture2D<float> TARGET_DEPTH : register(t1);

cbuffer DetectionPulseConstants : register(b0)
{
    float4 PULSE_PARAMS;
};

static const float DEPTH_EPSILON = 0.000001F;
static const float3 HIGHLIGHT_COLOR = float3(1.0F, 0.10F, 0.10F);
static const int2 OUTLINE_OFFSETS[12] = {
    int2(-2, 0), int2(2, 0), int2(0, -2), int2(0, 2),
    int2(-1, 0), int2(1, 0), int2(0, -1), int2(0, 1),
    int2(-1, -1), int2(-1, 1), int2(1, -1), int2(1, 1)
};

float4 VSMain(uint vertexId : SV_VertexID) : SV_POSITION
{
    const float2 ndc = float2((vertexId << 1) & 2u, vertexId & 2u) * 2.0F - 1.0F;
    return float4(ndc, 0.0F, 1.0F);
}

bool IsOccluded(int2 pixel, float targetDepth)
{
    return targetDepth < 1.0F && targetDepth > WORLD_DEPTH.Load(int3(pixel, 0)) + DEPTH_EPSILON;
}

float4 PSMain(float4 position : SV_POSITION) : SV_TARGET
{
    const int2 pixel = int2(position.xy);
    const float targetDepth = TARGET_DEPTH.Load(int3(pixel, 0));
    float alpha = 0.0F;
    if (IsOccluded(pixel, targetDepth))
    {
        alpha = 0.25F;
    }
    // 가려지지 않은 적 표면과 지형이 없는 배경에는 테두리를 덮지 않는다.
    else if (targetDepth >= 1.0F && WORLD_DEPTH.Load(int3(pixel, 0)) < 1.0F)
    {
        uint width, height;
        TARGET_DEPTH.GetDimensions(width, height);
        const int2 lastPixel = int2(width, height) - 1;
        [unroll]
        for (int i = 0; i < 12; ++i)
        {
            const int2 neighbor = clamp(pixel + OUTLINE_OFFSETS[i], int2(0, 0), lastPixel);
            if (IsOccluded(neighbor, TARGET_DEPTH.Load(int3(neighbor, 0))))
                alpha = 0.9F;
        }
    }
    return float4(HIGHLIGHT_COLOR, alpha * PULSE_PARAMS.x);
}
