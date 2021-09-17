
struct VSOutput
{
	float4 position : SV_Position;
	float2 tex0 : TEXCOORD0;
};

cbuffer cb_Ssaa : register(b0)
{
	uint g_SsaaRadius;
	uint g_HasCenter;
	uint g_SrvWidth;
	uint g_SrvHeight;
};

Texture2D<float> g_texture	: register(t0);

SamplerState g_samLinear  : register(s0);

float4 main(VSOutput vsOutput) : SV_Target0
{
	float SrvPixelStepX = 1.0f / float(g_SrvWidth);
	float SrvPixelStepY = 1.0f / float(g_SrvHeight);
	float RadiusLengthX = (g_SsaaRadius - 0.5f * (1 - g_HasCenter)) * SrvPixelStepX;
	float RadiusLengthY = (g_SsaaRadius - 0.5f * (1 - g_HasCenter)) * SrvPixelStepY;
	uint SsaaKernelSize = 2 * g_SsaaRadius + g_HasCenter;
	float SampleCount = SsaaKernelSize * SsaaKernelSize;

	float Color = 0.0f;
	float x = vsOutput.tex0.x - RadiusLengthX;
	for (uint xn = 0; xn < 2 * g_SsaaRadius + g_HasCenter; xn++)
	{
		float y = vsOutput.tex0.y - RadiusLengthY;
		for (uint yn = 0; yn < SsaaKernelSize; yn++)
		{
			Color += g_texture.Sample(g_samLinear, float2(x, y));
			y += SrvPixelStepY;
		}
		x += SrvPixelStepX;
	}	

	return Color / SampleCount;
}
