#include "../../Core/ShaderUtility/MathUtil.hlsl"
#include "../../Core/ShaderUtility/RenderCommonUtil.hlsl"


cbuffer cb_AccumMap : register(b0)
{
    float       g_reprojTestFactor;
    uint        g_dynamicMarkMaxLife;
    uint        g_winWidth;
    uint        g_winHeight;
};

Texture2D<float>    g_preLinearDepth	    : register(t0);
Texture2D<float>    g_curLinearDepth        : register(t1);
Texture2D<float2>   g_motionVec             : register(t2);
Texture2D<int>      g_dynmiacAreaMark       : register(t3);
Texture2D<int4>     g_idMap                 : register(t4);
Texture2D<int4>     g_preAccumMap           : register(t5);

RWTexture2D<int4>   g_outAccumMap           : register(u0); // AccumCount(int), NumSampleCount(int), DynamicInstanceHint(int), DynamicMark(int)

SamplerState g_samplerLinearBorder : register(s0);



[numthreads(8, 8, 1)]
void main(uint2 Gid : SV_GroupID, uint2 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex, uint2 DTid : SV_DispatchThreadID)
{
    uint2 curIndex = DTid;
    float2 curTex = index2Texcoord(uint2(g_winWidth, g_winHeight), curIndex);
    float2 preTex = curTex + g_motionVec[curIndex].xy;
    uint2 preIndex = texcoord2Index(uint2(g_winWidth, g_winHeight), preTex);
    
    float curZ = g_curLinearDepth.SampleLevel(g_samplerLinearBorder, curTex, 0.0f).x;
    float preZ = g_preLinearDepth.SampleLevel(g_samplerLinearBorder, preTex, 0.0f).x;
    bool reprojTest = verifyTexcoord(preTex) && (abs(1.0f - curZ / preZ) < g_reprojTestFactor);
    if (!reprojTest)
    {
        g_outAccumMap[curIndex] = int4(0, 0, 0, -1);
        return;
    }
    
    int dynamicAreaMark = g_dynmiacAreaMark[curIndex];
    
    int4 ids = g_idMap[curIndex];  
    int dynamicInstanceHint = (ids.w == int(DYNAMIC_INSTANCE_TAG));
     
    int4 accumMap = g_preAccumMap[preIndex].xyzw;
    
    accumMap.y = 0;
    accumMap.z = dynamicInstanceHint;
    accumMap.w = dynamicAreaMark;

    g_outAccumMap[curIndex] = accumMap;
}