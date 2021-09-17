#include "../../../GraphicFramework/Core/ShaderUtility/SceneForwardRenderUtil.hlsl"

#define HIGH_LIGHT_COLOR float3(0.5f, 0.5f, 0.0f)

struct VSOutput
{
    float4 position : SV_Position;
    float4 ShadowPosH : ShadowPos;
    float3 worldPos : WorldPos;
    float2 texCoord : TexCoord0;
    float3 normal : Normal;
    float3 tangent : Tangent;
    float3 bitangent : Bitangent;
    nointerpolation uint sceneModelIndex : SceneModelIndex;
    nointerpolation uint instanceIndex : ModelInstanceIndex;
    nointerpolation uint instanceTag : InstanceTag;
};

cbuffer cb_HighLightIndex : register(b1, space0)
{
    uint g_HighLightModelInstIndex;
    uint g_HighLightGrassInstIndex;
};

Texture2D<float> g_ShadowMap : register(t0, space0);

SamplerComparisonState g_samShadow : register(s0);

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
    ShadingResult shadingResult = pbrShading(vsOutput.worldPos, vsOutput.normal, vsOutput.tangent, vsOutput.bitangent, vsOutput.texCoord);
    float3 color = shadingResult.ambient + shadingResult.diffuse + shadingResult.specular;
    // HDR tonemapping
    color = color / (color + float3(1.0f, 1.0f, 1.0f));
    // gamma correct
    color = pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));
    
    float shadowFactor = CalcShadowFactor(vsOutput.ShadowPosH);
    color *= lerp(0.7f, 1.0f, clamp((1.0f - shadowFactor), 0.0f, 1.0f));
    
    if (g_HighLightModelInstIndex == vsOutput.instanceIndex && !(GRASS_INST_TAG & vsOutput.instanceTag))
    {
        color += HIGH_LIGHT_COLOR;
    }
    else if (g_HighLightGrassInstIndex == vsOutput.instanceIndex && (GRASS_INST_TAG & vsOutput.instanceTag))
    {
        color += HIGH_LIGHT_COLOR;
    }
    
    return float4(color, shadingResult.alpha);
}
