#define LIGHT_TYPE_UNKNOWN		0 
#define LIGHT_TYPE_POINT		1
#define LIGHT_TYPE_DIRECTIONAL	2

#include "MathUtil.hlsl"

struct LightData
{
    float3   posW;              ///< World-space position of the center of a light source
    uint     type;              ///< Type of the light source (see above)
    float3   dirW;              ///< World-space orientation of the light source
    float    openingAngle;      ///< For point (spot) light: Opening angle of a spot light cut-off, pi by default - full-sphere point light
    float3   intensity;         ///< Emitted radiance of th light source
    float    cosOpeningAngle;   ///< For point (spot) light: cos(openingAngle), -1 by default because openingAngle is pi by default
    float3   pad;
    float    penumbraAngle;     ///< For point (spot) light: Opening angle of penumbra region in radians, usually does not exceed openingAngle. 0.f by default, meaning a spot light with hard cut-off
};

struct LightSample
{
    float3  diffuse; // The light intensity at the surface location used for the diffuse term
    float3  specular; // The light intensity at the surface location used for the specular term. For light probes, the diffuse and specular components are different
    float3  L; // The direction from the surface to the light source
    float3  posW; // The world-space position of the light 
    float   NdotH; // Unclamped, can be negative
    float   NdotL; // Unclamped, can be negative
    float   LdotH; // Unclamped, can be negative
    float   distance; // Distance from the light-source to the surface
};

struct MeshMaterial
{
    float4 diffuseAlbedo;
    float3 fresnelR0;
    float  shininess;
};

struct PbrParameter
{
    float3 posW;
    float3 V;
    float3 N;
    float2 uv;
    float  NdotV;
    float4 albedo;
    float3 diffuse;
    float3 specular;
    float  linearRoughness;
    float  roughness;
    float  occlusion;
};

struct PbrResult
{
    float3 diffuseBrdf; // The result of the diffuse BRDF
    float3 specularBrdf; // The result of the specular BRDF
    float3 diffuse; // The diffuse component of the result
    float3 specular; // The specular component of the result
};

LightSample initLightSample()
{
    LightSample ls;
    ls.diffuse = float3(0.0f, 0.0f, 0.0f);
    ls.specular = float3(0.0f, 0.0f, 0.0f);
    ls.L = float3(0.0f, 0.0f, 0.0f);
    ls.posW = float3(0.0f, 0.0f, 0.0f);
    ls.NdotH = 0.0f;
    ls.NdotL = 0.0f;
    ls.LdotH = 0.0f;
    ls.distance = 0.0f;
    return ls;
}

PbrParameter initPbrParameter()
{
    PbrParameter pp;
    pp.posW = float3(0.0f, 0.0f, 0.0f);
    pp.V = float3(0.0f, 0.0f, 0.0f);
    pp.N = float3(0.0f, 0.0f, 0.0f);
    pp.uv = float2(0.0f, 0.0f);
    pp.NdotV = 0.0f;
    pp.albedo = float4(0.0f, 0.0f, 0.0f, 0.0f);
    pp.diffuse = float3(0.0f, 0.0f, 0.0f);
    pp.specular = float3(0.0f, 0.0f, 0.0f);
    pp.linearRoughness = 0.0f;
    pp.roughness = 0.0f;
    pp.occlusion = 0.0f;
    return pp;
}

PbrResult initPbrResult()
{
    PbrResult pr;
    pr.diffuseBrdf = float3(0.0f, 0.0f, 0.0f);
    pr.specularBrdf = float3(0.0f, 0.0f, 0.0f);
    pr.diffuse = float3(0.0f, 0.0f, 0.0f);
    pr.specular = float3(0.0f, 0.0f, 0.0f);
    return pr;
}


//================================
// Light Intensity Help Functions
//================================

void calcCommonLightProperties(float3 V, float3 L, float3 N, inout LightSample ls)
{
    float3 H = normalize(V + L);
    ls.NdotH = dot(N, H);
    ls.NdotL = dot(N, L);
    ls.LdotH = dot(L, H);
};

float getDistanceFalloff(float distSquared)
{
    float falloff = 1 / ((0.01 * 0.01) + distSquared); // The 0.01 is to avoid infs when the light source is close to the shading point
    return falloff;
}

// Evaluate a directional light source intensity/direction at a shading point
void evalDirectionalLight(LightData light, float3 posW, inout LightSample ls)
{
    ls.diffuse = light.intensity;
    ls.specular = light.intensity;
    ls.L = -normalize(light.dirW);
    float dist = length(posW - light.posW);
    ls.posW = posW - light.dirW * dist;
}

// Evaluate a point light source intensity/direction at a shading point
void evalPointLight(LightData light, float3 posW, inout LightSample ls)
{
    ls.posW = light.posW;
    ls.L = light.posW - posW;
    // Avoid NaN
    float distSquared = dot(ls.L, ls.L);
    ls.distance = (distSquared > 1e-5f) ? length(ls.L) : 0;
    ls.L = (distSquared > 1e-5f) ? normalize(ls.L) : 0;

    // Calculate the falloff
    float falloff = getDistanceFalloff(distSquared);

    // Calculate the falloff for spot-lights
    float cosTheta = -dot(ls.L, light.dirW); // cos of angle of light orientation
    if (cosTheta < light.cosOpeningAngle)
    {
        falloff = 0;
    }
    else if (light.penumbraAngle > 0)
    {
        float deltaAngle = light.openingAngle - acos(cosTheta);
        falloff *= saturate((deltaAngle - light.penumbraAngle) / light.penumbraAngle);
    }
    ls.diffuse = light.intensity * falloff;
    ls.specular = ls.diffuse;
}

// Evaluate a light source intensity/direction at a shading point
LightSample evalLight(LightData light, float3 posW, float3 N, float3 V)
{
    LightSample ls = initLightSample();
    if (light.type == LIGHT_TYPE_DIRECTIONAL)
    {
        evalDirectionalLight(light, posW, ls);
    }
    else if (light.type == LIGHT_TYPE_POINT)
    {
        evalPointLight(light, posW, ls);
    }

    calcCommonLightProperties(V, ls.L, N, ls);

    return ls;
};


//==============================
// Blinn Phong Help Functions
//==============================


// Schlick gives an approximation to Fresnel reflectance (see pg. 233 "Real-Time Rendering 3rd Ed.").
// R0 = ( (n-1)/(n+1) )^2, where n is the index of refraction.
float3 schlickFresnel(float3 R0, float3 N, float3 L)
{
    float cosIncidentAngle = saturate(dot(N, L));

    float f0 = 1.0f - cosIncidentAngle;
    float3 reflectPercent = R0 + (1.0f - R0) * (f0 * f0 * f0 * f0 * f0);

    return reflectPercent;
}

void evalBlinnPhong(LightSample ls, float3 N, float3 V, MeshMaterial mat, inout float3 diffuse, inout float3 specular)
{
    const float m = mat.shininess * 256.0f;
    float3 halfVec = normalize(V + ls.L);

    float roughnessFactor = (m + 8.0f) * pow(max(dot(halfVec, N), 0.0f), m) / 8.0f;
    float3 fresnelFactor = schlickFresnel(mat.fresnelR0, halfVec, ls.L);

    float3 specAlbedo = fresnelFactor * roughnessFactor;

    // Our spec formula goes outside [0,1] range, but we are 
    // doing LDR rendering.  So scale it down a bit.
    specAlbedo = specAlbedo / (specAlbedo + 1.0f);

    diffuse += mat.diffuseAlbedo.rgb * ls.diffuse * ls.NdotL;
    specular += specAlbedo * ls.specular * ls.NdotL;
}



//==============================
// PBR Help Functions
//==============================

float3 fresnelSchlick(float3 f0, float3 f90, float u)
{
    return f0 + (f90 - f0) * pow(1 - u, 5);
}

// Disney's diffuse term. 
// Based on https://disney-animation.s3.amazonaws.com/library/s2012_pbs_disney_brdf_notes_v2.pdf
float disneyDiffuseFresnel(float NdotV, float NdotL, float LdotH, float linearRoughness)
{
    float fd90 = 0.5 + 2 * LdotH * LdotH * linearRoughness;
    float fd0 = 1;
    float lightScatter = fresnelSchlick(fd0, fd90, NdotL).r;
    float viewScatter = fresnelSchlick(fd0, fd90, NdotV).r;
    return lightScatter * viewScatter;
}

float3 evalDiffuseDisneyBrdf(PbrParameter pp, LightSample ls)
{
    return disneyDiffuseFresnel(pp.NdotV, ls.NdotL, ls.LdotH, pp.linearRoughness) * M_INV_PI * pp.diffuse.rgb;
}

// Lambertian diffuse
float3 evalDiffuseLambertBrdf(PbrParameter pp, LightSample ls)
{
    return pp.diffuse.rgb * (1 / M_PI);
}

// Frostbites's diffuse term. 
// Based on https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
float3 evalDiffuseFrostbiteBrdf(PbrParameter sd, LightSample ls)
{
    float energyBias = lerp(0, 0.5, sd.linearRoughness);
    float energyFactor = lerp(1, 1.0 / 1.51, sd.linearRoughness);

    float fd90 = energyBias + 2 * ls.LdotH * ls.LdotH * sd.linearRoughness;
    float fd0 = 1;
    float lightScatter = fresnelSchlick(fd0, fd90, ls.NdotL).r;
    float viewScatter = fresnelSchlick(fd0, fd90, sd.NdotV).r;
    return (viewScatter * lightScatter * energyFactor * M_INV_PI) * sd.diffuse.rgb;
}

float3 evalDiffuseBrdf(PbrParameter pp, LightSample ls)
{
#ifdef DIFFUSE_LAMBERT_BRDF
    return evalDiffuseLambertBrdf(pp, ls);
#endif

#ifdef DIFFUSE_DISNEY_BRDF
    return evalDiffuseDisneyBrdf(pp, ls);
#endif

#ifdef DIFFUSE_FROSTBITE_BRDF
    return evalDiffuseFrostbiteBrdf(pp, ls);
#endif

    return evalDiffuseLambertBrdf(pp, ls);
}

float evalGGX(float roughness, float NdotH)
{
    float a2 = roughness * roughness;
    float d = ((NdotH * a2 - NdotH) * NdotH + 1);
    return a2 / (d * d);
}

float evalSmithGGX(float NdotL, float NdotV, float roughness)
{
    // Optimized version of Smith, already taking into account the division by (4 * NdotV)
    float a2 = roughness * roughness;
    // `NdotV *` and `NdotL *` are inversed. It's not a mistake.
    float ggxv = NdotL * sqrt((-NdotV * a2 + NdotV) * NdotV + a2);
    float ggxl = NdotV * sqrt((-NdotL * a2 + NdotL) * NdotL + a2);
    return 0.5f / (ggxv + ggxl);
}

float3 evalSpecularBrdf(PbrParameter sd, LightSample ls)
{
    float roughness = sd.roughness;
    
    float D = evalGGX(roughness, ls.NdotH);
    float G = evalSmithGGX(ls.NdotL, sd.NdotV, roughness);
    float3 F = fresnelSchlick(sd.specular, 1, max(0, ls.LdotH));
    return D * G * F * M_INV_PI;
}

PbrResult evalPBR(PbrParameter pp, LightSample ls)
{
    PbrResult pr = initPbrResult();

    // If the light doesn't hit the surface or we are viewing the surface from the back, return
    if (ls.NdotL <= 0)
        return pr;

    pp.NdotV = saturate(pp.NdotV);

    // Calculate the diffuse term
    pr.diffuseBrdf = evalDiffuseBrdf(pp, ls);
    pr.diffuse = ls.diffuse * pr.diffuseBrdf * ls.NdotL;

    // Calculate the specular term
    pr.specularBrdf = evalSpecularBrdf(pp, ls);
    pr.specular = ls.specular * pr.specularBrdf * ls.NdotL;

    return pr;
};