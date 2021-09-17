

struct VSOutput
{
    float4 position : SV_Position;
    float2 tex0 : TEXCOORD0;
};

Texture2D<int4> g_texture : register(t0);

float4 main(VSOutput vsOutput) : SV_Target0
{
    uint width, height, numMips;
    g_texture.GetDimensions(0, width, height, numMips);
    uint2 index = uint2(width, height) * vsOutput.tex0;
    int4 accumMap = g_texture[index];
    
    //Debug Accum Count  
    //{
    //    const int c_max_accum_count = 150;
    //    int accumCount = accumMap.x;
    //    return float4(0.0f, accumCount, 0.0f, 1.0f) / float(c_max_accum_count);
    //}

    //Debug New Sample Count
    {
        const int c_medSampleCount = 8;
        const int c_maxSampleCount = 16;
        int sampleCount = accumMap.y;
        if (sampleCount < c_medSampleCount)
        {
            return float4(0.0f, 0.0f, 1.0f, 1.0f) ;
        }
        else if (sampleCount < c_maxSampleCount)
        {
            return float4(0.0f, 1.0f, 0.0f, 1.0f);
        }
        else
        {
            return float4(1.0f, 0.0f, 0.0f, 1.0f);
        }
    }
    
    //Debug Dynamic Marks
    //{   
    //    if (accumMap.z > 0)
    //    {
    //        return float4(1.0f, 0.0f, 0.0f, 1.0f);
    //    }
    //    else if (accumMap.w > 0)
    //    {
    //        return float4(1.0f, 1.0f, 0.0f, 1.0f);
    //    }

    //    return float4(0.0f, 0.0f, 0.0f, 1.0f);
    //}
}
