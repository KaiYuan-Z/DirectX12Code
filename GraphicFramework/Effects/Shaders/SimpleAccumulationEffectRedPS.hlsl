
struct VSOutput
{
	float4 position : SV_Position;
	float2 tex0 : TEXCOORD0;
};

cbuffer AccumulationRootConstants : register(b0)
{
	uint g_accumCount;
};

Texture2D<float> g_lastFrame	: register(t0);
Texture2D<float> g_curFrame	: register(t1);

SamplerState g_samPoint  : register(s0);

float main(VSOutput vsOutput) : SV_Target0
{
	float curColor = g_curFrame.Sample(g_samPoint, vsOutput.tex0);
	float preColor = g_lastFrame.Sample(g_samPoint, vsOutput.tex0);

	return float(g_accumCount*preColor + curColor) / float(g_accumCount + 1);
}
