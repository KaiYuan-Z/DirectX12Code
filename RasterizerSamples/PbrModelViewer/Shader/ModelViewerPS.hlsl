#define DIFFUSE_FROSTBITE_BRDF

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

float4 main(VSOutput vsOutput) : SV_Target0
{
    ShadingResult shadingResult = pbrShading(vsOutput.worldPos, vsOutput.normal, vsOutput.tangent, vsOutput.bitangent, vsOutput.texCoord);
    float3 color = shadingResult.ambient + shadingResult.diffuse + shadingResult.specular;
     // HDR tonemapping
    color = color / (color + float3(1.0f, 1.0f, 1.0f));
    // gamma correct
    color = pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));
    return float4(color, shadingResult.alpha);
}
