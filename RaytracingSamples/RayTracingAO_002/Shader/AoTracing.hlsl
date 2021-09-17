#include "../../../GraphicFramework/RaytracingUtility/ShaderUtility/RaytracingSceneRenderHelper.hlsl"
#include "RandomNumberGenerator.hlsli"

#define RNG_SEED 1879

struct AORayPayload
{
	float aoValue;  // Store 0 if we hit a surface, 1 if we miss all surfaces
};

cbuffer RayGenCB : register(b0, space0)
{
	float g_AORadius;
	float g_MinT;
	uint  g_NumRays;
    bool  g_OpenMultiJitter;
    
    uint  g_numSampleSets;
    uint  g_numSamplesPerSet;
    uint  g_numPixelsPerDimPerSet;
    uint  pad1;
};

Texture2D<float4>                   g_Pos           : register(t0, space0);
Texture2D<float2>                   g_Norm          : register(t1, space0);
StructuredBuffer<float4>            g_sampleSets    : register(t2, space0);
RWTexture2D<float>                  g_Output        : register(u0, space0);

float hash(float3 p)
{
    p = frac(p * 0.3183099 + .1);
    p *= 17.0;
    return frac(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float3 getRandomRayDirection(in uint2 srcRayIndex, in float3 surfaceNormal, in uint2 textureDim, in uint raySampleIndexOffset)
{
    // Calculate coordinate system for the hemisphere.
    float3 u, v, w;
    w = surfaceNormal;

    // Get a vector that's not parallel to w.
    float3 right = 0.3f * w + float3(-0.72f, 0.56f, -0.34f);
    v = normalize(cross(w, right));
    u = cross(v, w);

    // Calculate offsets to the pregenerated sample set.
    uint sampleSetJump; // Offset to the start of the sample set
    uint sampleJump; // Offset to the first sample for this pixel within a sample set.
    {
        // Neighboring samples NxN share a sample set, but use different samples within a set.
        // Sharing a sample set lets the pixels in the group get a better coverage of the hemisphere 
        // than if each pixel used a separate sample set with less samples pregenerated per set.

        // Get a common sample set ID and seed shared across neighboring pixels.
        uint numSampleSetsInX = (textureDim.x + g_numPixelsPerDimPerSet - 1) / g_numPixelsPerDimPerSet;
        uint2 sampleSetId = srcRayIndex / g_numPixelsPerDimPerSet;

        // Get a common hitPosition to adjust the sampleSeed by. 
        // This breaks noise correlation on camera movement which otherwise results 
        // in noise pattern swimming across the screen on camera movement.
        uint2 pixelZeroId = sampleSetId * g_numPixelsPerDimPerSet;
        float3 pixelZeroHitPosition = g_Pos[pixelZeroId].xyz;
        uint sampleSetSeed = (sampleSetId.y * numSampleSetsInX + sampleSetId.x) * hash(pixelZeroHitPosition) + RNG_SEED;
        uint RNGState = RNG::SeedThread(sampleSetSeed);

        sampleSetJump = RNG::Random(RNGState, 0, g_numSampleSets - 1) * g_numSamplesPerSet;

        // Get a pixel ID within the shared set across neighboring pixels.
        uint2 pixeIDPerSet2D = srcRayIndex % g_numPixelsPerDimPerSet;
        uint pixeIDPerSet = pixeIDPerSet2D.y * g_numPixelsPerDimPerSet + pixeIDPerSet2D.x;

        // Randomize starting sample position within a sample set per neighbor group 
        // to break group to group correlation resulting in square alias.
        uint numPixelsPerSet = g_numPixelsPerDimPerSet * g_numPixelsPerDimPerSet;
        sampleJump = pixeIDPerSet + RNG::Random(RNGState, 0, numPixelsPerSet - 1) + raySampleIndexOffset;
    }

    // Load a pregenerated random sample from the sample set.
    float3 sample = g_sampleSets[sampleSetJump + (sampleJump % g_numSamplesPerSet)].xyz;
    float3 rayDirection = normalize(sample.x * u + sample.y * v + sample.z * w);

    return rayDirection;
}


// A wrapper function that encapsulates shooting an ambient occlusion ray query
float shootAmbientOcclusionRay( float3 orig, float3 dir, float minT, float maxT )
{
	// Setup ambient occlusion ray and payload
	AORayPayload  rayPayload = { 1.0f };  // Specified value is returned if we hit a surface
	RayDesc       rayAO;
	rayAO.Origin    = orig;               // Where does our ray start?
	rayAO.Direction = dir;                // What direction does our ray go?
	rayAO.TMin      = minT;               // Min distance to detect an intersection
	rayAO.TMax      = maxT;               // Max distance to detect an intersection

	// Trace our ray.  Ray stops after it's first definite hit
	TraceRay(g_Scene,
		0,
		0xFF, 0, g_hitProgCount, 0, rayAO, rayPayload);

	// Copy our AO value out of the ray payload.
	return rayPayload.aoValue;
}


// How do we generate the rays that we trace?
[shader("raygeneration")]
void AoRayGen()
{
	// Where is this thread's ray on screen?
	uint2 launchIndex = DispatchRaysIndex().xy;
	uint2 launchDim   = DispatchRaysDimensions().xy;

	// Load the position and normal from our g-buffer
	float4 worldPos = g_Pos[launchIndex];
    float3 worldNorm = decodeNormal(g_Norm[launchIndex]);

	float3 X, Y, Z;
	genBaseForNormal(worldNorm.xyz, X, Y, Z);
    
    float ambientOcclusion = 0.0f;
    if (g_OpenMultiJitter)
    {
        for (int i = 0; i < g_NumRays; i++)
        {
            float3 worldDir = getRandomRayDirection(launchIndex, worldNorm, launchDim, i);
            ambientOcclusion += shootAmbientOcclusionRay(worldPos.xyz, worldDir, g_MinT, g_AORadius);
        }
    }
    else
    {
        for (int i = 0; i < g_NumRays; i++)
        {
            float2 u = halton23(launchIndex.x, launchIndex.y, i);
            float3 worldDir = getCosHemisphereSample(X, Y, Z, u.x, u.y);
            ambientOcclusion += shootAmbientOcclusionRay(worldPos.xyz, worldDir, g_MinT, g_AORadius);
        }
    }
    
    g_Output[launchIndex] = ambientOcclusion / g_NumRays;
}


[shader("closesthit")]
void AoClosestHit(inout AORayPayload rayData, in BuiltInTriangleIntersectionAttributes attr)
{
	rayData.aoValue = 0.0f;
}


// What code is executed when our ray misses all geometry?
[shader("miss")]
void AoMiss(inout AORayPayload rayData)
{
	// Our ambient occlusion value is 1 if we hit nothing.
	rayData.aoValue = 1.0f;
}

[shader("anyhit")]
void AoAnyHit(inout AORayPayload rayData, in BuiltInTriangleIntersectionAttributes attr)
{
    alphaClipInAnyHit(attr);
}


