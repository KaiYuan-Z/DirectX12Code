#define DIFFUSE_FROSTBITE_BRDF
#include "../../Core/ShaderUtility/LightingUtil.hlsl"

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosV : POSITION;
    float2 TexC : TEXCOORD0;
};

struct ShadingResult
{
    float3 ambient;
    float3 diffuse;
    float3 specular;
    float3 sssDiffuse;
    float alpha;
};

//================================
// PBR Lighting Resources
//================================

cbuffer cbPerframe : register(b0)
{
    float4x4 g_InvProj;
    float3 g_CameraPosW;
    uint g_LightRenderCount;
    float3 g_AmbientLight;
    bool g_SDFEffectEnable;
};

Texture2D<float4> g_PositionMap : register(t0);
Texture2D<float4> g_NormalMap : register(t1);
Texture2D<float4> g_BaseColorMap : register(t2);
Texture2D<float4> g_SpecularMap : register(t3);
Texture2D<float4> g_ShadowAndSSSMap : register(t4);
StructuredBuffer<LightData> g_LightData : register(t5);
StructuredBuffer<uint> g_LightRenderIndeces : register(t6);

SamplerState g_samLinear : register(s0);

// Prepare the hit-point data
PbrParameter preparePbrParameter(float3 posW, float3 normal, float2 tex)
{
    PbrParameter pp = initPbrParameter();

    // Sample the diffuse texture and apply the alpha test
    float4 baseColor = g_BaseColorMap.Sample(g_samLinear, tex);
    //float4 baseColor = float4(0.75,0.63,0.8,1.0);
    pp.albedo = baseColor;
    clip(baseColor.a - 0.3f);

    pp.posW = posW;
    pp.uv = tex;
    pp.V = normalize(g_CameraPosW.xyz - posW);
    pp.N = normalize(normal);

    // Sample the spec texture
    float3 spec = g_SpecularMap.Sample(g_samLinear, tex).rgb;
    // R - Occlusion; G - Roughness; B - Metalness
    pp.diffuse = lerp(baseColor.rgb, float3(0.0f, 0.0f, 0.0f), spec.b);
    // UE4 uses 0.08 multiplied by a default specular value of 0.5 as a base, hence the 0.04
    pp.specular = lerp(float3(0.04f, 0.04f, 0.04f), baseColor.rgb, spec.b);
    pp.linearRoughness = spec.g;
    pp.linearRoughness = max(0.08, pp.linearRoughness); // Clamp the roughness so that the BRDF won't explode
    pp.roughness = pp.linearRoughness * pp.linearRoughness;
    pp.occlusion = spec.r;

    pp.NdotV = dot(pp.N, pp.V);

    return pp;
}

ShadingResult pbrShading(float3 posW, float3 normal, float2 tex)
{
    PbrParameter pbrParameter = preparePbrParameter(posW, normal, tex);

    LightSample lightSample;
    PbrResult pbrResult;
    ShadingResult shadingResult;
    shadingResult.diffuse = float3(0.0f, 0.0f, 0.0f);
    shadingResult.specular = float3(0.0f, 0.0f, 0.0f);
    for (uint i = 0; i < g_LightRenderCount; i++)
    {
        lightSample = evalLight(g_LightData[g_LightRenderIndeces[i]], posW, pbrParameter.N, pbrParameter.V);
        pbrResult = evalPBR(pbrParameter, lightSample);
        shadingResult.diffuse += pbrResult.diffuse;
        shadingResult.specular += pbrResult.specular;
        shadingResult.sssDiffuse = lightSample.diffuse * g_BaseColorMap.Sample(g_samLinear, tex).xyz;
    }
   
    shadingResult.ambient = g_AmbientLight.rgb * pbrParameter.albedo.rgb * (1.0f - pbrParameter.occlusion);
    shadingResult.alpha = pbrParameter.albedo.a;

    return shadingResult;
}

float4 main(VertexOut pin) : SV_Target
{
    float3 PosW = g_PositionMap.Sample(g_samLinear, pin.TexC).rgb;
    float3 NormW = g_NormalMap.Sample(g_samLinear, pin.TexC).rgb;
    ShadingResult shadingResult = pbrShading(PosW, NormW, pin.TexC);
    float3 color;
    if (g_SDFEffectEnable)
    {
        float3 directLight = shadingResult.diffuse + shadingResult.specular;
        float4 baseColor = g_BaseColorMap.Sample(g_samLinear, pin.TexC);
        color = shadingResult.ambient + directLight * g_ShadowAndSSSMap.Sample(g_samLinear, pin.TexC).a;
        color += g_ShadowAndSSSMap.Sample(g_samLinear, pin.TexC).rgb * shadingResult.sssDiffuse;
    }
    else
    {
        color = shadingResult.ambient + shadingResult.diffuse + shadingResult.specular;
    }

     // HDR tonemapping
    color = color / (color + float3(1.0f, 1.0f, 1.0f));
    // gamma correct
    color = pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));
    return float4(color, shadingResult.alpha);
}