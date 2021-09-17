#include "../../Core/ShaderUtility/MathUtil.hlsl"

#define MAX_DIFFERENCE 0.03

struct VSOutput
{
	float4 position : SV_Position;
	float2 tex0 : TEXCOORD0;
};

cbuffer cb_AccumMap : register(b0)
{
    bool     g_viewUpdateFlag;
    float3   pad;
};

Texture2D<float>  g_preLinearDepthMap		: register(t0);
Texture2D<float>  g_curLinearDepthMap       : register(t1);
Texture2D<float2> g_motionVecMap            : register(t2);
Texture2D<float4> g_preAccumMap             : register(t3); // Do not use uint, must use float and use point sampler or higher sampler.

SamplerState g_samplerPointBorder  : register(s0);
SamplerState g_samplerLinearBorder : register(s1);


//Texture2D<float4> AccumMap :AccumCount(float), NumSampleCount(float), HaltonIndex(float), UserDefine(float)

float4 main(VSOutput vsOutput) : SV_Target0
{
    if (!g_viewUpdateFlag)
    {
        float4 accumMap = g_preAccumMap.Sample(g_samplerPointBorder, vsOutput.tex0).xyzw;
        return accumMap;
    }
    
    float curZ = g_curLinearDepthMap.Sample(g_samplerLinearBorder, vsOutput.tex0);
    float2 preTex = vsOutput.tex0 + g_motionVecMap.Sample(g_samplerPointBorder, vsOutput.tex0).xy;
    float preZ = g_preLinearDepthMap.Sample(g_samplerLinearBorder, preTex);

    if (verifyTexcoord(preTex) && abs(1.0f - curZ / preZ) < MAX_DIFFERENCE)
	{
        float4 accumMap = g_preAccumMap.Sample(g_samplerPointBorder, preTex).xyzw;
        return accumMap;
    }

    return float4(0.0f, 0.0f, 0.0f, 0.0f);
}