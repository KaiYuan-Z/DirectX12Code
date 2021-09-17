#include "../../../GraphicFramework/Core/ShaderUtility/SceneForwardRenderUtil.hlsl"


struct VSOutput
{
    float4 position : SV_Position;
    float3 worldPos : WorldPos;
    float2 texCoord : TexCoord0;
    float3 normal : Normal;
    float3 tangent : Tangent;
    float3 bitangent : Bitangent;
    nointerpolation uint sceneModelIndex : SceneModelIndex;
    nointerpolation uint instanceIndex : ModelInstanceIndex;
};

VSOutput main(VSInput vsInput, uint instanceID : SV_InstanceID)
{
    InstanceData instanceData = g_InstanceData[instanceID];

    VSOutput vsOutput;
    vsOutput.worldPos = mul(instanceData.world, float4(vsInput.position, 1.0)).xyz;
    vsOutput.position = mul(g_ViewProj, float4(vsOutput.worldPos, 1.0));
    vsOutput.texCoord = vsInput.texcoord0;
    vsOutput.normal = mul((float3x3)instanceData.world, vsInput.normal);
    vsOutput.tangent = mul((float3x3)instanceData.world, vsInput.tangent);
    vsOutput.bitangent = mul((float3x3)instanceData.world, vsInput.bitangent);
    vsOutput.sceneModelIndex = instanceData.sceneModelIndex;
    vsOutput.instanceIndex = instanceData.instanceIndex;

    return vsOutput;
}