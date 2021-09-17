#include "../../Core/ShaderUtility/SceneGeometryOnlyRenderUtil.hlsl"

struct VSOutput
{
    float4 position : SV_Position;
    float2 texCoord : TexCoord0;
};

void main(VSOutput vsOutput)
{
    float4 diffuseAlbedo = g_TexDiffuse.Sample(g_samLinear, vsOutput.texCoord);
    clip(diffuseAlbedo.a - 0.3f);
}
