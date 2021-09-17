#include "LightingUtil.hlsl"
#include "RenderCommonUtil.hlsl"

struct ShadingResult
{
    float3 ambient;
    float3 diffuse;
    float3 specular;
    float alpha;
};

struct VSInput
{
    float3 pos : POSITION;
    float2 tex : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

struct GBufferData
{
    float3 posW;
    float3 N;
    float4 diffuse; // float3: albedo, float: alpha
    float4 specular; // Blinn Phong, float3: fresnelR0, float: shininess; PBR, float3:spec, float: unused
};

cbuffer cb_PerFrame : register(b0, space1)
{
    float4x4 g_PreView;
    float4x4 g_CurView;
    float4x4 g_Proj;
    float4x4 g_CurViewProjWithOutJitter;
    float4x4 g_PreViewProjWithOutJitter;
    uint     g_EnableNormalMap;
    uint     g_EnableNormalMapCompression;
    uint2    Pad;
};

cbuffer cb_MeshMaterial : register(b1, space1)
{
    MeshMaterial g_MeshMaterial;
};

cbuffer cb_MeshIndex : register(b2, space1)
{
    uint g_MeshIndex;
};

Texture2D<float4> g_TexDiffuse : register(t0, space1);
Texture2D<float3> g_TexSpecular : register(t1, space1);
Texture2D<float4> g_TexEmissive : register(t2, space1);
Texture2D<float3> g_TexNormal : register(t3, space1);
Texture2D<float4> g_TexLightmap : register(t4, space1);
Texture2D<float4> g_TexReflection : register(t5, space1);

StructuredBuffer<InstanceData> g_InstanceData : register(t6, space1);
StructuredBuffer<float4x4> g_PreInstanceWorld : register(t7, space1);

SamplerState g_samLinear : register(s0, space1);


// Convert RGB to normal
float3 rgbToNormal(float3 rgb)
{
    float3 n = rgb * 2 - 1;
    return normalize(n);
}

// Convert RG to normal
float3 rgToNormal(float2 rg)
{
    float3 n;
    n.xy = rg * 2 - 1;

    // Saturate because error from BC5 can break the sqrt
    n.z = saturate(dot(rg, rg)); // z = r*r + g*g
    n.z = sqrt(1 - n.z);
    return normalize(n);
}

float3 sampleNormalMap(float3 normal, float3 bitangent, float2 tex)
{
    float3 N = g_TexNormal.Sample(g_samLinear, tex).rgb;
            
    if (!g_EnableNormalMapCompression)
        N = rgbToNormal(N);
    else
        N = rgToNormal(N.rg);

    bitangent = bitangent - normal * (dot(bitangent, normal));
    float3 tangent = cross(bitangent, normal);
           
    float3x3 TBN = float3x3(normalize(tangent), normalize(bitangent), normalize(normal));

    return normalize(mul(N, TBN));
}

GBufferData prepareBlinnPhongGBuffer(float3 posW, float3 normal, float3 bitangent, float2 tex)
{
    GBufferData GBuffer;
    
    float3 N = normal;
    if (g_EnableNormalMap) N = sampleNormalMap(normal, bitangent, tex);

    float4 diffuseColor = g_TexDiffuse.Sample(g_samLinear, tex);

	//Alhpa Clip
    clip(diffuseColor.a - 0.3f);

    GBuffer.posW = posW;
    GBuffer.N = N;
    GBuffer.diffuse = float4(diffuseColor.xyz * g_MeshMaterial.diffuseAlbedo.xyz, diffuseColor.a);
    GBuffer.specular = float4(g_MeshMaterial.fresnelR0, g_MeshMaterial.shininess);

    return GBuffer;
}

GBufferData preparePbrGBuffer(float3 posW, float3 normal, float3 bitangent, float2 tex)
{
    GBufferData GBuffer;
    
    float4 baseColor = g_TexDiffuse.Sample(g_samLinear, tex);
    clip(baseColor.a - 0.3f);

    float3 N = normalize(normal);
    if (g_EnableNormalMap) N = sampleNormalMap(normal, bitangent, tex);

    float3 spec = g_TexSpecular.Sample(g_samLinear, tex).rgb;

    GBuffer.posW = posW;
    GBuffer.N = N;
    GBuffer.diffuse = baseColor;
    GBuffer.specular = float4(spec, 0.0f);

    return GBuffer;
}