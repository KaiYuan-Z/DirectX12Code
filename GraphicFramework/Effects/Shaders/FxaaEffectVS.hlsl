struct VSInput
{
	float3 position : POSITION;
	float2 tex : TEXCOORD;
};

struct VSOutput
{
	float4 position : SV_Position;
	float2 tex0 : TEXCOORD0;
};


VSOutput main(VSInput vsInput)
{
	VSOutput vsOutput;

	vsOutput.position = float4(vsInput.position, 1.0);
	vsOutput.tex0 = vsInput.tex;

	return vsOutput;
}
