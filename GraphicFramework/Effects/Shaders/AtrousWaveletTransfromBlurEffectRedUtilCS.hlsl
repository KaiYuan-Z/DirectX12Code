#include "../../Core/ShaderUtility/MathUtil.hlsl"

cbuffer cbBlurParam : register(b0)
{
    uint2   g_TextureDim;
    float   g_DepthWeightCutoff;
    float   g_DepthSigma;
    
    float   g_NormalSigma; 
    uint3   pad;
};

Texture2D<float>        g_LinearDepth       : register(t0);
Texture2D<float2>       g_Norm              : register(t1);
Texture2D<float2>       g_Ddxy              : register(t2);
Texture2D<float>        g_InputValue        : register(t3);

RWTexture2D<float>      g_OutputValue      : register(u0);

SamplerState g_samPointClamp : register(s0);
SamplerState g_samLinearClamp : register(s1);

float calcDepthThreshold(float depth, float2 ddxy, float2 pixelOffset)
{
    return dot(1, abs(pixelOffset * ddxy));
}

void AddFilterContribution(
    inout float weightedValueSum,
    inout float weightSum,
    in float value,
    in float depth,
    in float3 normal,
    in float2 ddxy,
    in uint row,
    in uint col,
    in uint radius,
    in uint2 DTid)
{
    const float normalSigma = g_NormalSigma;
    const float depthSigma = g_DepthSigma;
 
    int2 pixelOffset;
    float kernelWidth;
    float varianceScale = 1;

    pixelOffset = int2(row - radius, col - radius);
    int2 id = int2(DTid) + pixelOffset;

    if (isWithinBounds(id, g_TextureDim))
    {
        float iDepth = g_LinearDepth[id];
        float3 iNormal = decodeNormal(g_Norm[id].xy);
        
        float iValue = g_InputValue[id];

        // Calculate a weight for the neighbor's contribtuion.
        // Ref:[SVGF]
        float w;
        {
            // Normal based weight.
            float w_n = pow(max(0, dot(normal, iNormal)), normalSigma);

            // Depth based weight.
            float w_d;
            {
                float depthFloatPrecision = floatPrecision(max(depth, iDepth), 23);
                float depthThreshold = calcDepthThreshold(depth, ddxy, pixelOffset);
                float depthTolerance = depthSigma * depthThreshold + depthFloatPrecision;
                float delta = abs(depth - iDepth);
                delta = max(0, delta - depthFloatPrecision); // Avoid distinguising initial values up to the float precision. Gets rid of banding due to low depth precision format.
                w_d = exp(-delta / depthTolerance);

                // Scale down contributions for samples beyond tolerance, but completely disable contribution for samples too far away.
                w_d *= w_d >= g_DepthWeightCutoff;
            }

            // Filter kernel weight.
            float w_h = calcGaussWeights(radius, pixelOffset.x) * calcGaussWeights(radius, pixelOffset.y);

            // Final weight.
            w = w_h * w_n * w_d;
        }

        weightedValueSum += w * iValue;
        weightSum += w;
    }
}


[numthreads(8, 8, 1)]
void main(uint2 Gid : SV_GroupID, uint2 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex, uint2 DTid : SV_DispatchThreadID)
{
    uint Radius = 1;

    #ifdef BLUR_KERNEL_3X3
        Radius = 1;
    #endif

    #ifdef BLUR_KERNEL_5X5
        Radius = 2;
    #endif

    #ifdef BLUR_KERNEL_7X7
        Radius = 3;
    #endif

    #ifdef BLUR_KERNEL_9X9
        Radius = 4;
    #endif

    #ifdef BLUR_KERNEL_11X11
        Radius = 5;
    #endif

    #ifdef BLUR_KERNEL_13X13
        Radius = 6;
    #endif

    #ifdef BLUR_KERNEL_15X15
        Radius = 7;
    #endif
       
    float depth = g_LinearDepth[DTid];
    float value = g_InputValue[DTid];
    float2 ddxy = g_Ddxy[DTid];
    float3 normal = decodeNormal(g_Norm[DTid].xy);
  
    float w = calcGaussWeights(Radius, 0);
    float weightSum = w * w;
    float weightedValueSum = weightSum * value;
    
    uint kernelWidth = Radius * 2 + 1;
    
    {
        // Add contributions from the neighborhood.
        [unroll]
        for (uint r = 0; r < kernelWidth; r++)
        {  
            [unroll]
            for (uint c = 0; c < kernelWidth; c++)
            {      
                if (r == Radius && c == Radius)
                {
                    continue;
                }
                    
                AddFilterContribution(
                    weightedValueSum,
                    weightSum,
                    value,
                    depth,
                    normal,
                    ddxy,
                    r,
                    c,
                    Radius,
                    DTid);
            }
        }
    }

    const float c_smallValue = 1e-6f;
    
    float filteredValue = value;
    if (weightSum > c_smallValue)
    {
        filteredValue = weightedValueSum / weightSum;
    }
    else
    {
        filteredValue = 0;
    }

    g_OutputValue[DTid] = filteredValue;
}
