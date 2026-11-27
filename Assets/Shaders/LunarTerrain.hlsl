#include "LitColor.hlsl"

// t0은 일반 오브젝트 재질용으로 비워 둠. 지형 텍스처 테이블은 연속된 슬롯에
// sRGB 기본색 3장, BC5 노멀 3장, 선형 AO/거칠기/높이 맵 3장, 디테일 노멀 순으로 들어감.
SamplerState TERRAIN_SAMPLER : register(s1);
Texture2D DUST_TEXTURE : register(t1);
Texture2D ROCK_TEXTURE : register(t2);
Texture2D GRAVEL_TEXTURE : register(t3);
Texture2D DUST_NORMAL : register(t4);
Texture2D ROCK_NORMAL : register(t5);
Texture2D GRAVEL_NORMAL : register(t6);
Texture2D DUST_ARH : register(t7);
Texture2D ROCK_ARH : register(t8);
Texture2D GRAVEL_ARH : register(t9);
Texture2D DETAIL_NORMAL_TEXTURE : register(t10);
// Blender에서 만든 월드 공간 마스크. 선형 RGBA8이고, 0번 행이 맵의 -Z 쪽 가장자리.
Texture2D WORLD_SURFACE_MASK : register(t11);

float3 Triplanar(Texture2D tex, float3 p, float3 axes)
{
    return tex.Sample(TERRAIN_SAMPLER, p.zy).rgb * axes.x
        + tex.Sample(TERRAIN_SAMPLER, p.xz).rgb * axes.y
        + tex.Sample(TERRAIN_SAMPLER, p.xy).rgb * axes.z;
}

float2 NormalSlope(Texture2D tex, float2 uv)
{
    float2 xy = tex.Sample(TERRAIN_SAMPLER, uv).rg * 2 - 1;
    float2 mirrorSign = 1 - 2 * (floor(uv) - 2 * floor(uv * 0.5));
    // BC5에 없는 Z를 복원하고, 탄젠트 공간 노멀을 표면 기울기로 바꿈.
    return xy * mirrorSign / max(sqrt(saturate(1 - dot(xy, xy))), 0.25);
}

float3 TriplanarSlope(Texture2D tex, float3 p, float3 axes)
{
    float2 x = NormalSlope(tex, p.zy);
    float2 y = NormalSlope(tex, p.xz);
    float2 z = NormalSlope(tex, p.xy);
    return float3(0, x.y, x.x) * axes.x + float3(y.x, 0, y.y) * axes.y
        + float3(z.x, z.y, 0) * axes.z;
}

float MoonHash(float2 p)
{
    return frac(sin(dot(p, float2(127.1, 311.7))) * 43758.5453);
}

float MoonNoise(float2 p)
{
    float2 i = floor(p);
    float2 f = frac(p);
    f = f * f * (3.0 - 2.0 * f);
    return lerp(lerp(MoonHash(i), MoonHash(i + float2(1, 0)), f.x),
                lerp(MoonHash(i + float2(0, 1)), MoonHash(i + 1), f.x), f.y);
}

float4 PSTerrain(PSInput input) : SV_TARGET
{
    float3 p = input.positionW;
    float3 geometricNormal = normalize(input.normalW);
    float distanceM = length(CAMERA_POSITION_SPECULAR_POWER.xyz - p);
    float3 axes = pow(abs(geometricNormal), 4);
    axes /= max(dot(axes, float3(1, 1, 1)), 0.0001);

    float macro = MoonNoise(p.xz / 67.0) * 0.65 + MoonNoise(p.xz / 23.0 + 9.0) * 0.35;
    float3 authored = WORLD_SURFACE_MASK.Sample(TERRAIN_SAMPLER, saturate((p.xz + 192.0) / 384.0)).rgb;
    float roadWeight = authored.r * (1.0 - SURFACE.z);
    float rockWeight = max(smoothstep(0.13, 0.48, 1.0 - geometricNormal.y), authored.b * 0.85);
    rockWeight *= 1.0 - roadWeight;
    rockWeight = lerp(rockWeight, 1.0, SURFACE.z);
    float gravelWeight = max(authored.g, roadWeight * 0.9) * (1.0 - rockWeight);
    float dustWeight = 1.0 - rockWeight - gravelWeight;

    float3 dustArh = DUST_ARH.Sample(TERRAIN_SAMPLER, p.xz / 5.0).rgb;
    float3 gravelArh = GRAVEL_ARH.Sample(TERRAIN_SAMPLER, p.xz / 3.0).rgb;
    float3 rockArh = Triplanar(ROCK_ARH, p / 7.0, axes);
    // 높이 맵으로 레이어 비중을 조절함. 비중이 0인 레이어가 새로 생기지는 않음.
    float3 weights = float3(dustWeight, rockWeight, gravelWeight)
        * (0.65 + 0.7 * float3(dustArh.b, rockArh.b, gravelArh.b));
    weights /= max(dot(weights, float3(1, 1, 1)), 0.001);
    dustWeight = weights.x;
    rockWeight = weights.y;
    gravelWeight = weights.z;

    float3 dust = DUST_TEXTURE.Sample(TERRAIN_SAMPLER, p.xz / 5.0).rgb;
    float3 gravel = GRAVEL_TEXTURE.Sample(TERRAIN_SAMPLER, p.xz / 3.0).rgb;
    // 트라이플래너 매핑은 드러난 바위 레이어에만 씀.
    float3 rock = Triplanar(ROCK_TEXTURE, p / 7.0, axes);
    float3 albedo = dust * dustWeight + rock * rockWeight + gravel * gravelWeight;
    albedo *= lerp(float3(0.62, 0.72, 0.90), float3(1.12, 1.10, 1.06), macro);
    // 다져지고 먼지 덮인 길이 멀리서도 구분되게 함.
    albedo = lerp(albedo, gravel * float3(0.39, 0.43, 0.49), roadWeight * 0.88);

    // 작은 노멀 흔들림을 도형의 접평면 위로 투영함.
    float2 dustSlope = NormalSlope(DUST_NORMAL, p.xz / 5.0);
    float2 gravelSlope = NormalSlope(GRAVEL_NORMAL, p.xz / 3.0);
    float3 materialSlope = float3(dustSlope.x, 0, dustSlope.y) * dustWeight
        + float3(gravelSlope.x, 0, gravelSlope.y) * gravelWeight
        + TriplanarSlope(ROCK_NORMAL, p / 7.0, axes) * rockWeight;
    float detailFade = 1.0 - smoothstep(12.0, 65.0, distanceM);
    float3 perturbation = materialSlope * 0.7 + TriplanarSlope(DETAIL_NORMAL_TEXTURE, p / 0.65, axes)
        * detailFade * lerp(0.18, 0.5, gravelWeight);
    perturbation -= geometricNormal * dot(perturbation, geometricNormal);
    float3 normalW = normalize(geometricNormal + perturbation * lerp(1.0, 0.35, roadWeight));
    float3 arh = dustArh * dustWeight + rockArh * rockWeight + gravelArh * gravelWeight;

    float3 light = normalize(-LIGHT_DIRECTION_INTENSITY.xyz);
    float diffuse = saturate(dot(normalW, light));
    // 넓고 차가운 보조광과 살짝 단계를 준 확산광으로 분화구의 스타일 있는 형태를 살림.
    float shapedDiffuse = lerp(diffuse, smoothstep(0.05, 0.92, diffuse), 0.3);
    float3 ambient = float3(0.56, 0.68, 0.94) * (LIGHT_COLOR_AMBIENT.a + 0.09 * saturate(normalW.y));
    float3 color = albedo * (ambient * arh.r + LIGHT_COLOR_AMBIENT.rgb * shapedDiffuse * LIGHT_DIRECTION_INTENSITY.a);
    float3 view = normalize(CAMERA_POSITION_SPECULAR_POWER.xyz - p);
    float3 halfDirection = normalize(light + view);
    float roughness = clamp(arh.g, 0.1, 1);
    // 달 재질 3종은 모두 비금속(metallic=0)이고 스스로 빛나지 않음.
    float specular = pow(saturate(dot(normalW, halfDirection)), lerp(96.0, 8.0, roughness))
        * 0.04 * (1.0 - roughness) * diffuse;
    color += specular * LIGHT_COLOR_AMBIENT.rgb * LIGHT_DIRECTION_INTENSITY.a;
    // 거리 안개를 약하게 넣음. 지형 셰이더에만 둬서 UI와 소품은 원래 재질 그대로 그려짐.
    float haze = 0.26 * smoothstep(100.0, 450.0, distanceM);
    color = lerp(color, float3(0.12, 0.17, 0.26), haze);
    return float4(color, 1);
}
