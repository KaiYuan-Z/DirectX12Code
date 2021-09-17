#include "../../Core/ShaderUtility/MathUtil.hlsl"

struct VSOutput
{
	float4 position : SV_Position;
	float2 tex0 : TEXCOORD0;
};

cbuffer cb_AccumMap : register(b0)
{
    float4x4 g_view;
    bool     g_viewUpdateFlag;
    float3   pad;
};

Texture2D<int4>   g_preIDMap        : register(t0);
Texture2D<int4>   g_curIDMap        : register(t1);
Texture2D<float2> g_motionVecMap    : register(t2);
Texture2D<float4> g_preAccumMap     : register(t3); // Do not use uint, must use float and use point sampler or higher sampler.


SamplerState g_samplerPointBorder  : register(s0);
SamplerState g_samplerLinearBorder : register(s1);

bool isIdEqual(int4 id0, int4 id1)
{
    return (id0.r == id1.r) && (id0.g == id1.g) && (id0.b == id1.b);
}

//Texture2D<float4> AccumMap :AccumCount(float), NumSampleCount(float), UserDefine(float2)

float4 main(VSOutput vsOutput) : SV_Target0
{
    if (!g_viewUpdateFlag)
    {
        float4 accumMap = g_preAccumMap.Sample(g_samplerPointBorder, vsOutput.tex0).xyzw;
        return accumMap;
    }

    uint width, height, numMips;
    g_curIDMap.GetDimensions(0, width, height, numMips);
    
    int4 curId = g_curIDMap[vsOutput.position.xy];
    float2 preTex = vsOutput.tex0 + g_motionVecMap.Sample(g_samplerPointBorder, vsOutput.tex0).xy;
    int4 preId = g_preIDMap[preTex * float2(width, height)];

    if (verifyTexcoord(preTex) && isIdEqual(curId, preId))
    {
        float4 accumMap = g_preAccumMap.Sample(g_samplerPointBorder, preTex).xyzw;
        return accumMap;
    }

    return float4(0.0f, 0.0f, 0.0f, 0.0f);
}
