#include "../../Core/ShaderUtility/RenderCommonUtil.hlsl"
#include "../../Core/ShaderUtility/CrossBilateralWeights.hlsl"

#define KernelSize 4

cbuffer cb_AccumMap : register(b0)
{
    uint        g_winWidth;
    uint        g_winHeight;
    float       g_invWinWidth;
    float       g_invWinHeight;
};



Texture2D<float2>           g_preLinearDepth                   : register(t0, space0);
Texture2D<float2>           g_preNorm                          : register(t1, space0);
Texture2D<float2>           g_curLinearDepth                   : register(t2, space0);
Texture2D<float2>           g_curNorm                          : register(t3, space0);
Texture2D<float2>           g_curDdxy                          : register(t4, space0);
Texture2D<float2>           g_curMotionVec                     : register(t5, space0);
Texture2D<int4>             g_curIds                           : register(t6, space0);
Texture2D<int>              g_curDynmiacAreaMark               : register(t7, space0);
Texture2D<float>            g_preAccumFrame                    : register(t8, space0);
Texture2D<int4>             g_preAccumMap                      : register(t9, space0);

RWTexture2D<float>          g_outAccumFrame                    : register(u0, space0);
RWTexture2D<int4>           g_outAccumMap                      : register(u1, space0); // AccumCount(int), NumSampleCount(int), DynamicInstanceHint(int), DynamicMark(int)

SamplerState                g_samplerPointClamp                : register(s0, space0);
SamplerState                g_samplerLinearClamp               : register(s1, space0);



float4 BilateralResampleWeights(in uint2 textureDim, in float TargetDepth, in float3 TargetNormal, in float4 SampleDepths, in float3 SampleNormals[4], in float2 Ddxy, in float2 TargetOffsetToTopLeft, in int2 sampleIndices[4])
{    
    bool4 bIsWithinBounds = bool4(
        isWithinBounds(sampleIndices[0], textureDim),
        isWithinBounds(sampleIndices[1], textureDim),
        isWithinBounds(sampleIndices[2], textureDim),
        isWithinBounds(sampleIndices[3], textureDim));
 
    CrossBilateral::BilinearDepthNormal::Parameters params;
    params.Depth.WeightCutoff = 0.05;
    params.Normal.Sigma = 1;
    params.Normal.SigmaExponent = 128;

    float4 bilinearDepthNormalWeights = CrossBilateral::BilinearDepthNormal::GetWeights(
        TargetDepth,
        TargetNormal,
        TargetOffsetToTopLeft,
        SampleDepths,
        SampleNormals,
        Ddxy,
        params);

    float4 weights = bIsWithinBounds * bilinearDepthNormalWeights;

    return weights;
}


[numthreads(8, 8, 1)]
void main(uint2 Gid : SV_GroupID, uint2 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex, uint2 DTid : SV_DispatchThreadID)
{
    uint2 curIndex = DTid;
    uint2 textureDim = uint2(g_winWidth, g_winHeight);
    float2 invTextureDim = float2(g_invWinWidth, g_invWinHeight);
    float2 curTex = index2Texcoord(textureDim, curIndex);
    float2 preTex = curTex + g_curMotionVec[curIndex].xy;
       
    int4 ids = g_curIds[curIndex];
    int dynamicInstanceHint = (ids.w == int(DYNAMIC_INSTANCE_TAG));
    int dynamicAreaMark = g_curDynmiacAreaMark[curIndex];
    
    float accumValue = 0.0f;
    int4 accumMap = int4(0, 0, dynamicInstanceHint, dynamicAreaMark);
    float2 clampCache = { 0.0f, 0.0f };
    
    if (verifyTexcoord(preTex))
    {
        int2 preTopLeftTexIndex = floor(preTex * textureDim - 0.5f);
        float2 preTopLeftTex = (preTopLeftTexIndex + 0.5f) * invTextureDim;

        float2 toPreTopLeftPixelOffset = preTex * textureDim - (preTopLeftTexIndex + 0.5f);

        const int2 srcIndexOffsets[4] = { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } };

        int2 preTexIndices[4] =
        {
            preTopLeftTexIndex + srcIndexOffsets[0],
            preTopLeftTexIndex + srcIndexOffsets[1],
            preTopLeftTexIndex + srcIndexOffsets[2],
            preTopLeftTexIndex + srcIndexOffsets[3]
        };

        float curDepth = g_curLinearDepth[curIndex].y; // Reprojected Linear Depth
        float3 curNorm = decodeNormal(g_curNorm[curIndex]);
        float2 ddxy = g_curDdxy[curIndex];
    
        float4 preDepths = g_preLinearDepth.GatherRed(g_samplerPointClamp, preTopLeftTex).wzxy;
        float4 normXs = g_preNorm.GatherRed(g_samplerPointClamp, preTopLeftTex).wzxy;
        float4 normYs = g_preNorm.GatherGreen(g_samplerPointClamp, preTopLeftTex).wzxy;
        float3 preNorms[4] =
        {
            decodeNormal(float2(normXs.x, normYs.x)),
            decodeNormal(float2(normXs.y, normYs.y)),
            decodeNormal(float2(normXs.z, normYs.z)),
            decodeNormal(float2(normXs.w, normYs.w))
        };
        
        float4 preAccumValues = g_preAccumFrame.GatherRed(g_samplerPointClamp, preTopLeftTex).wzxy;

        float4 weights = BilateralResampleWeights(textureDim, curDepth, curNorm, preDepths, preNorms, ddxy, toPreTopLeftPixelOffset, preTexIndices);
        weights = preAccumValues >= 0.0f ? weights : 0;
        float weightSum = dot(1, weights);
    
        float tspp = 0;
        if (weightSum > 1e-3f)
        {
            float4 nWeights = weights / weightSum;
        
            float4 preTspp = g_preAccumMap.GatherRed(g_samplerPointClamp, preTopLeftTex).wzxy;
            tspp = round(dot(nWeights, preTspp));
            
            if (tspp > 0)
            {
                accumValue = dot(nWeights, preAccumValues);
            }
            else
            {
                tspp = 0;
            }
        }
        
        accumMap.x = tspp;
    }
    
    g_outAccumFrame[curIndex] = accumValue;
    g_outAccumMap[curIndex] = accumMap;
}