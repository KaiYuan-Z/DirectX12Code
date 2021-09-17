#include "../../Core/ShaderUtility/SceneGeometryOnlyRenderUtil.hlsl"
#include "../../Core/ShaderUtility/MathUtil.hlsl"

struct VSOutput
{
    float4 pos          : SV_Position;
    float3 prePosW      : PrePosW;
    float3 posW         : PosW;
    float3 normal       : Normal;
    float2 texcoord     : Texcoord;
    nointerpolation uint instanceID : InstanceID;
};

VSOutput main(VSInput vsInput, uint instanceID : SV_InstanceID)
{
    InstanceData instanceData = g_InstanceData[instanceID];

    VSOutput vsOutput;
    vsOutput.posW = mul(instanceData.world, float4(vsInput.pos, 1.0)).xyz;
    vsOutput.pos = mul(g_Proj, mul(g_CurView, float4(vsOutput.posW, 1.0)));
    vsOutput.normal = mul((float3x3) instanceData.world, vsInput.normal);
    vsOutput.instanceID = instanceID;
    vsOutput.prePosW = mul(g_PreInstanceWorld[instanceID], float4(vsInput.pos, 1.0)).xyz;
    vsOutput.texcoord = vsInput.tex;
    
    if ((instanceData.instanceTag & DEFORMABLE_INSTANCE_TAG) > 0)
    {
        vsOutput.prePosW = mul(instanceData.world, float4(vsInput.tangent, 1.0)).xyz;
    }

    return vsOutput;
}
