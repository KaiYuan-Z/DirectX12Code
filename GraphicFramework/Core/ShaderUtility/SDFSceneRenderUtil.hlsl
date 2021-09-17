#include "LightingUtil.hlsl"
#include "RenderCommonUtil.hlsl"

#define MAX_MESH_SDF_COUNT 20
#define INVALID_SDF_INDEX 0xffff
#define INDEX_OFFSET 1

struct VSInput
{
    float3 position : POSITION;
    float2 texcoord0 : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosV : POSITION;
    float2 TexC : TEXCOORD0;
};

struct SSubSurfaceScatteringParam
{
	float SSSMaxDist;
	float SSSDistortion;
	float SSSPower;
	float SSSScale;
};

struct SSDFParam
{
	uint3		SDFSize;
	float		Scale;
	float3		UVOffset;
	uint		SDFIndex;
	float3		LocalObjectExtent;
	float		Push;
	float		SurfaceThreshold;
	float		SoftShadowFactor;
    float2      Padding;
};

struct SSDFModelData
{
	SSubSurfaceScatteringParam		SSSParam;
	SSDFParam						SDFParam;
    bool                            IsUsingSSS;
};

cbuffer cb_PerFrame : register(b0, space1)
{
    float4 g_CameraPosW;
    float4x4 g_View;
    float4x4 g_Proj;
    float4x4 g_InvProj;
    float4 g_AmbientLight;

};
    
cbuffer cb_Scene : register(b1, space1)
{
    uint g_SDFLightCount;
    uint g_SSSLightIndex;
    uint g_SDFInstanceNum;
};

Texture2D<float4>   g_PostionMap : register(t0, space1);
Texture2D<float4>   g_NormWMap : register(t1, space1);
Texture2D<uint4>    g_ModelIDAndInstanceIDMap : register(t2, space1);

StructuredBuffer<LightData>     g_LightData : register(t3, space1);
StructuredBuffer<SSDFModelData> g_ModelDataBuffer : register(t4, space1);
StructuredBuffer<SInstanceData> g_InstanceDataBuffer : register(t5, space1);

Texture3D<float> g_SDFBuffer[MAX_MESH_SDF_COUNT] : register(t6, space1);

SamplerState g_samLinear : register(s0, space1);

uint3 GetModelIDAndInstanceIDAndValidPixel(float2 PixelTec)
{
    uint width, height;
    g_ModelIDAndInstanceIDMap.GetDimensions(width, height);
    uint2 PixelUV = PixelTec * uint2(width, height);
    uint2 PixelModelIDAndInstanceID = g_ModelIDAndInstanceIDMap[PixelUV].rg;
    return uint3(PixelModelIDAndInstanceID.r - INDEX_OFFSET, PixelModelIDAndInstanceID.g - INDEX_OFFSET, 1 - PixelModelIDAndInstanceID.r);
}

bool IsSDFModel(uint ModelID)
{
    return g_ModelDataBuffer[ModelID].SDFParam.SDFIndex != INVALID_SDF_INDEX;
}

//Calculate line intersect boundingbox
float2 LineBoxIntersect(float3 RayOrigin, float3 RayEnd, float3 BoxMin, float3 BoxMax)
{
	float3 InvRayDir = 1.0f / (RayEnd - RayOrigin);

	float3 FirstPlaneIntersections = (BoxMin - RayOrigin) * InvRayDir;
	float3 SecondPlaneIntersections = (BoxMax - RayOrigin) * InvRayDir;
	float3 ClosestPlaneIntersections = min(FirstPlaneIntersections, SecondPlaneIntersections);
	float3 FurthestPlaneIntersections = max(FirstPlaneIntersections, SecondPlaneIntersections);
	float2 BoxIntersections;

	BoxIntersections.x = max(ClosestPlaneIntersections.x, max(ClosestPlaneIntersections.y, ClosestPlaneIntersections.z));
	BoxIntersections.y = min(FurthestPlaneIntersections.x, min(FurthestPlaneIntersections.y, FurthestPlaneIntersections.z));

	return saturate(BoxIntersections);
}

bool CompareVectorGreater(float3 a, float3 b)
{
	return (a.x >= b.x) & (a.y >= b.y) & (a.z >= b.z);
}

bool CompareVectorLower(float3 a, float3 b)
{
	return (a.x <= b.x) & (a.y <= b.y) & (a.z <= b.z);
}

float ComputeSoftShadow(in Texture3D<float> SDFBuffer, in SamplerState SamLinear, float3 StartSamplePos, float3 RayDirection, float SampleRayLength,
						float3 SDFUVScale, float3 SDFUVAdd, float3 LocalObjectExtent,
						uint MaxStep, float Threshold, float Push, float SoftShadowFactor, float MaxDist, bool bUseSubsurfaceTransmission)
{
	float Radius = Push;
	SampleRayLength += Push;
	float SubsurfaceDensity = -.05f * log(0.5);//0-1
	float ph = 1e10; // big, such that y = 0 on the first iteration
	float Result = 1;
	[unroll]
	for (uint StepIndex = 0; StepIndex < MaxStep; StepIndex++)
	{
		float3 SamplePos = StartSamplePos + RayDirection * SampleRayLength;
		float3 ClampedPos = clamp(SamplePos, -LocalObjectExtent, LocalObjectExtent);

		float ClampLength = length(ClampedPos - SamplePos);
		float3 SamplePosUV = ClampedPos / SDFUVScale + SDFUVAdd;
		float SDFValue = SDFBuffer.SampleLevel(SamLinear, SamplePosUV, 0).x;
		float dist = abs(SDFValue) + ClampLength;

		////-----------------------------Optimize Soft Shadow--------------------------------
		////---------------------------------------------------------------------------------
		//float y = dist * dist / (2.0*ph);
		//float d = sqrt(dist*dist - y * y);
		//float StepVisibility = SoftShadowFactor * d / max(0.0, Radius - y);
		//ph = dist;
		//Radius += dist;
		////---------------------------------------------------------------------------------

		//----------------------------Traditional Soft Shadow------------------------------
		//---------------------------------------------------------------------------------
		float StepVisibility = SoftShadowFactor * dist / Radius;
		Radius += dist;
		//---------------------------------------------------------------------------------

		if (SDFValue < Threshold)
		{
			StepVisibility = 0;
		}

		if (bUseSubsurfaceTransmission)
		{
			float SubsurfaceVisibility = saturate(exp(-SampleRayLength * SubsurfaceDensity));
			StepVisibility = max(SubsurfaceVisibility, StepVisibility);
		}

		SampleRayLength += dist;

		Result = min(StepVisibility, Result);

		if (SampleRayLength > MaxDist /*|| SampleRayLength > VolumeRayLength*/)
			break;
	}
	return Result;
}

float AOConeTrace(in Texture3D<float> SDFBuffer, in SamplerState SamLinear, float3 StartSamplePos, float3 AODirection, float SampleRayLength,
				  float3 SDFUVScale, float3 SDFUVAdd, float3 LocalObjectExtent,
				  uint MaxStep, float Threshold, float Push, float SoftShadowFactor, float MaxDist)
{
	float Radius = Push;
	SampleRayLength += Push;

	float Result = 1;
	[unroll]
	for (uint StepIndex = 0; StepIndex < MaxStep; StepIndex++)
	{
		float3 SamplePos = StartSamplePos + AODirection * SampleRayLength;
		float3 ClampedPos = clamp(SamplePos, -LocalObjectExtent, LocalObjectExtent);

		float ClampLength = length(ClampedPos - SamplePos);
		float3 SamplePosUV = ClampedPos / SDFUVScale + SDFUVAdd;
		float SDFValue = abs(SDFBuffer.SampleLevel(SamLinear, SamplePosUV, 0).x);
		float Dist = SDFValue + ClampLength;

		Result = min(Result, (SoftShadowFactor * Dist / Radius));
		Radius += Dist;

		SampleRayLength += Dist;

		if (SDFValue < Threshold || SampleRayLength > MaxDist)
		{
			break;
		}
	}
	return saturate(Result);
}

float AmbientOcclude(in Texture3D<float> SDFBuffer, in SamplerState SamLinear, float3 StartSamplePos, float SampleRayLength, 
					float3 SDFUVScale, float3 SDFUVAdd, float3 LocalObjectExtent,
					float3 Normal, float MaxStep, float Threshold, float Push, float MaxDist)
{
	const int ConeNums = 9;
	float3 Dirs[ConeNums];
	Dirs[0] = Normal;
	float3 up = float3(0, -1, 0);
	if (abs(dot(Normal, up)) >= 0.9)
		up = float3(1, 0, 0);
	float3 right = normalize(cross(Normal, up));
	float3 front = normalize(cross(right, Normal));
	float3 fr = (front + right)*0.5;
	float3 fl = (front - right)*0.5;
	float3 br = (right - front)*0.5;
	float3 bl = (-right - front)*0.5;
	Dirs[1] = (Normal + right)*0.5;
	Dirs[2] = (Normal - right)*0.5;
	Dirs[3] = (Normal + front)*0.5;
	Dirs[4] = (Normal - front)*0.5;
	Dirs[5] = (Normal + fr)*0.5;
	Dirs[6] = (Normal + fl)*0.5;
	Dirs[7] = (Normal + br)*0.5;
	Dirs[8] = (Normal + bl)*0.5;

	float minAO = 1.0;
	for (int i = 0; i < ConeNums; ++i)
	{
		minAO += AOConeTrace(SDFBuffer, SamLinear, StartSamplePos, Dirs[i], SampleRayLength,
							 SDFUVScale, SDFUVAdd, LocalObjectExtent,
							 MaxStep, Threshold, Push, 10, MaxDist);
	}
	minAO /= ConeNums;
	return saturate(minAO);
}

float ThicknessTrace(in Texture3D<float> SDFBuffer, in SamplerState SamLinear, float3 StartSamplePos, float3 AODirection,
					 float3 SDFUVScale, float3 SDFUVAdd, float3 LocalObjectExtent,
					 uint MaxStep, float StepSize, float MaxThicknessTraceDist)
{
	float Thickness = 0;
	float SampleRayLength = 3;
	[unroll]
	for (uint StepIndex = 0; StepIndex < MaxStep; StepIndex++)
	{
		float3 SamplePos = StartSamplePos + AODirection * SampleRayLength;
		float3 ClampedPos = clamp(SamplePos, -LocalObjectExtent, LocalObjectExtent);

		float3 SamplePosUV = ClampedPos / SDFUVScale + SDFUVAdd;
		float SDFValue = SDFBuffer.SampleLevel(SamLinear, SamplePosUV, 0).x;

		if (SDFValue < 0)
		{
			Thickness += min(abs(SDFValue),StepSize);
		}

		if (Thickness > MaxThicknessTraceDist)
		{
			break;
		}

		SampleRayLength += StepSize;
	}
	return Thickness = 1 - clamp(Thickness, 0.f, MaxThicknessTraceDist) / MaxThicknessTraceDist;
}