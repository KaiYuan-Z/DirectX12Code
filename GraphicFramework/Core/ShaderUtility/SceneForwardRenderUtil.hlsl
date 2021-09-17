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
    float3 position : POSITION;
    float2 texcoord0 : TEXCOORD;
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
    float4x4 g_ViewProj;
    float4   g_EyePosW;
    float4   g_AmbientLight;
    uint     g_LightRenderCount;
    uint     Pad;
    uint     g_EnableNormalMap;
    uint     g_EnableNormalMapCompression;
};

cbuffer cb_MeshMaterial : register(b1, space1)
{
    MeshMaterial g_MeshMaterial;
};

Texture2D<float4> g_TexDiffuse : register(t0, space1);
Texture2D<float3> g_TexSpecular : register(t1, space1);
Texture2D<float4> g_TexEmissive : register(t2, space1);
Texture2D<float3> g_TexNormal : register(t3, space1);
Texture2D<float4> g_TexLightmap : register(t4, space1);
Texture2D<float4> g_TexReflection : register(t5, space1);

StructuredBuffer<LightData>         g_LightData : register(t6, space1);
StructuredBuffer<uint>              g_LightRenderIndeces : register(t7, space1);
StructuredBuffer<InstanceData>      g_InstanceData : register(t8, space1);

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

float3 sampleNormalMap(float3 normal, float3 tangent, float3 bitangent, float2 tex)
{
    float3 N = g_TexNormal.Sample(g_samLinear, tex).rgb;
            
    if (!g_EnableNormalMapCompression)
        N = rgbToNormal(N);
    else
        N = rgToNormal(N.rg);

    bitangent = bitangent - normal * (dot(bitangent, normal));
    tangent = cross(bitangent, normal);
           
    float3x3 TBN = float3x3(normalize(tangent), normalize(bitangent), normalize(normal));

    return normalize(mul(N, TBN));
}

ShadingResult blinnPhongShading(float3 posW, float3 normal, float3 tangent, float3 bitangent, float2 tex)
{
    float3 V = normalize(g_EyePosW.xyz - posW);
	
    float3 N = normal;
    if (g_EnableNormalMap)
        N = sampleNormalMap(normal, tangent, bitangent, tex);

    float4 diffuseColor = g_TexDiffuse.Sample(g_samLinear, tex);

	//Alhpa Clip
    clip(diffuseColor.a - 0.3f);

    float4 diffuseAlbedo = float4(diffuseColor.xyz * g_MeshMaterial.diffuseAlbedo.xyz, 1.0f);
    float4 ambient = g_AmbientLight * diffuseAlbedo;

    MeshMaterial mat = { diffuseAlbedo, g_MeshMaterial.fresnelR0, g_MeshMaterial.shininess };

    float3 diffuse = { 0.0f, 0.0f, 0.0f };
    float3 specular = { 0.0f, 0.0f, 0.0f };
    LightSample lightSample;
    for (uint i = 0; i < g_LightRenderCount; i++)
    {
        lightSample = evalLight(g_LightData[g_LightRenderIndeces[i]], posW, N, V);
        evalBlinnPhong(lightSample, N, V, mat, diffuse, specular);
    }

    ShadingResult shadingResult;
    shadingResult.ambient = ambient.rgb;
    shadingResult.diffuse = diffuse;
    shadingResult.specular = specular;
    shadingResult.alpha = diffuseAlbedo.a;

    return shadingResult;
}

// Prepare the hit-point data
PbrParameter preparePbrParameter(float3 posW, float3 normal, float3 tangent, float3 bitangent, float2 tex)
{
    PbrParameter pp = initPbrParameter();

    // Sample the diffuse texture and apply the alpha test
    float4 baseColor = g_TexDiffuse.Sample(g_samLinear, tex);
    pp.albedo = baseColor;
    clip(baseColor.a - 0.3f);

    pp.posW = posW;
    pp.uv = tex;
    pp.V = normalize(g_EyePosW.xyz - posW);
    pp.N = normalize(normal);
 
    if (g_EnableNormalMap) pp.N = sampleNormalMap(normal, tangent, bitangent, tex);

    // Sample the spec texture
    float3 spec = g_TexSpecular.Sample(g_samLinear, tex).rgb;
    // R - Occlusion; G - Roughness; B - Metalness
    pp.diffuse = lerp(baseColor.rgb, float3(0.0f, 0.0f, 0.0f), spec.b);
    // UE4 uses 0.08 multiplied by a default specular value of 0.5 as a base, hence the 0.04
    pp.specular = lerp(float3(0.04f, 0.04f, 0.04f), baseColor.rgb, spec.b);
    pp.linearRoughness = spec.g;
    pp.linearRoughness = max(0.08, pp.linearRoughness); // Clamp the roughness so that the BRDF won't explode
    pp.roughness = pp.linearRoughness * pp.linearRoughness;
    pp.occlusion = spec.r;

    pp.NdotV = dot(pp.N, pp.V);

    return pp;
}

ShadingResult pbrShading(float3 posW, float3 normal, float3 tangent, float3 bitangent, float2 tex)
{
    PbrParameter pbrParameter = preparePbrParameter(posW, normal, tangent, bitangent, tex);

    LightSample lightSample;
    PbrResult pbrResult;
    ShadingResult shadingResult;
    shadingResult.diffuse = float3(0.0f, 0.0f, 0.0f);
    shadingResult.specular = float3(0.0f, 0.0f, 0.0f);
    for (uint i = 0; i < g_LightRenderCount; i++)
    {
        lightSample = evalLight(g_LightData[g_LightRenderIndeces[i]], posW, pbrParameter.N, pbrParameter.V);
        pbrResult = evalPBR(pbrParameter, lightSample);
        shadingResult.diffuse += pbrResult.diffuse;
        shadingResult.specular += pbrResult.specular;
    }
   
    shadingResult.ambient = g_AmbientLight.rgb * pbrParameter.albedo.rgb * (1.0f - pbrParameter.occlusion);
    shadingResult.alpha = pbrParameter.albedo.a;

    return shadingResult;
}