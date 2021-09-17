struct VertexOut
{
    float4 PosH : SV_Position;
    float4 Color : Color;
};

float4 main(VertexOut pin) : SV_Target
{
    return pin.Color;
}


