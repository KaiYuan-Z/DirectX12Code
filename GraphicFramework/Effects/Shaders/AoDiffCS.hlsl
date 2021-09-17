#include "../../Core/ShaderUtility/MathUtil.hlsl"


cbuffer DownsampleAoDiffCB : register(b0)
{
    int2   g_aoMapSize;
    int    g_kernelSize;
    int    pad;
};

Texture2D<float>    g_aoDiffTestMap        : register(t0);

RWTexture2D<float>  g_outAoDiffMap         : register(u0);


[numthreads(8, 8, 1)]
void main(uint2 Gid : SV_GroupID, uint2 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex, uint2 DTid : SV_DispatchThreadID)
{
    uint2 index = DTid;
    
    float diffSum = 0.0f;
    int count = 0;
    
    const int Radius = 10;
    
    [unroll]
    for (int i = -Radius; i <= Radius; i++)
    {[unroll]
        for (int j = -Radius; j <= Radius; j++)
        {        
            uint2 sampleIndex = uint2(i, j) + index;
            if (isWithinBounds(sampleIndex, g_aoMapSize))
            {
                diffSum += g_aoDiffTestMap[sampleIndex];
                count++;
            }
        }
    }

    diffSum /= float(count);
    
    g_outAoDiffMap[index] = diffSum;
}
