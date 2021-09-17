#include "../../../GraphicFramework/RaytracingUtility/ShaderUtility/RaytracingSceneRenderHelper.hlsl"

//
// Single-Ray Per Pixel
//

// Payload for our AO rays.
struct AORayPayload
{
	float aoValue;  // Store 0 if we hit a surface, 1 if we miss all surfaces
	uint RayID;
};

// A constant buffer we'll populate from our C++ code 
cbuffer RayGenCB : register(b0, space0)
{
	float g_AORadius;
	float g_MinT;
	int   g_PixelNumRays;
	int   g_AONumRays;
	int	  g_HaltonBase1;
	int   g_HaltonBase2;
	int   g_HaltonIndexStep;
	int   g_HaltonStartIndex;
};

RWTexture2D<float> g_Output : register(u0, space0);

float shootRayToScene(uint rayID, float3 origin, float3 direction)
{
	// Set the ray's extents.
	// Note: (1)Set TMin to a zero value to avoid aliasing artifacts along contact areas.
	//		 (2)Make sure to enable face culling so as to avoid surface face fighting.
	RayDesc rayDesc;
	rayDesc.Origin = origin;
	rayDesc.Direction = direction;
	rayDesc.TMin = 0.001f;
	rayDesc.TMax = 10000;
	AORayPayload rayPayload = { 1.0f, rayID };
	TraceRay(g_Scene,
		RAY_FLAG_CULL_BACK_FACING_TRIANGLES,
		0xFF, 0, g_hitProgCount, 0,
		rayDesc, rayPayload);

	return rayPayload.aoValue;
}

float generateCameraRay(uint2 index, float3 cameraPosition, float4x4 invViewProj)
{
	uint randSeed = initRand(index.x, index.y);

	float ao = 0.0f;
	for (uint i = 0; i < g_PixelNumRays; i++)
	{
		float x = index.x + nextRand(randSeed);
		float y = index.y + nextRand(randSeed);

		float2 screenPos = float2(x, y) / DispatchRaysDimensions().xy * 2.0f - 1.0f;

		// Invert Y for DirectX-style coordinates.
		screenPos.y = -screenPos.y;

		// Unproject the pixel coordinate into a world positon.
        float4 world = mul(invViewProj, float4(screenPos, 0.0f, 1.0f));
		world.xyz /= world.w;

		float3 rayOrigin = cameraPosition;
		float3 rayDirection = normalize(world.xyz - rayOrigin);
		ao += shootRayToScene(i, rayOrigin, rayDirection);
	}

	return ao / float(g_PixelNumRays);
}

// A wrapper function that encapsulates shooting an ambient occlusion ray query
float shootAmbientOcclusionRay(uint rayID, float3 orig, float3 dir, float minT, float maxT)
{
	// Setup ambient occlusion ray and payload
	RayDesc rayAO;
	rayAO.Origin = orig;					// Where does our ray start?
	rayAO.Direction = dir;					// What direction does our ray go?
	rayAO.TMin = minT;						// Min distance to detect an intersection
	rayAO.TMax = maxT;						// Max distance to detect an intersection
	AORayPayload rayPayload = { 0.0f, rayID };		// Specified value is returned if we hit a surface
	// Trace our ray.  Ray stops after it's first definite hit
	TraceRay(g_Scene,
		RAY_FLAG_CULL_BACK_FACING_TRIANGLES /*| RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH*/,
		0xFF, 1, g_hitProgCount, 0, rayAO, rayPayload);

	// Copy our AO value out of the ray payload.
	return rayPayload.aoValue;
}


[shader("raygeneration")]
void ScreenRayGen()
{
	float aoColor = generateCameraRay(DispatchRaysIndex().xy, g_cameraPosition.xyz, g_invViewProj);
	g_Output[DispatchRaysIndex().xy] = aoColor;
}


[shader("closesthit")]
void CameraRayClosestHit(inout AORayPayload rayPayload, in BuiltInTriangleIntersectionAttributes attr)
{
    // Get the position and normal
    const uint3 indices = hitTriangleIndices();
	float3 vertexPos[3] =
	{
        g_Vertices[indices[0]].position,
		g_Vertices[indices[1]].position,
		g_Vertices[indices[2]].position
    };
    float3 worldPos = mul(float4(hitAttribute(vertexPos, attr), 1.0f), g_InstanceWorlds[InstanceIndex()]).xyz;
	float3 vertexNormals[3] =
	{
        g_Vertices[indices[0]].normal,
		g_Vertices[indices[1]].normal,
		g_Vertices[indices[2]].normal
    };
    float3 worldNorm = mul(float4(hitAttribute(vertexNormals, attr), 0.0f), g_InstanceWorlds[InstanceIndex()]).xyz;
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
    if (a < 0.3f)
    {
        rayPayload.aoValue = 1.0f;
        return;
    }

	// Initialize a random seed, per-pixel, based on a screen position.
	uint2 launchIndex = DispatchRaysIndex().xy;
	uint2 launchDimensions = DispatchRaysDimensions().xy;

	float3 X, Y, Z;
	genBaseForNormal(worldNorm, X, Y, Z);

	// Start accumulating from zero if we don't hit the background
	float ambientOcclusion = 0.0f;
	for (int i = 0; i < g_AONumRays; i++)
	{
		// Sample cosine-weighted hemisphere around surface normal to pick a random ray direction
		float3 worldDir = getHaltonCosHemisphereSample(X, Y, Z, rayPayload.RayID*g_AONumRays + g_HaltonStartIndex + (launchIndex.x + launchIndex.y*launchDimensions.x)*g_HaltonIndexStep + i, g_HaltonBase1, g_HaltonBase2);

		// Shoot our ambient occlusion ray and update the value we'll output with the result
		ambientOcclusion += shootAmbientOcclusionRay(i, worldPos, worldDir, g_MinT, g_AORadius);
	}

	//Save out our AO color
	rayPayload.aoValue = ambientOcclusion / float(g_AONumRays);
}


[shader("closesthit")]
void AoClosestHit(inout AORayPayload rayData, in BuiltInTriangleIntersectionAttributes attr)
{
	rayData.aoValue = 0.0f;
}


// What code is executed when our ray misses all geometry?
[shader("miss")]
void Miss(inout AORayPayload rayData)
{
	// Our ambient occlusion value is 1 if we hit nothing.
	rayData.aoValue = 1.0f;
}


[shader("anyhit")]
void AnyHit(inout AORayPayload rayData, in BuiltInTriangleIntersectionAttributes attr)
{
    alphaClipInAnyHit(attr);
}