#include "../../Core/ShaderUtility/MathUtil.hlsl"

struct VSOutput
{
	float4 position : SV_Position;
	float2 tex0 : TEXCOORD0;
};

Texture2D<float>  g_preFrame		: register(t0);
Texture2D<float>  g_curFrame		: register(t1);
Texture2D<float2> g_motionVecMap    : register(t2);
Texture2D<float4> g_curAccumMap     : register(t3);

SamplerState g_samplerPointBorder   : register(s0);
SamplerState g_samplerLinearBorder  : register(s1);


float main(VSOutput vsOutput) : SV_Target0
{
    float2 preTex = vsOutput.tex0 + g_motionVecMap.Sample(g_samplerPointBorder, vsOutput.tex0).xy;
    float4 accumMap = g_curAccumMap.Sample(g_samplerPointBorder, vsOutput.tex0).xyzw;
    float accumCount = accumMap.x;
    float newSampleCount = accumMap.y;
	float curColor = g_curFrame.Sample(g_samplerLinearBorder, vsOutput.tex0).r;
    float preColor = g_preFrame.Sample(g_samplerLinearBorder, preTex).r;
    return ((accumCount - newSampleCount) * preColor + newSampleCount * curColor) / accumCount;
}
