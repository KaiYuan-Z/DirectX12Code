
struct VSOutput
{
	float4 position : SV_Position;
	float2 tex0 : TEXCOORD0;
};

Texture2D<float> g_texture	: register(t0);

SamplerState g_samLinear  : register(s0);

float4 main(VSOutput vsOutput) : SV_Target0
{
	return g_texture.Sample(g_samLinear, vsOutput.tex0);
}
