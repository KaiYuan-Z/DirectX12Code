struct VertexIn
{
	float3 PosL    : POSITION;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosL : POSITION;
};
 

cbuffer SkyCB : register(b0, space0)
{
    float4x4 g_ViewProj;
    float3   g_EyePosW;
    float    pad;
};
 
VertexOut main(VertexIn vin)
{
	VertexOut vout;

	// Use local vertex position as cubemap lookup vector.
    vout.PosL = vin.PosL;
	
	// Transform to world space.
    float4 posW = float4(vin.PosL * 200.0f, 1.0f);

	// Always center sky about camera.
    posW.xyz += g_EyePosW;

	// Set z = w so that z/w = 1 (i.e., skydome always on far plane).
    vout.PosH = mul(g_ViewProj, posW).xyww;
	
	return vout;
}

