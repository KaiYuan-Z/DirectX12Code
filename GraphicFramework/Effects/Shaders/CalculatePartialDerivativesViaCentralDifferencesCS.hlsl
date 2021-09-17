#include "../../Core/ShaderUtility/MathUtil.hlsl"


Texture2D<float> g_inValue : register(t0);
RWTexture2D<float2> g_outValue : register(u0);


[numthreads(8, 8, 1)]
void main(uint2 Gid : SV_GroupID, uint2 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex, uint2 DTid : SV_DispatchThreadID)
{
    uint2 textureDim; 
    g_inValue.GetDimensions(textureDim.x, textureDim.y);
    
    //                x
    //        ----------------->
    //    |    x     [top]     x
    // y  |  [left]   DTiD   [right]
    //    v    x    [bottom]   x
    //
    uint2 top = clamp(DTid.xy + uint2(0, -1), 0, textureDim - 1);
    uint2 bottom = clamp(DTid.xy + uint2(0, 1), 0, textureDim - 1);
    uint2 left = clamp(DTid.xy + uint2(-1, 0), 0, textureDim - 1);
    uint2 right = clamp(DTid.xy + uint2(1, 0), 0, textureDim - 1);

    float centerValue = g_inValue[DTid.xy];
    float2 backwardDifferences = centerValue - float2(g_inValue[left], g_inValue[top]);
    float2 forwardDifferences = float2(g_inValue[right], g_inValue[bottom]) - centerValue;

    // Calculates partial derivatives as the min of absolute backward and forward differences. 

    // Find the absolute minimum of the backward and foward differences in each axis
    // while preserving the sign of the difference.
    float2 ddx = float2(backwardDifferences.x, forwardDifferences.x);
    float2 ddy = float2(backwardDifferences.y, forwardDifferences.y);

    uint2 minIndex = {
        getIndexOfValueClosestToTheReference(0, ddx),
        getIndexOfValueClosestToTheReference(0, ddy)
    };
    float2 ddxy = float2(ddx[minIndex.x], ddy[minIndex.y]);

    // Clamp ddxy to a reasonable value to avoid ddxy going over surface boundaries
    // on thin geometry and getting background/foreground blended together on blur.
    float maxDdxy = 1;
    float2 _sign = sign(ddxy);
    ddxy = _sign * min(abs(ddxy), maxDdxy);

    g_outValue[DTid] = ddxy;
}