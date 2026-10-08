#include "LitColor.hlsl"

// t0 remains reserved for ordinary object materials. The contiguous terrain table
// contains 3 sRGB albedos, 3 BC5 normals, 3 linear AO/roughness/height maps, detail.
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
    // Reconstruct BC5 Z and convert tangent-space normal to a surface slope.
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
    float rockWeight = smoothstep(0.13, 0.48, 1.0 - geometricNormal.y);
    rockWeight = lerp(rockWeight, 1.0, SURFACE.z);
    float gravelWeight = smoothstep(0.43, 0.69, MoonNoise(p.xz / 11.0)) * (1.0 - rockWeight);
    float dustWeight = 1.0 - rockWeight - gravelWeight;

    float3 dustArh = DUST_ARH.Sample(TERRAIN_SAMPLER, p.xz / 5.0).rgb;
    float3 gravelArh = GRAVEL_ARH.Sample(TERRAIN_SAMPLER, p.xz / 3.0).rgb;
    float3 rockArh = Triplanar(ROCK_ARH, p / 7.0, axes);
    // Height modulates existing layer weights without introducing absent layers.
    float3 weights = float3(dustWeight, rockWeight, gravelWeight)
        * (0.65 + 0.7 * float3(dustArh.b, rockArh.b, gravelArh.b));
    weights /= max(dot(weights, float3(1, 1, 1)), 0.001);
    dustWeight = weights.x;
    rockWeight = weights.y;
    gravelWeight = weights.z;

    float3 dust = DUST_TEXTURE.Sample(TERRAIN_SAMPLER, p.xz / 5.0).rgb;
    float3 gravel = GRAVEL_TEXTURE.Sample(TERRAIN_SAMPLER, p.xz / 3.0).rgb;
    // Triplanar mapping is limited to the exposed rock layer.
    float3 rock = Triplanar(ROCK_TEXTURE, p / 7.0, axes);
    float3 albedo = dust * dustWeight + rock * rockWeight + gravel * gravelWeight;
    albedo *= lerp(float3(0.62, 0.72, 0.90), float3(1.12, 1.10, 1.06), macro);

    // Project small normal perturbations onto the geometric tangent plane.
    float2 dustSlope = NormalSlope(DUST_NORMAL, p.xz / 5.0);
    float2 gravelSlope = NormalSlope(GRAVEL_NORMAL, p.xz / 3.0);
    float3 materialSlope = float3(dustSlope.x, 0, dustSlope.y) * dustWeight
        + float3(gravelSlope.x, 0, gravelSlope.y) * gravelWeight
        + TriplanarSlope(ROCK_NORMAL, p / 7.0, axes) * rockWeight;
    float detailFade = 1.0 - smoothstep(12.0, 65.0, distanceM);
    float3 perturbation = materialSlope * 0.7 + TriplanarSlope(DETAIL_NORMAL_TEXTURE, p / 0.65, axes)
        * detailFade * lerp(0.18, 0.5, gravelWeight);
    perturbation -= geometricNormal * dot(perturbation, geometricNormal);
    float3 normalW = normalize(geometricNormal + perturbation);
    float3 arh = dustArh * dustWeight + rockArh * rockWeight + gravelArh * gravelWeight;

    float3 light = normalize(-LIGHT_DIRECTION_INTENSITY.xyz);
    float diffuse = saturate(dot(normalW, light));
    // Broad cool fill and gently stepped diffuse preserve the stylized crater shapes.
    float shapedDiffuse = lerp(diffuse, smoothstep(0.05, 0.92, diffuse), 0.3);
    float3 ambient = float3(0.56, 0.68, 0.94) * (LIGHT_COLOR_AMBIENT.a + 0.09 * saturate(normalW.y));
    float3 color = albedo * (ambient * arh.r + LIGHT_COLOR_AMBIENT.rgb * shapedDiffuse * LIGHT_DIRECTION_INTENSITY.a);
    float3 view = normalize(CAMERA_POSITION_SPECULAR_POWER.xyz - p);
    float3 halfDirection = normalize(light + view);
    float roughness = clamp(arh.g, 0.1, 1);
    // All three lunar materials are dielectric (metallic=0) and non-emissive.
    float specular = pow(saturate(dot(normalW, halfDirection)), lerp(96.0, 8.0, roughness))
        * 0.04 * (1.0 - roughness) * diffuse;
    color += specular * LIGHT_COLOR_AMBIENT.rgb * LIGHT_DIRECTION_INTENSITY.a;
    // Restrained distance haze, kept in the terrain shader so UI/props retain their material path.
    float haze = 0.26 * smoothstep(100.0, 450.0, distanceM);
    color = lerp(color, float3(0.12, 0.17, 0.26), haze);
    return float4(color, 1);
}
