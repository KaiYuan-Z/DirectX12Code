#include "../../Core/ShaderUtility/MathUtil.hlsl"

Texture2D<float2> g_InputMap         : register(t0);

RWTexture2D<float4> g_OutputMap       : register(u0);


[numthreads(8, 8, 1)]
void main(uint2 Gid : SV_GroupID, uint2 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex, uint2 DTid : SV_DispatchThreadID)
{
    g_OutputMap[DTid] = float4(decodeNormal(g_InputMap[DTid]), 1.0f);
}
