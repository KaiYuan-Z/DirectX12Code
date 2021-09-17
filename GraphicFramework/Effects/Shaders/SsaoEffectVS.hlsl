struct VSInput
{
	float3 position : POSITION;
	float2 texcoord0 : TEXCOORD;
};

cbuffer cbPerframe : register(b0)
{
	float4x4 g_View;
	float4x4 g_Proj;
	float4x4 g_InvProj;
	float4x4 g_ProjTex;
};

struct VertexOut
{
	float4 PosH : SV_POSITION;
	float3 PosV : POSITION;
	float2 TexC : TEXCOORD0;
};

VertexOut main(VSInput vsInput)
{
	VertexOut vout;

	vout.TexC = vsInput.texcoord0;

	// Quad covering screen in NDC space.
	vout.PosH = float4(2.0f*vout.TexC.x - 1.0f, 1.0f - 2.0f*vout.TexC.y, 0.0f, 1.0f);

	// Transform quad corners to view space near plane.
	float4 ph = mul(vout.PosH, g_InvProj);
	vout.PosV = ph.xyz / ph.w;

	return vout;
}