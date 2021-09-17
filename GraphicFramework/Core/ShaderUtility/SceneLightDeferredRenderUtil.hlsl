#include "LightingUtil.hlsl"

struct GBufferResult
{
    float3 posW;
    float3 N;
    float4 diffuse;
    float4 specular;
    float2 uv;
    int    modelId;
    int    instanceId;
};

struct ShadingResult
{
    float3 ambient;
    float3 diffuse;
    float3 specular;
    float alpha;
};

struct VSInput
{
    float3 position : POSITION;
    float2 texcoord0 : TEXCOORD;
};

cbuffer cb_PerFrame : register(b0, space1)
{
    float4 g_EyePosW;
    float4 g_AmbientLight;
    uint   g_LightRenderCount;
    uint   g_TexWidth;
    uint   g_TexHeight;
    uint   Pad;
};

Texture2D<float4> g_TexPos : register(t0, space1);
Texture2D<float4> g_TexNorm : register(t1, space1);
Texture2D<float4> g_TexDiff : register(t2, space1);
Texture2D<float4> g_TexSpec : register(t3, space1);
Texture2D<int4>   g_TexId : register(t4, space1);

StructuredBuffer<LightData> g_LightData : register(t5, space1);
StructuredBuffer<uint>      g_LightRenderIndeces : register(t6, space1);



uint2 calcPixelPos(float2 tex)
{
    return uint2(tex.x * g_TexWidth, tex.y * g_TexHeight);
}

GBufferResult prepareGBufferResult(float2 tex)
{
    uint2 pixelPos = calcPixelPos(tex);
    
    GBufferResult gb;
    gb.posW = g_TexPos[pixelPos].rgb;
    gb.N = decodeNormal(g_TexNorm[pixelPos].ba);
    gb.diffuse = g_TexDiff[pixelPos].rgba;
    gb.specular = g_TexSpec[pixelPos].rgba;
    gb.uv = tex;
    int2 IdPair = g_TexId[pixelPos].rg;
    gb.instanceId = IdPair.r;
    gb.modelId = IdPair.g >> 16;   

    return gb;
}

ShadingResult deferredBlinnPhongShading(GBufferResult gb)
{ 
    float3 V = normalize(g_EyePosW.xyz - gb.posW);

    MeshMaterial mat = { gb.diffuse, gb.specular.rgb, gb.specular.a };

    float3 D = { 0.0f, 0.0f, 0.0f };
    float3 S = { 0.0f, 0.0f, 0.0f };
    LightSample lightSample;
    for (uint i = 0; i < g_LightRenderCount; i++)
    {
        lightSample = evalLight(g_LightData[g_LightRenderIndeces[i]], gb.posW, gb.N, V);
        evalBlinnPhong(lightSample, gb.N, V, mat, D, S);
    }

    ShadingResult shadingResult;
    shadingResult.ambient = g_AmbientLight.rgb * gb.diffuse.rgb;
    shadingResult.diffuse = D;
    shadingResult.specular = S;
    shadingResult.alpha = gb.diffuse.a;

    return shadingResult;
}

PbrParameter prepareDeferredPbrParameter(GBufferResult gb)
{
    float3 V = normalize(g_EyePosW.xyz - gb.posW);

    PbrParameter pp = initPbrParameter();
    pp.albedo = gb.diffuse;
    pp.posW = gb.posW;
    pp.uv = gb.uv;
    pp.N = gb.N;
    pp.V = V;
    
    // Sample the spec texture
    float3 spec = gb.specular.rgb;
    // R - Occlusion; G - Roughness; B - Metalness
    pp.diffuse = lerp(gb.diffuse.rgb, float3(0.0f, 0.0f, 0.0f), spec.b);
    // UE4 uses 0.08 multiplied by a default specular value of 0.5 as a base, hence the 0.04
    pp.specular = lerp(float3(0.04f, 0.04f, 0.04f), gb.diffuse.rgb, spec.b);
    pp.linearRoughness = spec.g;
    pp.linearRoughness = max(0.08, pp.linearRoughness); // Clamp the roughness so that the BRDF won't explode
    pp.roughness = pp.linearRoughness * pp.linearRoughness;
    pp.occlusion = spec.r;

    pp.NdotV = dot(pp.N, pp.V);

    return pp;
}

ShadingResult deferredPbrShading(GBufferResult gb)
{
    PbrParameter pbrParameter = prepareDeferredPbrParameter(gb);

    LightSample lightSample;
    PbrResult pbrResult;
    ShadingResult shadingResult;
    shadingResult.diffuse = float3(0.0f, 0.0f, 0.0f);
    shadingResult.specular = float3(0.0f, 0.0f, 0.0f);
    for (uint i = 0; i < g_LightRenderCount; i++)
    {
        lightSample = evalLight(g_LightData[g_LightRenderIndeces[i]], pbrParameter.posW, pbrParameter.N, pbrParameter.V);
        pbrResult = evalPBR(pbrParameter, lightSample);
        shadingResult.diffuse += pbrResult.diffuse;
        shadingResult.specular += pbrResult.specular;
    }
   
    shadingResult.ambient = g_AmbientLight.rgb * pbrParameter.albedo.rgb * (1.0f - pbrParameter.occlusion);
    shadingResult.alpha = pbrParameter.albedo.a;

    return shadingResult;
}