#include "../../Core/ShaderUtility/MathUtil.hlsl"

cbuffer AccumFrameCB : register(b0, space0)
{
    float g_stdDevGamma;
    float g_clampMinStdDevTolerance;
    float g_clampDifferenceToTsppScale;
    float g_minSmoothingFactor;
    uint  g_minTsppToUseTemporalVariance;
    uint  g_minNsppForClamp;
    uint  g_dynamicMarkMaxLife;
    uint  g_winWidth;
    uint  g_winHeight;
    uint  g_maxAccumCount;
    float2 pad;
};

Texture2D<float>    g_preAccumFrame         : register(t0, space0);
Texture2D<float>    g_preAccumSquaredMean   : register(t1, space0);
Texture2D<float>    g_curFrame              : register(t2, space0);
Texture2D<float2>   g_motionVec             : register(t3, space0);
Texture2D<float2>   g_localMeanVariance     : register(t4, space0);
Texture2D<int4>     g_curAccumMap           : register(t5, space0);

RWTexture2D<float> g_outAccumFrame          : register(u0, space0);
RWTexture2D<float> g_outAccumSquaredMean    : register(u1, space0);
RWTexture2D<float> g_outAccumVariance       : register(u2, space0);
RWTexture2D<int4>  g_outAccumMap            : register(u3, space0); // AccumCount(int), NumSampleCount(int), DynamicMeshHint(int), DynamicMark(int)

SamplerState g_samplerPointClamp : register(s0);
SamplerState g_samplerLinearClamp : register(s1);


[numthreads(8, 8, 1)]
void main(uint2 Gid : SV_GroupID, uint2 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex, uint2 DTid : SV_DispatchThreadID)
{
    uint2 curIndex = DTid;
    float2 curTex = index2Texcoord(uint2(g_winWidth, g_winHeight), curIndex);
    float2 preTex = curTex + g_motionVec[curIndex].xy;
    uint2 preIndex = texcoord2Index(uint2(g_winWidth, g_winHeight), preTex);

    float curValue = g_curFrame[curIndex];
    float preValue = g_preAccumFrame.SampleLevel(g_samplerLinearClamp, preTex, 0.0f);
    float preSquaredMeanValue = g_preAccumSquaredMean.SampleLevel(g_samplerLinearClamp, preTex, 0.0f);
    float2 localMeanVariance = g_localMeanVariance[curIndex];
    int4 accumMap = g_curAccumMap[curIndex].xyzw;
    
    float curSquaredMeanValue = curValue * curValue;
    float localMean = localMeanVariance.x;
    float localVariance = localMeanVariance.y;
    uint Tspp = accumMap.x;
    uint Nspp = accumMap.y;
    int dynamicInstanceHint = accumMap.z;
  
    float newAccumValue = 0.0f;
    float newAccumSquaredMeanValue = 0.0f;  

    Tspp = clamp(Tspp + Nspp, 0, g_maxAccumCount);
        
    if (Nspp >= g_minNsppForClamp)
    {
        float nonClampedPreValue = preValue;
        
        float localStdDev = max(g_stdDevGamma * sqrt(localVariance), g_clampMinStdDevTolerance);
        float clampedPreValue = clamp(nonClampedPreValue, localMean - localStdDev, localMean + localStdDev);

        float clampFactor = max(clampedPreValue, nonClampedPreValue);
        clampFactor = clampFactor > 0 ? abs(clampedPreValue - nonClampedPreValue) / clampFactor : 0.0f;
        float accumCountScale = saturate(g_clampDifferenceToTsppScale * clampFactor);
        Tspp = lerp(Tspp, 0, accumCountScale);
        
        float invTspp = Tspp > 0 ? 1.0f / Tspp : 1.0f;
    
        const bool c_forceUseMinSmoothingFactor = false;
        float smoothingFactor = c_forceUseMinSmoothingFactor ? g_minSmoothingFactor : max(invTspp, g_minSmoothingFactor);
        float MaxSmoothingFactor = 1;
        smoothingFactor = min(smoothingFactor, MaxSmoothingFactor);
            
        newAccumValue = lerp(clampedPreValue, curValue, smoothingFactor);
        newAccumSquaredMeanValue = lerp(preSquaredMeanValue, curSquaredMeanValue, smoothingFactor);
    }
    else
    {
        newAccumValue = (float(Tspp - Nspp) * preValue + float(Nspp) * curValue) / float(Tspp);
        newAccumSquaredMeanValue = ((Tspp - Nspp) * preSquaredMeanValue + Nspp * curSquaredMeanValue) / float(Tspp);
    }
    
    // Variance.
    float temporalVariance = newAccumSquaredMeanValue - newAccumValue * newAccumValue;
    temporalVariance = max(0, temporalVariance); // Ensure variance doesn't go negative due to imprecision.
    float variance = (Tspp >= g_minTsppToUseTemporalVariance) ? temporalVariance : localVariance;
    variance = max(0.1, variance);
    
    accumMap.x = Tspp;
    
    g_outAccumSquaredMean[curIndex] = newAccumSquaredMeanValue;
    g_outAccumMap[curIndex] = accumMap;
    g_outAccumFrame[curIndex] = newAccumValue;
    g_outAccumVariance[curIndex] = variance;
}