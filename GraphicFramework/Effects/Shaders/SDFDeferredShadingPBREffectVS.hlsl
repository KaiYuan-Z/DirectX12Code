struct VSInput
{
	float3 position : POSITION;
	float2 texcoord0 : TEXCOORD;
};

cbuffer cbPerframe : register(b0)
{
    float4x4 g_InvProj;
    float3 g_CameraPosW;
    uint g_LightRenderCount;
    float3 g_AmbientLight;
    bool g_SDFEffectEnable;
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
    float4 ph = mul(g_InvProj, vout.PosH);
	vout.PosV = ph.xyz / ph.w;

	return vout;
}