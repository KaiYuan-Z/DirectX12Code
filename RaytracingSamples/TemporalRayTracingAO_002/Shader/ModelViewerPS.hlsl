#define DIFFUSE_FROSTBITE_BRDF

#include "../../../GraphicFramework/Core/ShaderUtility/SceneLightDeferredRenderUtil.hlsl"

cbuffer ParamterCB : register(b0, space0)
{
    bool g_openAo;
    bool g_openShadow;
};

cbuffer ShadowCB : register(b1, space0)
{
    float4x4 g_ShadowTransform;
}

Texture2D<float> g_AoMap : register(t0, space0);
Texture2D<float> g_ShadowMap : register(t1, space0);
Texture2D<float4> g_SkyColor : register(t2, space0);

SamplerState g_samLinear0 : register(s0, space0);
SamplerComparisonState g_samShadow : register(s1, space0);

float3 toneMap(float3 color)
{
    // HDR tonemapping
    color = color / (color + float3(1.0f, 1.0f, 1.0f));
    // gamma correct
    return pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2)); 
}

struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TexCoord0;
};

float CalcShadowFactor(float4 shadowPosH)
{
    // Complete projection by doing division by w.
    shadowPosH.xyz /= shadowPosH.w;

    // Depth in NDC space.
    float depth = shadowPosH.z;

    uint width, height, numMips;
    g_ShadowMap.GetDimensions(0, width, height, numMips);

    // Texel size.
    float dx = 1.0f / (float) width;

    float percentLit = 0.0f;
    const float2 offsets[9] =
    {
        float2(-dx, -dx), float2(0.0f, -dx), float2(dx, -dx),
        float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
        float2(-dx, +dx), float2(0.0f, +dx), float2(dx, +dx)
    };

    [unroll]
    for (int i = 0; i < 9; ++i)
    {
        percentLit += g_ShadowMap.SampleCmpLevelZero(g_samShadow, shadowPosH.xy + offsets[i], depth).r;
    }
    
    return percentLit / 9.0f;
}

float4 main(VSOutput vsOutput) : SV_Target0
{   
    GBufferResult gb = prepareGBufferResult(vsOutput.texCoord);
    
    if (gb.diffuse.a < 0.3f)
        return g_SkyColor.Sample(g_samLinear0, vsOutput.texCoord).rgba;
    
    float ao = 1.0f;
    if (g_openAo)
    {
        float minAo = gb.modelId > 3 ? 0.35f : 0.15;
        ao = g_AoMap.Sample(g_samLinear0, vsOutput.texCoord);
        ao = pow(ao, 8);
        ao = lerp(minAo, 1.0f, ao);
    }

    ShadingResult shadingResult = deferredPbrShading(gb);
    float3 color = shadingResult.ambient * ao + shadingResult.diffuse + shadingResult.specular;
    
    if(g_openShadow)
    {
        float4 shadowPosH = mul(g_ShadowTransform, float4(gb.posW, 1.0));
        float shadowFactor = CalcShadowFactor(shadowPosH);
        color *= lerp(0.5f, 1.0f, clamp((1.0f - shadowFactor), 0.0f, 1.0f));
    }
    
    return float4(toneMap(color), shadingResult.alpha);
}

