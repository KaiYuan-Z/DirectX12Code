#define HLSL
#include "../ShaderUtility/MathUtil.hlsl"
#include "../ShaderUtility/VertexUtil.hlsl"

#define N_GRASS_TRIANGLES 5
#define N_GRASS_VERTICES 7
#define MAX_GRASS_STRAWS_1D 100
struct GenerateGrassStrawsConstantBuffer_AppParams
{
    uint2 activePatchDim; // Dimensions of active grass straws.
    uint2 maxPatchDim; // Dimensions of the whole vertex buffer.

    float2 timeOffset;
    float grassHeight;
    float grassScale;

    float3 patchSize;
    float grassThickness;

    float3 windDirection;
    float windStrength;

    float positionJitterStrength;
    float bendStrengthAlongTangent;
    float padding[2];
};

struct GenerateGrassStrawsConstantBuffer
{
    float2 invActivePatchDim;
    float padding1;
    float padding2;
    GenerateGrassStrawsConstantBuffer_AppParams p;
};

Texture2D<float4>                           g_windMap : register(t0);
RWStructuredBuffer<PosNormTexTanBitan>      g_inoutVertexBuffer : register(u0);

ConstantBuffer<GenerateGrassStrawsConstantBuffer> cb : register(b0);
SamplerState WrapLinearSampler : register(s0);

#if N_GRASS_TRIANGLES != 5 || N_GRASS_VERTICES != 7
On change, update triangle/vertex definitiions.
#endif


#define N_TRIANGLES 5
#define GRASS_TYPES 3
static const float GRASS_X[GRASS_TYPES][N_GRASS_VERTICES] = {
    // Tall straight grass:
    {-0.329877, 0.329877, -0.212571, 0.212571, -0.173286, 0.173286, 0.000000 },
    // Tall bent grass:
    { -0.435275, 0.435275, 0.037324, 0.706106, 1.814639, 2.406257, 3.000000 },
    // Chubby double grass:
    { -1.200000, -0.400000, -0.200000, 0.400000, 0.400000, 1.000000, 1.300000 }
};

static const float GRASS_Y[GRASS_TYPES][N_GRASS_VERTICES] = {
    // Tall straight grass:
    { 0.000000, 0.000000, 2.490297, 2.490297, 4.847759, 4.847759, 8.000000 },
    // Tall bent grass:
    { 0.000000, 0.000000, 3.691449, 3.691449, 7.022911, 7.022911, 8.000000 },
    // Chubby double grass:
    { 1.800000, 1.000000, 0.000000, 0.000000, 1.800000, 1.800000, 4.000000 }
};

static const float2 TRIANGLE_TEX[N_GRASS_VERTICES] =
{
    { 0.01f, 0.3f },
    { 0.99f, 0.3f },
    { 0.01f, 0.4f },
    { 0.99f, 0.5f },
    { 0.01f, 0.6f },
    { 0.99f, 0.7f },
    { 0.01f, 0.8f }
};
 
static const uint3 TRIANGLE_INDICES[N_GRASS_TRIANGLES] = { {0, 2, 1}, {1, 2, 3}, {2, 4, 3}, {3, 4, 5}, {4, 6, 5} };

float rand(float2 co)
{
    float a = 12.9898;
    float b = 78.233;
    float c = 43758.5453;
    float dt = dot(co.xy, float2(a, b));
    float sn = fmod(dt, 3.14);
    return frac(sin(sn) * c);
}

void GenerateGrassStraw(
    in uint baseVertexID, in float2 texCoord, in float3 rootPos, in float3 inNormal,
    in float3 base_u, in float3 base_v, in float grassX[N_GRASS_VERTICES], in float grassY[N_GRASS_VERTICES])
{
    uint randSeed = initRand(baseVertexID, 0);
    float2 noiseUV = {
        2 * nextRand(randSeed) - 1, // [0, 1) -> [-1, 1)
        2 * nextRand(randSeed) - 1
    };

    float3 up = float3(0, 1, 0);
    float3 tangent = (noiseUV.x * base_u + noiseUV.y * base_v);
    rootPos = rootPos 
            + cb.p.positionJitterStrength * tangent 
            - 0.001 * cb.p.grassHeight * up;           // Root it in the ground a bit for bottom vertices not to stick out.
    
    float2 windCoord = frac(texCoord + cb.p.timeOffset);
    float3 windNoise = g_windMap.SampleLevel(WrapLinearSampler, windCoord, 0).rbg;      // RGB -> RBG
    windNoise = 2 * windNoise - 1;                    // [0, 1] -> [-1, 1]
    float3 gradient = cb.p.windStrength * (0.5 * cb.p.windDirection + 2.5 * windNoise);
    
    gradient -= up * dot(up, gradient);             // Project onto xz-plane.
    float3 nTangent = normalize(tangent);
    gradient -= (1 - cb.p.bendStrengthAlongTangent) * nTangent * dot(nTangent, gradient);   // Restrain bending along tangent.

    float3 vertexPos[N_GRASS_VERTICES];
    for (uint i = 0; i < N_GRASS_VERTICES; i++) 
    {
        float bendInterp = pow(grassY[i] / grassY[N_GRASS_VERTICES - 1], 1.5);
        float3 y = cb.p.grassScale * cb.p.grassHeight * inNormal *  grassY[i];
        float3 xz = cb.p.grassScale * cb.p.grassThickness * (grassX[i] * tangent + bendInterp * gradient);

        vertexPos[i] = rootPos + xz + y;
    }

    // Grass straw triangles and vertices:
    //   6
    //   | \
    //   |   5   t4
    //   | / |
    //   4   |   t3
    //   | \ |
    //   |   3   t2
    //   | / |
    //   2   |   t1
    //   | \ |
    //   0 _ 1   t0
    //

    // Calculate per-face normals
    float3 faceNormals[N_TRIANGLES];
    for (uint t = 0; t < N_TRIANGLES; t++)
    {
        uint3 indices = TRIANGLE_INDICES[t];
        float3 vertices[3] = { vertexPos[indices.x], vertexPos[indices.y], vertexPos[indices.z] };
        faceNormals[t] = normalize(cross(vertices[1] - vertices[0], vertices[2] - vertices[0]));
    }

    // Calculate per-vertex normals
    float3 vertexNormal[N_GRASS_VERTICES] = {
        faceNormals[0],
        0.5f * (faceNormals[0] + faceNormals[1]),
        (1.f / 3) * (faceNormals[0] + faceNormals[1] + faceNormals[2]),
        (1.f / 3) * (faceNormals[1] + faceNormals[2] + faceNormals[3]),
        (1.f / 3) * (faceNormals[2] + faceNormals[3] + faceNormals[4]),
        0.5f * (faceNormals[3] + faceNormals[4]),
        faceNormals[4]
    };

    texCoord *= (0.5f * (noiseUV.x + 1));           // [-1, 1] -> [0, 1]

    for (uint v = 0; v < N_GRASS_VERTICES; v++)
    {
        PosNormTexTanBitan vertex;
        vertex.position = vertexPos[v];
        vertex.normal = vertexNormal[v];
        vertex.texcoord = TRIANGLE_TEX[v];
        vertex.tangent = g_inoutVertexBuffer[baseVertexID + v].position;
        vertex.bitangent = vertex.tangent;
        
        g_inoutVertexBuffer[baseVertexID + v] = vertex;
    }
}

[numthreads(8, 8, 1)]
void main(uint2 DTid : SV_DispatchThreadID)
{
    if (any(DTid >= cb.p.maxPatchDim))
    {
       return;
    }

    if (all(DTid < cb.p.activePatchDim))
    {
        float ddu = cb.invActivePatchDim.x;
        float ddv = cb.invActivePatchDim.y;
        float3 base_u = ddu * cb.p.patchSize.x * float3(1, 0, 0);
        float3 base_v = ddv * cb.p.patchSize.z * float3(0, 0, 1);
        float2 rootUV = float2((DTid.x + 0.5f) * ddu, (DTid.y + 0.5f) * ddv); // Center the uv at the root center.
        float3 rootPos = cb.p.patchSize * float3(rootUV.x, 0, rootUV.y);

        float3 surfaceNormal = float3(0, 1, 0);
        uint threadID = DTid.x + DTid.y * cb.p.maxPatchDim.x;
        uint baseVertexID = threadID * N_GRASS_VERTICES;

        uint grassType = dot(DTid & 1, 1) == 1;
        GenerateGrassStraw(baseVertexID, rootUV, rootPos, surfaceNormal, base_u, base_v, GRASS_X[grassType], GRASS_Y[grassType]);
    }
    else // Non-active geometry ~ make degenerate triangles to disable them in the acceleration structure builds.
    {
        PosNormTexTanBitan vertex;
        vertex.position = 0;
        vertex.normal = 0;
        vertex.texcoord = 0;
        vertex.tangent = 0;
        vertex.bitangent = 0;
        uint threadID = DTid.x + DTid.y * cb.p.maxPatchDim.x;
        uint baseVertexID = threadID * N_GRASS_VERTICES;

        for (uint v = 0; v < N_GRASS_VERTICES; v++)
        {
            g_inoutVertexBuffer[baseVertexID + v] = vertex;
        }
    }
}
