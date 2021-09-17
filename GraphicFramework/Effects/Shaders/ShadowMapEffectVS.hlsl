#include "../../Core/ShaderUtility/SceneGeometryOnlyRenderUtil.hlsl"

cbuffer cb_Shadow : register(b0, space0)
{
    float4x4 g_ShadowViewProj;
};

struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TexCoord0;
};

VSOutput main(VSInput vsInput, uint instanceID : SV_InstanceID)
{
    InstanceData instanceData = g_InstanceData[instanceID];

    VSOutput vsOutput;
    float3 worldPos = mul(instanceData.world, float4(vsInput.pos, 1.0)).xyz;
    vsOutput.position = mul(g_ShadowViewProj, float4(worldPos, 1.0));
    vsOutput.texCoord = vsInput.tex;

    return vsOutput;
}
