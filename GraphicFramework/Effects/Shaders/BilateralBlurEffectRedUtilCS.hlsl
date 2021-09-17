#include "../../Core/ShaderUtility/MathUtil.hlsl"

cbuffer cbBlurParam : register(b0)
{
    uint2   g_ImageSize;
    float2  g_InvImageSize;
    
    float   g_DepthSigma;
    float   g_DepthThreshold;  
    float   g_NormalSigma;
    float   g_NormalThreshold;
};

cbuffer cbRootConstants : register(b1)
{
	bool  g_HorizontalBlur;
};

Texture2D<float>    g_LinearDepthMap   : register(t0);
Texture2D<float2>   g_NormMap          : register(t1);
Texture2D<float2>   g_DdxyMap          : register(t2);
Texture2D<float>    g_InputMap         : register(t3);

RWTexture2D<float> g_OutputMap         : register(u0);

SamplerState g_samPointClamp : register(s0);
SamplerState g_samLinearClamp : register(s1);

float calcDepthThreshold(float2 ddxy, float2 pixelOffset)
{
    return dot(1, abs(pixelOffset * ddxy));
}

float calcWeight(
    int radius,
    int2 pixelOffset,
    float2 uv,
    float centerD,
    float2 centerDdxy,
    float3 centerN,
    float neighborD,
    float3 neighborN)
{
    float w_d;
    {
        float fwidthZ = dot(1, abs(centerDdxy));
        float weight = (fwidthZ + 1e-4) / (abs(centerD - neighborD) + 1e-4);        
        w_d = weight > g_DepthThreshold ? 1.0f : 0.0f;
    }
    
    float w_n = pow(max(0, dot(centerN, neighborN)), g_NormalSigma) > g_NormalThreshold ? 1.0f : 0.0f;

    int r = dot(1, pixelOffset);
    float w_g = calcGaussWeights(radius, r);
    
    return w_d * w_n * w_g;
}

void processSampleInnerHalf(
    int radius, 
    int2 pixelOffset, 
    float2 uv, 
    float centerD, 
    float2 centerDdxy,
    float3 centerN,
    inout float totalGaussW, 
    inout float totalColor)
{
    float neighborD = g_LinearDepthMap.SampleLevel(g_samPointClamp, uv, 0.0f);
    float3 neighborN = decodeNormal(g_NormMap.SampleLevel(g_samPointClamp, uv, 0.0f));  
    float weight = calcWeight(radius, pixelOffset, uv, centerD, centerDdxy, centerN, neighborD, neighborN);
	totalGaussW += weight;
    totalColor += weight * g_InputMap.SampleLevel(g_samPointClamp, uv, 0.0f);
}

void processSampleOuterHalf(
    int radius,
    int2 pixelOffset,
    float2 uv,
    float centerD,
    float2 centerDdxy,
    float3 centerN,
    inout float totalGaussW,
    inout float totalColor)
{  
    float neighborD = g_LinearDepthMap.SampleLevel(g_samLinearClamp, uv, 0.0f);
    float3 neighborN = decodeNormal(g_NormMap.SampleLevel(g_samLinearClamp, uv, 0.0f));  
    float weight = calcWeight(radius, pixelOffset, uv, centerD, centerDdxy, centerN, neighborD, neighborN);
    totalGaussW += weight;
    totalColor += weight * g_InputMap.SampleLevel(g_samLinearClamp, uv, 0.0f);
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
    
    float2 texOffset = g_HorizontalBlur ? float2(g_InvImageSize.x, 0.0f) : float2(0.0f, g_InvImageSize.y);
    int2 pixelOffset = g_HorizontalBlur ? int2(1, 0) : int2(0, 1);
    float2 texC = (DTid + 0.5f) * g_InvImageSize;
    
    float color = g_InputMap.SampleLevel(g_samPointClamp, texC, 0.0f);
    float centerDepth = g_LinearDepthMap.SampleLevel(g_samPointClamp, texC, 0.0f);
    float2 centerDdxy = g_DdxyMap.SampleLevel(g_samPointClamp, texC, 0.0f);
    float3 centerNorm = decodeNormal(g_NormMap.SampleLevel(g_samPointClamp, texC, 0.0f));
    float centerWeight = calcGaussWeights(Radius, 0, centerDepth, centerDepth);
	color *= centerWeight;
	float totalGaussWeight = centerWeight;

	int i = -int(Radius);

    [unroll]
	for (; i < -int(Radius / 2); i += 2)
	{
		float2 tex = texC + (i + 0.5) * texOffset;
        float2 pOffset = i * pixelOffset;
        processSampleOuterHalf(Radius, pOffset, tex, centerDepth, centerDdxy, centerNorm, totalGaussWeight, color);
    }

    [unroll]
    for (; i <= int(Radius / 2); i += 1)
	{
		// We already added in the center weight.
		if (i == 0)
			continue;

		float2 tex = texC + i * texOffset;
        float2 pOffset = i * pixelOffset;
        processSampleInnerHalf(Radius, pOffset, tex, centerDepth, centerDdxy, centerNorm, totalGaussWeight, color);
    }

    [unroll]
    for (; i <= int(Radius); i += 2)
	{
		float2 tex = texC + (i + 0.5) * texOffset;
        float2 pOffset = i * pixelOffset;
        processSampleOuterHalf(Radius, pOffset, tex, centerDepth, centerDdxy, centerNorm, totalGaussWeight, color);
    }

	// Compensate for discarded samples by making total weights sum to 1.
    g_OutputMap[DTid] =  color / totalGaussWeight;
}
