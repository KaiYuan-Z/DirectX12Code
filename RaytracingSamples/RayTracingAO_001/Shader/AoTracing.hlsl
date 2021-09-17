#include "../../../GraphicFramework/RaytracingUtility/ShaderUtility/RaytracingSceneRenderHelper.hlsl"

//
// Single-Ray Per Pixel
//

// Payload for our AO rays.
struct AORayPayload
{
	float aoValue;  // Store 0 if we hit a surface, 1 if we miss all surfaces
};


// A constant buffer we'll populate from our C++ code 
cbuffer RayGenCB : register(b0, space0)
{
	float g_AORadius;
	float g_MinT;
	uint  g_NumRays;
    bool  g_OpenCosineSample;
};

Texture2D<float4> g_Pos : register(t0, space0);
Texture2D<float2> g_Norm : register(t1, space0);
RWTexture2D<float> g_Output : register(u0, space0);


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
    uint2 launchDim = DispatchRaysDimensions().xy;

	// Load the position and normal from our g-buffer
    float4 worldPos = g_Pos[launchIndex];
    float3 worldNorm = decodeNormal(g_Norm[launchIndex]);

    float3 X, Y, Z;
    genBaseForNormal(worldNorm.xyz, X, Y, Z);   
    
    if (g_OpenCosineSample)
    {
        float ambientOcclusion = 0.0f;
        for (int i = 0; i < g_NumRays; i++)
        {
            float2 u = halton23(launchIndex.x, launchIndex.y, i);
            float3 worldDir = getCosHemisphereSample(X, Y, Z, u.x, u.y);
            ambientOcclusion += shootAmbientOcclusionRay(worldPos.xyz, worldDir, g_MinT, g_AORadius);
        }
        g_Output[launchIndex] = ambientOcclusion / g_NumRays;
    }
    else
    {
        float ambientOcclusion = 0;
        float weightSum = 0;
        if (worldPos.w != 0.0f)
        {
            ambientOcclusion = 0.0f;
            for (int i = 0; i < g_NumRays; i++)
            {
                float2 u = halton23(launchIndex.x, launchIndex.y, i);
                float3 worldDir = getHemisphereSample(X, Y, Z, u.x, u.y);
                float weight = dot(normalize(worldDir), normalize(worldNorm.xyz)) / M_PI;
                ambientOcclusion += shootAmbientOcclusionRay(worldPos.xyz, worldDir, g_MinT, g_AORadius) * weight;
            }
        }
        g_Output[launchIndex] = ambientOcclusion / g_NumRays * 2 * M_PI; // pdf:1/(2*M_PI)

    }
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

