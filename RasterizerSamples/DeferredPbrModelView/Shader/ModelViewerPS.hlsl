#define DIFFUSE_FROSTBITE_BRDF

#include "../../../GraphicFramework/Core/ShaderUtility/SceneLightDeferredRenderUtil.hlsl"


struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TexCoord0;
};

float4 main(VSOutput vsOutput) : SV_Target0
{
    GBufferResult gb = prepareGBufferResult(vsOutput.texCoord);
    clip(gb.modelId);
    ShadingResult shadingResult = deferredPbrShading(gb);
    float3 color = shadingResult.ambient + shadingResult.diffuse + shadingResult.specular;
    // HDR tonemapping
    color = color / (color + float3(1.0f, 1.0f, 1.0f));
    // gamma correct
    color = pow(color, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));
    return float4(color, shadingResult.alpha);
}
