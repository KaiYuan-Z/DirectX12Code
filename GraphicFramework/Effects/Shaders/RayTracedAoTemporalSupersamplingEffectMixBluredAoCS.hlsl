#include "../../Core/ShaderUtility/MathUtil.hlsl"

#define KernelSize 4

cbuffer MixParamterCB : register(b0)
{
    uint    g_useBlurMaxTspp;
    float   g_blurDecayStrength;
    float2  pad;
};

Texture2D<float>    g_rawAccumFrame         : register(t0);
Texture2D<float>    g_bluredAccumFrame      : register(t1);
Texture2D<float>    g_aoDiffMap             : register(t2);
Texture2D<int4>     g_curAccumMap           : register(t3);

RWTexture2D<float>  g_outMixedAccumFrame    : register(u0);


[numthreads(8, 8, 1)]
void main(uint2 Gid : SV_GroupID, uint2 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex, uint2 DTid : SV_DispatchThreadID)
{
    uint2 index = DTid;
    uint2 quadIndex = index / KernelSize;
    float rawValue = g_rawAccumFrame[index];
    float bluredValue = g_bluredAccumFrame[index];
    float aoDiff = g_aoDiffMap[quadIndex];
    int4 accum = g_curAccumMap[index]; 
    float tspp = accum.x;
    float nspp = accum.y;
    float TsppRatio = min(tspp, g_useBlurMaxTspp) / float(g_useBlurMaxTspp);
    float blurStrength = clamp(pow(1 - TsppRatio, g_blurDecayStrength), 0, 1);
    blurStrength = aoDiff > 0.0f ? 1 : blurStrength;
    g_outMixedAccumFrame[index] = lerp(rawValue, bluredValue, blurStrength);
}
