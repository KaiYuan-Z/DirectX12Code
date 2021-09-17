#include "../../Core/ShaderUtility/MathUtil.hlsl"
#include "../../Core/ShaderUtility/RenderCommonUtil.hlsl"

#define KernelSize 4

cbuffer AccumFrameCB : register(b0, space0)
{
    uint  g_winWidth;
    uint  g_winHeight;   
    uint  g_maxAccumCount;
    bool  g_reuseDiffAo;
    
    float g_aoDiffFactor1;
    float g_aoDiffFactor2;
    float2 pad;
};

Texture2D<float>    g_inAccumFrame          : register(t0, space0);
Texture2D<float>    g_inFrame               : register(t1, space0);
Texture2D<float>    g_inFrameDiff           : register(t2, space0);
Texture2D<int4>     g_inAccumMap            : register(t3, space0);

RWTexture2D<float> g_outAccumFrame          : register(u0, space0);
RWTexture2D<int4>  g_outAccumMap            : register(u1, space0); // AccumCount(int), NumSampleCount(int), DynamicMeshHint(int), DynamicMark(int)


[numthreads(8, 8, 1)]
void main(uint2 Gid : SV_GroupID, uint2 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex, uint2 DTid : SV_DispatchThreadID)
{
    uint2 curIndex = DTid;
    uint2 quadIndex = curIndex / KernelSize;

    float curValue = g_inFrame[curIndex];
    float preValue = g_inAccumFrame[curIndex];
    int4 accumMap = g_inAccumMap[curIndex].xyzw; 
    float frameDiff = g_inFrameDiff[quadIndex];
    
    uint Tspp = accumMap.x;
    uint Nspp = accumMap.y;
    int dynamicInstanceHint = accumMap.z;
    int dynamicAreaMark = accumMap.w;
    
    if (dynamicAreaMark == DYNAMIC_INSTANCE_MARK_1)
    {
        frameDiff *= g_aoDiffFactor1;
    }
    else if (dynamicAreaMark == DYNAMIC_INSTANCE_MARK_2)
    {
        frameDiff *= g_aoDiffFactor2;
    }
  
    float newAccumValue = 0.0f;
    float newAccumSquaredMeanValue = 0.0f;  

    Tspp = clamp(Tspp + Nspp, 0, g_maxAccumCount);
        
    if (frameDiff > 0 && Tspp - Nspp > 0)
    {
        if (g_reuseDiffAo)
        {
            float a = float(Nspp) / float(Tspp);
            float b0 = 1.0f - a;
            a = clamp((1.0f - frameDiff) * a + frameDiff, 0.0f, 1.0f);
            float b = 1.0f - a;
            Tspp = round(lerp(Tspp - Nspp, 0, frameDiff)) + Nspp;
            newAccumValue = b * preValue + a * curValue; //(float(Tspp - Nspp) * preValue + float(Nspp) * curValue) / float(Tspp);
        }
        else
        {
            float a = 1.0f;
            float b = 0.0f;
            newAccumValue = b * preValue + a * curValue; //(float(Tspp - Nspp) * preValue + float(Nspp) * curValue) / float(Tspp);
        }
    }
    else
    {
        newAccumValue = (float(Tspp - Nspp) * preValue + float(Nspp) * curValue) / float(Tspp);;
    }
    
    accumMap.x = Tspp;
    
    g_outAccumMap[curIndex] = accumMap;
    g_outAccumFrame[curIndex] = newAccumValue;
}