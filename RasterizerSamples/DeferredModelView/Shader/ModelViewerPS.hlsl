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
    ShadingResult shadingResult = deferredBlinnPhongShading(gb);
    float3 color = shadingResult.ambient + shadingResult.diffuse + shadingResult.specular;
    return float4(color, shadingResult.alpha);
}