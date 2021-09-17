#include "../../../GraphicFramework/RaytracingUtility/ShaderUtility/RaytracingSceneRenderHelper.hlsl"
#include "../../../GraphicFramework/Core/ShaderUtility/VisibilityBufferUtil.hlsl"


struct AORayPayload
{
    float aoValue; 
};

cbuffer RayGenCB : register(b0, space0)
{
    float g_AORadius;
    float g_MinT;
    float Pad1;
    float Pad2;
    
    uint  g_HighSampleNum;
    uint  g_MediumSampleNum;
    uint  g_LowSampleNum;
    uint  g_MaxAccumCount;
};


Texture2D<int4>         g_CurIds                : register(t0, space0);
Texture2D<float>        g_AoDiff                : register(t1, space0);

RWTexture2D<float>      g_OutAo                 : register(u0, space0);
RWTexture2D<float2>     g_OutPreSampleCache     : register(u1, space0);

RWTexture2D<uint>       g_InOutHaltonIndex      : register(u2, space0);
RWTexture2D<int4>       g_InOutAccumMap         : register(u3, space0);


uint calcSampleCount(uint curAccumCount)
{
    if (curAccumCount < 32)
    {
        return g_HighSampleNum;
    }
    
    if (curAccumCount < 64)
    {
        return g_MediumSampleNum;
    }
        
    return g_LowSampleNum;
}


float shootAmbientOcclusionRay(float3 orig, float3 dir, float minT, float maxT)
{
    AORayPayload rayPayload = { 1.0f };
    
    RayDesc rayAO;
    rayAO.Origin = orig;
    rayAO.Direction = dir;
    rayAO.TMin = minT;
    rayAO.TMax = maxT;

    TraceRay(g_Scene,
		0,
		0xFF, 0, 0, 0, rayAO, rayPayload);

    return rayPayload.aoValue;
}


[shader("raygeneration")]
void AoRayGen()
{
    uint2 launchIndex = DispatchRaysIndex().xy;
    uint2 launchDim = DispatchRaysDimensions().xy;
    
    int4 ids = g_CurIds[launchIndex];
    int instanceId = ids.x;
    int meshId = ids.y & 0xffff;
    int primitiveId = ids.z;

    int4 accumMap = g_InOutAccumMap[launchIndex];
    uint accumCount = accumMap.x;
    uint dynamicAreaMark = accumMap.w;
    
    uint aoRayNum = calcSampleCount(accumCount);
 
    uint haltonIndex = g_InOutHaltonIndex[launchIndex];
    float aoRayResult = 0.0f;
    float ambientOcclusion = float(aoRayNum);
    
    float aoRadius = dynamicAreaMark == DYNAMIC_INSTANCE_MARK_2 ? 0.2f : 0.35f;
    
    if (instanceId >= 0)
    {
        const uint3 indices = getTriangleIndices(instanceId, meshId, primitiveId);
        float3 vertexPos[3] =
        {
            g_Vertices[indices[0]].position,
		    g_Vertices[indices[1]].position,
		    g_Vertices[indices[2]].position
        };
        
        float3 vertexNormals[3] =
        {
            g_Vertices[indices[0]].normal,
		    g_Vertices[indices[1]].normal,
		    g_Vertices[indices[2]].normal
        };
         
        float4x4 instanceWorldMat = g_InstanceWorlds[instanceId];
        float3 b = getBarycentricCoordinates(g_invViewProj, instanceWorldMat, launchDim, launchIndex, g_cameraPosition, vertexPos[0], vertexPos[1], vertexPos[2]);
        float3 localPos = interpolationAttribute(vertexPos, b);
        float3 localNorm = interpolationAttribute(vertexNormals, b);
        float3 localBitan = getPerpendicularVector(localNorm);
        
        float3 worldPos = mul(instanceWorldMat, float4(localPos, 1.0f)).xyz;
        float3 worldNorm = mul(instanceWorldMat, float4(localNorm, 0.0f)).xyz;
        float3 worldBitan = mul(instanceWorldMat, float4(localBitan, 0.0f)).xyz;
        worldBitan = worldBitan - worldNorm * (dot(worldNorm, worldBitan));
            
        float3 X, Y, Z;
        genBaseForNormal(worldNorm, worldBitan, X, Y, Z);
        
        ambientOcclusion = 0.0f;
        
        for (int i = 0; i < aoRayNum; i++)
        {
            haltonIndex++;
            haltonIndex %= g_max_samples;
			
            float2 u = halton23(launchIndex.x, launchIndex.y, haltonIndex);
            float3 worldDir = getCosHemisphereSample(X, Y, Z, u.x, u.y);

            aoRayResult = shootAmbientOcclusionRay(worldPos.xyz, worldDir, g_MinT, aoRadius);
            ambientOcclusion += aoRayResult;
        }
    }
	
    ambientOcclusion /= aoRayNum;  
    accumMap.y = aoRayNum;
    
    g_OutAo[launchIndex] = ambientOcclusion;
    g_OutPreSampleCache[launchIndex] = float2(aoRayResult, aoRadius);
    g_InOutHaltonIndex[launchIndex] = haltonIndex;
    g_InOutAccumMap[launchIndex] = accumMap;
}


[shader("closesthit")]
void AoClosestHit(inout AORayPayload rayData, in BuiltInTriangleIntersectionAttributes attr)
{
    rayData.aoValue = 0.0f;
}


[shader("miss")]
void AoMiss(inout AORayPayload rayData)
{
    rayData.aoValue = 1.0f;
}

[shader("anyhit")]
void AoAnyHit(inout AORayPayload rayData, in BuiltInTriangleIntersectionAttributes attr)
{
    alphaClipInAnyHit(attr);
}