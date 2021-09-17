struct VertexOut
{
	float4 PosH : SV_POSITION;
    float3 PosL : POSITION;
};
 
TextureCube<float4> g_SkyMap : register(t0, space0);

SamplerState g_samLinear : register(s0, space0);

float4 main(VertexOut pin) : SV_Target
{
    return g_SkyMap.Sample(g_samLinear, pin.PosL).rgba;
}

