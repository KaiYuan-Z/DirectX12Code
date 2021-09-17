#include "../../RaytracingUtility/ShaderUtility/RaytracingSceneRenderHelper.hlsl"
#include "../../Core/ShaderUtility/VisibilityBufferUtil.hlsl"

struct AORayPayload
{
    float aoValue;
};


cbuffer RayGenCB : register(b0, space0)
{
    float4x4    g_preInvViewProj;
    float4      g_preCamPos;
  
    float       g_AoRadius;
    float       g_MinT;
    float       g_AoDiffScale;
    uint        g_OpenAoDiffTest;
    
    uint        g_UseDynamicMark;
    float3      pad;
};


Texture2D<uint>         g_preHaltonIndex    : register(t0, space0);
Texture2D<float2>        g_preSampleCache    : register(t1, space0);
Texture2D<int4>         g_preIds            : register(t2, space0);
Texture2D<float2>       g_motionVec         : register(t3, space0);
Texture2D<int4>         g_curAccumMap       : register(t4, space0); // AccumCount(int), NumSampleCount(int), DynamicInstanceHint(int), DynamicMarkLife(int)

RWTexture2D<float>      g_aoDiffOutput      : register(u0, space0);
//RWByteAddressBuffer     g_aoRayCounter      : register(u1, space0);


float shootAmbientOcclusionRay(float3 orig, float3 dir, float minT, float maxT)
{
    AORayPayload rayPayload;
    rayPayload.aoValue = 1.0f;
    
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

    uint2 testIndex = launchIndex;
    float2 testTex = index2Texcoord(launchDim, testIndex);
    float2 preTestTex = testTex + g_motionVec[testIndex];
    uint2 preTestIndex = texcoord2Index(launchDim, preTestTex);
            
    uint preHaltonIndex = g_preHaltonIndex[preTestIndex];
    float2 preSampleCache = g_preSampleCache[preTestIndex];
    float preSampleValue = preSampleCache.x;
    float aoRadius = preSampleCache.y;
    
    int4 preIds = g_preIds[preTestIndex];
    int preInstanceId = preIds.x;
    int preMeshId = preIds.y & 0xffff;
    int prePrimitiveId = preIds.z;
    int preInstanceTag = preIds.w;
    
    int4 curAccum = g_curAccumMap[testIndex];
   
    int dynamicInstanceHint = curAccum.z;
    int dynamicAreaMark = curAccum.w;
    
    if (g_UseDynamicMark == 0)
        dynamicAreaMark = DYNAMIC_INSTANCE_MARK_1;
    
    float aoTestValue = -1.0f;
    float aoDiffValue = 0.0f;
    if (dynamicAreaMark > 0 && preInstanceId >= 0 && g_OpenAoDiffTest > 0)
    {
        const uint3 indices = getTriangleIndices(preInstanceId, preMeshId, prePrimitiveId);
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
        
        float3 preVertexPos[3] = { vertexPos[0], vertexPos[1], vertexPos[2] };      
        if ((preInstanceTag & GRASS_INST_TAG) > 0)
        {
            preVertexPos[0] = g_Vertices[indices[0]].tangent;
            preVertexPos[1] = g_Vertices[indices[1]].tangent;
            preVertexPos[2] = g_Vertices[indices[2]].tangent;
        }
       
        float4x4 instancePreWorldMat = g_InstanceWorlds[g_instanceCount + preInstanceId];
        float3 b = getBarycentricCoordinates(g_preInvViewProj, instancePreWorldMat, launchDim, preTestIndex, g_preCamPos.xyz, preVertexPos[0], preVertexPos[1], preVertexPos[2]);
        float3 localPos = interpolationAttribute(vertexPos, b);
        float3 localNorm = interpolationAttribute(vertexNormals, b);       
        float3 localBitan = getPerpendicularVector(localNorm);
        
        float4x4 instanceWorldMat = g_InstanceWorlds[preInstanceId];
        float3 worldPos = mul(instanceWorldMat, float4(localPos, 1.0f)).xyz;
        float3 worldNorm = mul(instanceWorldMat, float4(localNorm, 0.0f)).xyz;
        float3 worldBitan = mul(instanceWorldMat, float4(localBitan, 0.0f)).xyz;
        worldBitan = worldBitan - worldNorm * (dot(worldNorm, worldBitan));
        
        float3 X, Y, Z;
        genBaseForNormal(worldNorm, worldBitan, X, Y, Z);
        //genBaseForNormal(worldNorm, X, Y, Z);
                 
        float2 u = halton23(preTestIndex.x, preTestIndex.y, preHaltonIndex);
        float3 worldDir = getCosHemisphereSample(X, Y, Z, u.x, u.y);
        
        float aoValue = shootAmbientOcclusionRay(worldPos.xyz, worldDir, g_MinT, aoRadius);
        
        aoTestValue = aoValue;
        aoDiffValue = (preSampleValue != aoValue);
        
        uint outValue;
        //g_aoRayCounter.InterlockedAdd(0, 1, outValue);
    }
    
    g_aoDiffOutput[launchIndex] = aoDiffValue * g_AoDiffScale;
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