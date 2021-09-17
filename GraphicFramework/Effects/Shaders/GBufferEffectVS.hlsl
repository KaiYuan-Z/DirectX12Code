#include "../../Core/ShaderUtility/SceneGBufferRenderUtil.hlsl"

struct VSOutput
{
    float4 pos : SV_Position;
    float3 posW : PosW;
    float3 prePosW : PrePosW;
    float3 normal : Normal;
    float3 bitangent : Bitangent;
    float2 tex : TexCoord0;
    nointerpolation uint instanceID : InstanceID;
};

VSOutput main(VSInput vsInput, uint instanceID : SV_InstanceID)
{
    InstanceData instanceData = g_InstanceData[instanceID];

    VSOutput vsOutput;
    vsOutput.posW = mul(instanceData.world, float4(vsInput.pos, 1.0)).xyz;
    vsOutput.pos = mul(g_Proj, mul(g_CurView, float4(vsOutput.posW, 1.0)));
    vsOutput.prePosW = mul(g_PreInstanceWorld[instanceID], float4(vsInput.pos, 1.0)).xyz;
    vsOutput.normal = mul((float3x3) instanceData.world, vsInput.normal);
    vsOutput.bitangent = mul((float3x3) instanceData.world, vsInput.bitangent);
    vsOutput.tex= vsInput.tex;
    vsOutput.instanceID = instanceID;
    
    if ((instanceData.instanceTag & DEFORMABLE_INSTANCE_TAG) > 0)
    {
        vsOutput.prePosW = mul(instanceData.world, float4(vsInput.tangent, 1.0)).xyz;
    }
  
    return vsOutput;
}
