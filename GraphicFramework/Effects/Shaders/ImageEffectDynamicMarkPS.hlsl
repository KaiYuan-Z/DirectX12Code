
struct VSOutput
{
	float4 position : SV_Position;
	float2 tex0 : TEXCOORD0;
};

Texture2D<int> g_texture	: register(t0);

float4 main(VSOutput vsOutput) : SV_Target0
{
    int x, y;
    g_texture.GetDimensions(x, y);
    int2 index = vsOutput.tex0 * int2(x, y);
    
    if (g_texture[index] > 0)
    {
        return float4(0.0f, 1.0f, 0.0f, 1.0f);
    }
    else
    {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }
}
