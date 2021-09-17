#include "../../Core/ShaderUtility/MathUtil.hlsl"

cbuffer cbBlurParam : register(b0)
{
    int2 g_ImageSize;
};

Texture2D<float2> g_InputMap : register(t0);

RWTexture2D<float2> g_OutputMap : register(u0);


[numthreads(8, 8, 1)]
void main(uint2 Gid : SV_GroupID, uint2 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex, uint2 DTid : SV_DispatchThreadID)
{
    int Size = 2;

#ifdef DOWN_SAMPLE_SIZE_2X2
        Size = 2;
#endif

#ifdef DOWN_SAMPLE_SIZE_3X3
        Size = 3;
#endif

#ifdef DOWN_SAMPLE_SIZE_4X4
        Size = 4;
#endif
    
    int2 quadIndex = DTid;
    int2 quadLeftTopImageIndex = quadIndex * Size;
    
    float2 sumValue = float2(0.0f, 0.0f);
    int count = 0;
    
    [unroll]
    for (int i = 0; i < Size; i++)
    {
        [unroll]
        for (int j = 0; j < Size; j++)
        {
            int2 imageIndex = quadLeftTopImageIndex + int2(i, j);
            
            if (isWithinBounds(imageIndex, g_ImageSize))
            {
                sumValue += g_InputMap[imageIndex];
                count++;
            }
        }
    }

    g_OutputMap[quadIndex] = count > 0 ? sumValue / float(count) : 0.0f;
}
