cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj; 
};

struct VertexIn
{
	float3 PosL  : Position;
    float4 Color : Color;
};

struct VertexOut
{
    float4 PosH : SV_Position;
    float4 Color : Color;
};

VertexOut main(VertexIn vin)
{
	VertexOut vout;
	
	// Transform to homogeneous clip space.
	vout.PosH = mul(gWorldViewProj, float4(vin.PosL, 1.0f));
	
	// Just pass vertex color into the pixel shader.
    vout.Color = vin.Color;
    
    return vout;
}

