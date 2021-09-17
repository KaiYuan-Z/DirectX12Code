#include "RenderCommonUtil.hlsl"

struct VSInput
{
    float3 pos : POSITION;
    float2 tex : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

cbuffer cb_PerFrame : register(b0, space1)
{
    float4x4 g_PreView;
    float4x4 g_CurView;
    float4x4 g_Proj;
    float4x4 g_CurViewProjWithOutJitter;
    float4x4 g_PreViewProjWithOutJitter;
};

cbuffer cb_MeshInfo : register(b1, space1)
{
    uint g_MeshIndex;
    bool g_UseAlphaClip;
};

StructuredBuffer<InstanceData> g_InstanceData : register(t0, space1);
StructuredBuffer<float4x4> g_PreInstanceWorld : register(t1, space1);

Texture2D<float4> g_TexDiffuse : register(t2, space1);

SamplerState g_samLinear : register(s0, space1);

void alphaClip(float2 tex, float clipValue)
{
    if (g_UseAlphaClip)
    {
        clip(g_TexDiffuse.Sample(g_samLinear, tex).a - clipValue);
    }
}