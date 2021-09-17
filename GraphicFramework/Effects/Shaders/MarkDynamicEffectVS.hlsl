struct VSInput
{
    float3 pos : Position;
    float4 color : Color;
};

struct VSOutput
{
    float4 extentPos : SV_Position;
    nointerpolation int instanceTag : InstanceTag;
};

cbuffer cb_PerFrame : register(b0, space0)
{
    float4x4 g_viewProj;
};

StructuredBuffer<float4x4>      g_BoundingBoxWorld : register(t0, space0);
StructuredBuffer<int>           g_BoundingBoxHostInstTag : register(t1, space0);

VSOutput main(VSInput vsInput, uint instanceID : SV_InstanceID)
{  
    float4x4 M = g_BoundingBoxWorld[instanceID];
    float4x4 MVP = mul(g_viewProj, M);
    
    int tag = g_BoundingBoxHostInstTag[instanceID];

    VSOutput vsOutput;
    vsOutput.extentPos = mul(MVP, float4(vsInput.pos, 1.0f));
    vsOutput.instanceTag = tag;

    return vsOutput;
}
