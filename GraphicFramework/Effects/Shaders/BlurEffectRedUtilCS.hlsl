#include "../../Core/ShaderUtility/MathUtil.hlsl"

cbuffer cbBlurParam : register(b0)
{
    float2 g_InvImageSize;
    float2 pad;
};

cbuffer cbRootConstants : register(b1)
{
	bool  g_HorizontalBlur;
};

Texture2D<float> g_LinearDepthMap   : register(t0);
Texture2D<float> g_InputMap         : register(t1);

RWTexture2D<float> g_OutputMap      : register(u0);

SamplerState g_samPointClamp : register(s0);
SamplerState g_samLinearClamp : register(s1);

void processSampleInnerHalf(int radius, int r, float2 uv, float centerD, inout float totalGaussW, inout float totalColor)
{
    float neighborD = g_LinearDepthMap.SampleLevel(g_samPointClamp, uv, 0.0f).r;

    float weight = calcGaussWeights(radius, r, centerD, neighborD);
	totalGaussW += weight;

    totalColor += weight * g_InputMap.SampleLevel(g_samPointClamp, uv, 0.0f);
}

void processSampleOuterHalf(int radius, int r, float2 uv, float centerD, inout float totalGaussW, inout float totalColor)
{
    float neighborD = g_LinearDepthMap.SampleLevel(g_samLinearClamp, uv, 0.0f).r;

    float weight = calcGaussWeights(radius, r, centerD, neighborD);
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
    float2 texC = (DTid + 0.5f) * g_InvImageSize;
    
    float color = g_InputMap.SampleLevel(g_samPointClamp, texC, 0.0f);
    float centerDepth = g_LinearDepthMap.SampleLevel(g_samPointClamp, texC, 0.0f).r;
    float centerWeight = calcGaussWeights(Radius, 0, centerDepth, centerDepth);
	color *= centerWeight;
	float totalGaussWeight = centerWeight;

	int i = -int(Radius);

    [unroll]
	for (; i < -int(Radius / 2); i += 2)
	{
		float2 tex = texC + (i + 0.5) * texOffset;
		processSampleOuterHalf(Radius, i, tex, centerDepth, totalGaussWeight, color);
	}

    [unroll]
    for (; i <= int(Radius / 2); i += 1)
	{
		// We already added in the center weight.
		if (i == 0)
			continue;

		float2 tex = texC + i * texOffset;
		processSampleInnerHalf(Radius, i, tex, centerDepth, totalGaussWeight, color);
	}

    [unroll]
    for (; i <= int(Radius); i += 2)
	{
		float2 tex = texC + (i + 0.5) * texOffset;
		processSampleOuterHalf(Radius, i, tex, centerDepth, totalGaussWeight, color);
	}

	// Compensate for discarded samples by making total weights sum to 1.
    g_OutputMap[DTid] =  color / totalGaussWeight;
}
