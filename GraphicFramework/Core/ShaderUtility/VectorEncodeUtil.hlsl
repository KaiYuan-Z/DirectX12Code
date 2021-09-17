//#include "MathUtil.hlsl"

/*
Ref: J. Smith, G. Petrova, S. Schaefe, Encoding Normal Vectors using Optimized SphericalCoordinates.
http://faculty.cs.tamu.edu/schaefer/research/normalCompression.pdf
*/

static const uint g_Nphi_64 = 8;
static const uint g_Nthetas_64[8] = { 1, 7, 11, 13, 13, 11, 7, 1 };
static const uint g_Offset_64[8] = { 0, 1, 8, 19, 32, 45, 56, 63 };
static const uint g_Itheta_64[64] =
{
    0, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 7
};

static const uint g_Nphi_124 = 10;
static const uint g_Nthetas_124[10] = { 1, 9, 14, 18, 20, 20, 18, 14, 9, 1 };
static const uint g_Offset_124[10] = { 0, 1, 10, 24, 42, 62, 82, 100, 114, 123 };
static const uint g_Itheta_124[124] =
{
    0, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 3, 3, 3, 3, 3, 3,
    3, 3, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 9
};



uint EncodeVectorNormalized_64(float3 baseX, float3 baseZ, float3 vecNormalized)
{
    float phi = getAngleNormalized(baseZ, vecNormalized);
    uint i = uint(phi * (g_Nphi_64 - 1) / M_PI + 0.5f);
     
    float theta = getHorizontalRotationAngleNormalized(baseX, baseZ, vecNormalized);
    uint Ntheta = g_Nthetas_64[i];
    uint j = uint(theta * Ntheta / M_PI2 + 0.5f);
    j = (j == Ntheta) ? 0 : j;

    return g_Offset_64[i] + j;
}

float3 DecodeVectorNormalized_64(float3 baseX, float baseY, float3 baseZ, uint index)
{
    uint i = g_Itheta_64[index];
    uint j = index - g_Offset_64[i];
    
    float phi = i * M_PI / (g_Nphi_64 - 1);
    float theta = j * M_PI2 / g_Nthetas_64[i];

    return sin(phi) * cos(theta) * baseX + sin(phi) * sin(theta) * baseY + cos(phi) * baseZ;
}



uint EncodeVectorNormalized_124(float3 baseX, float3 baseZ, float3 vecNormalized)
{
    float phi = getAngleNormalized(baseZ, vecNormalized);
    int i = int(phi * (g_Nphi_124 - 1) / M_PI + 0.5f);
     
    float theta = getHorizontalRotationAngleNormalized(baseX, baseZ, vecNormalized);
    int Ntheta = g_Nthetas_124[i];
    int j = int(theta * Ntheta / M_PI2 + 0.5f);
    j = (j == Ntheta) ? 0 : j;

    return g_Offset_124[i] + j;
}

float3 DecodeVectorNormalized_124(float3 baseX, float baseY, float3 baseZ, uint index)
{
    uint i = g_Itheta_124[index];
    uint j = index - g_Offset_124[i];
    
    float phi = i * M_PI / (g_Nphi_124 - 1);
    float theta = j * M_PI2 / g_Nthetas_124[i];

    return sin(phi)*cos(theta)*baseX + sin(phi)*sin(theta)*baseY + cos(phi)*baseZ;
}