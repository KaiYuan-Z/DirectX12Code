#include "../../Core/ShaderUtility/LightingUtil.hlsl"
#include "../../Core/ShaderUtility/VertexUtil.hlsl"
#include "../../Core/ShaderUtility/RenderCommonUtil.hlsl"
#include "RaytracingUtil.hlsl"


struct RtMaterial
{
    float4  diffuseAlbedo;
    float3  fresnelR0;
    float   shininess;
    uint    texDiffuseIndex;
    uint    texSpecularIndex;
    uint    texEmissiveIndex;
    uint    texNormalIndex;
    uint    texLightmapIndex;
    uint    texReflectionIndex;
    uint    pad[2];
};

struct RtMeshData
{
    uint    materialIndex;
    uint    indexOffsetBytes;
    uint    vertexOffsetBytes;
    uint    vertexStride;
    uint    indexTypeSize;
    uint    pad[3];
};

struct RtInstanceData
{
    uint    instanceIndex;
    uint    modelIndex;
    uint    instanceTag;
    uint    firstMeshGlobalIndex;
};

cbuffer cb_Scene : register(b0, space1)
{
    uint        g_instanceCount;
    uint        g_lightCount;
    uint        g_hitProgCount;
    uint        pad_of_cb_acene;
};

cbuffer cb_Frame : register(b1, space1)
{
    float4x4    g_viewProj;
    
    float4x4    g_invViewProj;
    
    float3      g_cameraPosition;
    uint        g_frameID;
    
    float2      g_jitter;
    float2      pad_of_cb_frame;
};

cbuffer cb_HaltonEnum : register(b2, space1)
{
    uint g_p2; // Smallest integer with 2^g_p2 >= width.
    uint g_p3; // Smallest integer with 3^g_p3 >= height.
    uint g_x; // 3^g_p3 * ((2^g_p2)^(-1) mod 3^g_p3).
    uint g_y; // 2^g_p2 * ((3^g_p3)^(-1) mod 2^g_p2).
    float g_scale_x; // 2^g_p2.
    float g_scale_y; // 3^g_p3.
    uint g_increment; // Product of prime powers, i.e. m_res2 * m_res3.
    uint g_max_samples;
};

cbuffer cb_HitMeshInfo : register(b0, space2)
{
    uint g_meshGlobalIndex;
    uint g_meshLocalIndex;
};

RaytracingAccelerationStructure         g_Scene         : register(t0, space1);
StructuredBuffer<RtMaterial>            g_Materials     : register(t1, space1);
StructuredBuffer<RtMeshData>            g_Meshes        : register(t2, space1);
StructuredBuffer<RtInstanceData>        g_Instances     : register(t3, space1);
StructuredBuffer<LightData>             g_Lights        : register(t4, space1);
StructuredBuffer<PosNormTexTanBitan>    g_Vertices      : register(t5, space1);
ByteAddressBuffer                       g_Indices       : register(t6, space1);
StructuredBuffer<uint>                  g_HaltonPerm3   : register(t7, space1);
StructuredBuffer<uint>                  g_InstanceUpdateFrameIds : register(t8, space1);
StructuredBuffer<float4x4>              g_InstanceWorlds : register(t9, space1);
Texture2D<float4>                       g_Textures[]    : register(t10, space1);

SamplerState g_samLinear : register(s0, space1);


bool dynamicInstance()
{
    uint instanceIndex = InstanceIndex();
    uint dynamicHint = g_Instances[instanceIndex].instanceTag & DYNAMIC_INSTANCE_TAG;
    return (dynamicHint > 0);
}

uint3 hitTriangleIndices()
{
    const uint meshID = g_meshGlobalIndex;
    uint meshFirstVertexIndex = g_Meshes[meshID].vertexOffsetBytes / g_Meshes[meshID].vertexStride;
    const uint3 indices = load3x32BitIndices(g_Indices, g_Meshes[meshID].indexOffsetBytes, PrimitiveIndex());
    return uint3(indices[0] + meshFirstVertexIndex, indices[1] + meshFirstVertexIndex, indices[2] + meshFirstVertexIndex);
}

uint3 getTriangleIndices(uint instanceID, uint meshID, uint primitiveIndex)
{
    const uint meshGlobalID = g_Instances[instanceID].firstMeshGlobalIndex + meshID;
    uint meshFirstVertexIndex = g_Meshes[meshGlobalID].vertexOffsetBytes / g_Meshes[meshGlobalID].vertexStride;
    const uint3 indices = load3x32BitIndices(g_Indices, g_Meshes[meshGlobalID].indexOffsetBytes, primitiveIndex);
    return uint3(indices[0] + meshFirstVertexIndex, indices[1] + meshFirstVertexIndex, indices[2] + meshFirstVertexIndex);
}

bool isDynamicInstance(uint instanceIndex)
{
    if (instanceIndex < g_instanceCount)
    {
        uint dynamicHint = g_Instances[instanceIndex].instanceTag & DYNAMIC_INSTANCE_TAG;
        uint updateFrameID = g_InstanceUpdateFrameIds[instanceIndex];
        return (dynamicHint > 0) && (updateFrameID == g_frameID);
    }
    
    return false;
}

bool hitDynamic()
{
    uint instanceIndex = InstanceIndex();
    uint dynamicHint = g_Instances[instanceIndex].instanceTag & DYNAMIC_INSTANCE_TAG;
    uint updateFrameID = g_InstanceUpdateFrameIds[instanceIndex];
    return (dynamicHint > 0) && (updateFrameID == g_frameID);
}

bool hitSameInstance(uint instanceId)
{
    uint hitInstanceId = InstanceIndex();
    return (instanceId == hitInstanceId);
}

bool hitSameInstanceMesh(uint instanceId, uint localMeshId)
{
    uint hitInstanceId = InstanceIndex();
    uint hitModelMeshId = g_meshLocalIndex;
    return ((hitInstanceId == instanceId) && (hitModelMeshId == localMeshId));
}

bool hitSameModelMesh(uint modelId, uint localMeshId)
{
    uint hitInstanceId = InstanceIndex();
    uint hitModelId = g_Instances[hitInstanceId].modelIndex;
    uint hitModelMeshId = g_meshLocalIndex;
    return ((hitModelId == modelId) && (hitModelMeshId == localMeshId));
}

void alphaClipInAnyHit(BuiltInTriangleIntersectionAttributes attr, float clipValue = 0.3f)
{
    const uint3 indices = hitTriangleIndices();
    float3 vertexTex[3] =
    {
        float3(g_Vertices[indices[0]].texcoord, 0.0f),
		float3(g_Vertices[indices[1]].texcoord, 0.0f),
		float3(g_Vertices[indices[2]].texcoord, 0.0f)

    };
    float2 tex = hitAttribute(vertexTex, attr).xy;
    
    uint matIndex = g_Meshes[g_meshGlobalIndex].materialIndex;
    uint diffuseIndex = g_Materials[matIndex].texDiffuseIndex;
    float a = g_Textures[diffuseIndex].SampleLevel(g_samLinear, tex, 0).a;
   
    if (a < clipValue)
    {
        IgnoreHit();
    }
}


//
// Halton Functons
//
// Special case: radical inverse in base 2, with direct bit reversal.
float halton2(uint index)
{
    index = (index << 16) | (index >> 16);
    index = ((index & 0x00ff00ff) << 8) | ((index & 0xff00ff00) >> 8);
    index = ((index & 0x0f0f0f0f) << 4) | ((index & 0xf0f0f0f0) >> 4);
    index = ((index & 0x33333333) << 2) | ((index & 0xcccccccc) >> 2);
    index = ((index & 0x55555555) << 1) | ((index & 0xaaaaaaaa) >> 1);
    // Write reversed bits directly into floating-point mantissa.
    uint result = 0x3f800000u | (index >> 9);
    return asfloat(result) - 1.f;
}

float halton3(uint index)
{
    return (g_HaltonPerm3[index % 243u] * 14348907u +
            g_HaltonPerm3[(index / 243u) % 243u] * 59049u +
            g_HaltonPerm3[(index / 59049u) % 243u] * 243u +
            g_HaltonPerm3[(index / 14348907u) % 243u]) * float(0x1.fffffcp-1 / 3486784401u); // Results in [0,1).
}

uint halton2_inverse(uint index, uint digits)
{
    index = (index << 16) | (index >> 16);
    index = ((index & 0x00ff00ff) << 8) | ((index & 0xff00ff00) >> 8);
    index = ((index & 0x0f0f0f0f) << 4) | ((index & 0xf0f0f0f0) >> 4);
    index = ((index & 0x33333333) << 2) | ((index & 0xcccccccc) >> 2);
    index = ((index & 0x55555555) << 1) | ((index & 0xaaaaaaaa) >> 1);
    return index >> (32 - digits);
}

uint halton3_inverse(uint index, uint digits)
{
    uint result = 0;
    for (uint d = 0; d < digits; ++d)
    {
        result = result * 3 + index % 3;
        index /= 3;
    }
    return result;
}

uint get_index(uint x, uint y, uint i)
{
    /*
     Ref: L.Gruenschloss, M.Raab, and A.Keller:
     "Enumerating Quasi-Monte Carlo Point Sequences in Elementary Intervals".
    */
    
    // Promote to 64 bits to avoid overflow.
    const uint hx = halton2_inverse(x, g_p2);
    const uint hy = halton3_inverse(y, g_p3);
    // Apply Chinese remainder theorem.
    const uint offset = (hx * g_x + hy * g_y) % g_increment;
    return offset + i * g_increment;
}

float scale_x(const float x)
{
    return x * g_scale_x;
}

float scale_y(const float y)
{
    return y * g_scale_y;
}

float2 halton23(uint x, uint y, uint i)
{
    /*
     Ref: L.Gruenschloss, M.Raab, and A.Keller:
     "Enumerating Quasi-Monte Carlo Point Sequences in Elementary Intervals".
    */
    
    uint index = get_index(x, y, i);
    float u1 = frac(scale_x(halton2(index)));
    float u2 = frac(scale_y(halton3(index)));
    return float2(u1, u2);
}