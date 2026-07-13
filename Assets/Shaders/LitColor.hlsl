cbuffer SceneConstants : register(b0)
{
    float4x4 VIEW_PROJECTION;
    float4 LIGHT_DIRECTION_INTENSITY;
    float4 LIGHT_COLOR_AMBIENT;
    float4 CAMERA_POSITION_SPECULAR_POWER;
};

cbuffer ObjectConstants : register(b1)
{
    float4x4 WORLD;
    float4 BASE_COLOR;
    float4 SURFACE;
    float4 EMISSION;
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
    float3 positionW : TEXCOORD0;
    float3 normalW : NORMAL;
    float4 colorLinear : COLOR;
};

PSInput VSMain(VSInput input)
{
    PSInput output;
    const float4 worldPosition = mul(WORLD, float4(input.positionM, 1.0F));
    output.position = mul(VIEW_PROJECTION, worldPosition);
    output.positionW = worldPosition.xyz;
    output.normalW = normalize(mul((float3x3)WORLD, input.normal));
    output.colorLinear = input.colorLinear;
    return output;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    const float3 normalW = normalize(input.normalW);
    const float3 lightDirectionW = normalize(-LIGHT_DIRECTION_INTENSITY.xyz);
    const float3 viewDirectionW = normalize(CAMERA_POSITION_SPECULAR_POWER.xyz - input.positionW);
    const float3 reflectionDirectionW = reflect(-lightDirectionW, normalW);

    const float diffuse = saturate(dot(normalW, lightDirectionW));
    const float specularPower = max(CAMERA_POSITION_SPECULAR_POWER.w, 1.0F);
    const float roughness = saturate(SURFACE.y);
    const float specularStrength = (1.0F - roughness) * 0.45F;
    const float specular = pow(saturate(dot(viewDirectionW, reflectionDirectionW)), specularPower) * specularStrength;

    const float3 albedo = input.colorLinear.rgb * BASE_COLOR.rgb;
    const float3 ambientColor = albedo * LIGHT_COLOR_AMBIENT.rgb * LIGHT_COLOR_AMBIENT.a;
    const float3 diffuseColor = albedo * LIGHT_COLOR_AMBIENT.rgb * diffuse * LIGHT_DIRECTION_INTENSITY.a;
    const float3 specularColor = LIGHT_COLOR_AMBIENT.rgb * specular * LIGHT_DIRECTION_INTENSITY.a;
    const float3 emissiveColor = EMISSION.rgb * EMISSION.a;
    const float3 litColor = ambientColor + diffuseColor + specularColor + emissiveColor;
    return float4(litColor, input.colorLinear.a * BASE_COLOR.a);
}
