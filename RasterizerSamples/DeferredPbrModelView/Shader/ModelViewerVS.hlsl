#include "../../../GraphicFramework/Core/ShaderUtility/SceneLightDeferredRenderUtil.hlsl"


struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TexCoord0;
};

VSOutput main(VSInput vsInput)
{
    VSOutput vsOutput;
    
    vsOutput.position = float4(vsInput.position, 1.0);
    vsOutput.texCoord = vsInput.texcoord0;

    return vsOutput;
}