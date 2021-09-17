#include "../../Core/ShaderUtility/MathUtil.hlsl"

cbuffer MixParamterCB : register(b0)
{
    uint    g_useBlurMaxTspp;
    float   g_blurDecayStrength;
    float2  pad;
};

Texture2D<float>    g_rawAccumFrame         : register(t0);
Texture2D<float>    g_bluredAccumFrame      : register(t1);
Texture2D<int4>     g_curAccumMap           : register(t2);

RWTexture2D<float>  g_outMixedAccumFrame    : register(u0);


[numthreads(8, 8, 1)]
void main(uint2 Gid : SV_GroupID, uint2 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex, uint2 DTid : SV_DispatchThreadID)
{
    uint2 index = DTid;
    float rawValue = g_rawAccumFrame[index];
    float bluredValue = g_bluredAccumFrame[index];
    float tspp = g_curAccumMap[index].x; 
    float TsppRatio = min(tspp, g_useBlurMaxTspp) / float(g_useBlurMaxTspp);
    float blurStrength = pow(1 - TsppRatio, g_blurDecayStrength);
    g_outMixedAccumFrame[index] = lerp(rawValue, bluredValue, blurStrength);
}
