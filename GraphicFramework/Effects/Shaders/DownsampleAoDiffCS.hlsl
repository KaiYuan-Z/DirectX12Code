#include "../../Core/ShaderUtility/MathUtil.hlsl"

#define KernelSize 4

cbuffer DownsampleAoDiffCB : register(b0)
{
    int2   g_aoMapSize;
    int2   pad;
};

Texture2D<float>    g_aoDiffTestMap        : register(t0);

RWTexture2D<float>  g_outAoDiffMap         : register(u0);


[numthreads(8, 8, 1)]
void main(uint2 Gid : SV_GroupID, uint2 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex, uint2 DTid : SV_DispatchThreadID)
{
    uint2 index = DTid;
    uint2 quadFirstIndex = KernelSize * index;
    
    float diffSum = 0.0f;
    
    [unroll]
    for (int i = 0; i < KernelSize; i++)
    {
        [unroll]
        for (int j = 0; j < KernelSize; j++)
        {        
            uint2 sampleIndex = quadFirstIndex + uint2(i, j);
            sampleIndex = clamp(sampleIndex, 0, g_aoMapSize - 1);
            diffSum += g_aoDiffTestMap[sampleIndex];
        }
    }

    diffSum /= float(KernelSize * KernelSize);
    
    g_outAoDiffMap[index] = diffSum;
}
