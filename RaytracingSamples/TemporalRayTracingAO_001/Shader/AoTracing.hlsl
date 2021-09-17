#include "HlslUtils.hlsli"


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
};

cbuffer SceneCB : register(b1, space0)
{
	float4x4 g_ViewProj;
	float4x4 g_InvViewProj;
	float4 g_CameraPosition;
	uint  g_FrameCount;
};

cbuffer Material : register(b2)
{
	uint l_MeshID;
};


RaytracingAccelerationStructure			g_Scene : register(t0, space0);
ByteAddressBuffer						g_Indices : register(t1, space0);
StructuredBuffer<SPosNormTexTanBitan>	g_Vertices : register(t2, space0);
StructuredBuffer<SModelMeshInfo>		g_MeshInfo : register(t3, space0);
RWTexture2D<float>						g_Output : register(u0, space0);

SamplerState g_samLinear  : register(s0);


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
		0xFF, 1, 1, 0,
		rayDesc, rayPayload);

	return rayPayload.aoValue;
}

float generateCameraRay(uint2 index, float3 cameraPosition, float4x4 invViewProj)
{
	uint randSeed = initRand(index.x + index.y*DispatchRaysDimensions().x, g_FrameCount);

	float ao = 0.0f;
	for (uint i = 0; i < g_PixelNumRays; i++)
	{
		float x = index.x + nextRand(randSeed);// NextRand(randSeed) range is 0.0f~1.0f.
		float y = index.y + nextRand(randSeed);

		float2 screenPos = float2(x, y) / DispatchRaysDimensions().xy * 2.0f - 1.0f;

		// Invert Y for DirectX-style coordinates.
		screenPos.y = -screenPos.y;

		// Unproject the pixel coordinate into a world positon.
		float4 world = mul(float4(screenPos, 0.0f, 1.0f), invViewProj);
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
		RAY_FLAG_CULL_BACK_FACING_TRIANGLES | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH,
		0xFF, 0, 0, 0, rayAO, rayPayload);

	// Copy our AO value out of the ray payload.
	return rayPayload.aoValue;
}


[shader("raygeneration")]
void ScreenRayGen()
{
	float aoColor = generateCameraRay(DispatchRaysIndex().xy, g_CameraPosition.xyz, g_InvViewProj);
	g_Output[DispatchRaysIndex().xy] = aoColor;
}


[shader("closesthit")]
void CameraRayClosestHit(inout AORayPayload rayPayload, in BuiltInTriangleIntersectionAttributes attr)
{
	// Get the position and normal
	const uint3 indices = load3x32BitIndices(g_Indices, g_MeshInfo[l_MeshID].IndexOffsetBytes, PrimitiveIndex());
	uint vertexFirstIndex = g_MeshInfo[l_MeshID].VertexOffsetBytes / g_MeshInfo[l_MeshID].VertexStride;
	float3 vertexPos[3] =
	{
		g_Vertices[indices[0] + vertexFirstIndex].Position,
		g_Vertices[indices[1] + vertexFirstIndex].Position,
		g_Vertices[indices[2] + vertexFirstIndex].Position
	};
	float3 worldPos = hitAttribute(vertexPos, attr);
	float3 vertexNormals[3] = 
	{
		g_Vertices[indices[0] + vertexFirstIndex].Normal,
		g_Vertices[indices[1] + vertexFirstIndex].Normal,
		g_Vertices[indices[2] + vertexFirstIndex].Normal
	};
	float3 worldNorm = hitAttribute(vertexNormals, attr);

	float3 X, Y, Z;
	genBaseForNormal(worldNorm, X, Y, Z);

	// Initialize a random seed, per-pixel, based on a screen position.
	uint2 launchIndex = DispatchRaysIndex().xy;
	uint2 launchDimensions = DispatchRaysDimensions().xy;
	uint rayStep = launchDimensions.x*launchDimensions.y;
	uint randSeed = initRand((launchIndex.x + launchIndex.y*launchDimensions.x) + rayStep*rayPayload.RayID, g_FrameCount);
	// Start accumulating from zero if we don't hit the background
	float ambientOcclusion = 0.0f;
    float weightSum = 0.0f;
	for (int i = 0; i < g_AONumRays; i++)
	{
		// Sample cosine-weighted hemisphere around surface normal to pick a random ray direction
		float3 worldDir = getNextHemisphereSample(X, Y, Z, randSeed);

        float weight = dot(normalize(worldDir), normalize(worldNorm.xyz)) / M_PI;
        
		// Shoot our ambient occlusion ray and update the value we'll output with the result
        ambientOcclusion += shootAmbientOcclusionRay(i, worldPos, worldDir, g_MinT, g_AORadius) * weight;
        weightSum += weight;
    }

	//Save out our AO color
    rayPayload.aoValue = ambientOcclusion / float(weightSum);
	//rayPayload.aoValue = float4(normalize(worldNorm), 0.0f);//hitWorldPosition()
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