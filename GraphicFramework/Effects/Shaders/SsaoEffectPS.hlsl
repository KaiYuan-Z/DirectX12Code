#include "../../Core/ShaderUtility/MathUtil.hlsl"

struct VertexOut
{
	float4 PosH : SV_POSITION;
	float3 PosV : POSITION;
	float2 TexC : TEXCOORD0;
};

cbuffer cbPerframe : register(b0)
{
	float4x4 g_View;
	float4x4 g_Proj;
	float4x4 g_InvProj;
	float4x4 g_ProjTex;
};

cbuffer cbSsao : register(b1)
{
	// Coordinates given in view space.
	float g_OcclusionRadius;
	float g_OcclusionFadeStart;
	float g_OcclusionFadeEnd;
	float g_SurfaceEpsilon;
	int  g_SampleCount;
	int  g_FrameCount;
	int  g_SsaoPower;
	bool g_UseTemporalRandom;
}

Texture2D<float4> g_NormalMap : register(t0);
Texture2D<float> g_DepthMap : register(t1);

SamplerState g_samPointClamp : register(s0);
SamplerState g_samDepth : register(s1);

float occlusionFunction(float distZ)
{
	float occlusion = 0.0f;
	if (distZ > g_SurfaceEpsilon)
	{
		float fadeLength = g_OcclusionFadeEnd - g_OcclusionFadeStart;

		occlusion = saturate((g_OcclusionFadeEnd - distZ) / fadeLength);
	}

	return occlusion;
}

float4 main(VertexOut pin) : SV_Target
{
	// Get viewspace normal and z-coord of this pixel.
	float3 n = mul(float4(g_NormalMap.Sample(g_samPointClamp, pin.TexC).xyz, 0.0f), g_View).xyz;
	float pz = g_DepthMap.Sample(g_samDepth, pin.TexC).x;
	pz = ndcDepth2ViewDepthRM(g_Proj, pz);

	float3 p = (pz / pin.PosV.z)*pin.PosV;

	float occlusionSum = 0.0f;

	uint width, height, numMips;
	g_NormalMap.GetDimensions(0, width, height, numMips);

	uint x = pin.TexC.x*width;
	uint y = pin.TexC.y*height;

	uint randSeed = 0;
	if(g_UseTemporalRandom)
		randSeed = initRand(x + width * y, g_FrameCount);
	else
		randSeed = initRand(x, y);

	// Sample neighboring points about p in the hemisphere oriented by n.
	for (int i = 0; i < g_SampleCount; ++i)
	{
		float3 randVec = getNextHemisphereSample(randSeed, n);

		float3 q = p + g_OcclusionRadius * randVec * nextRand(randSeed);

		float4 projQ = mul(float4(q, 1.0f), g_ProjTex);
		projQ /= projQ.w;

		float rz = g_DepthMap.Sample(g_samDepth, projQ.xy).x;

		rz = ndcDepth2ViewDepthRM(g_Proj, rz);

		float3 r = (rz / q.z) * q;

		float distZ = p.z - r.z;
		float dp = max(dot(n, normalize(r - p)), 0.0f);

		float occlusion = dp * occlusionFunction(distZ);

		occlusionSum += occlusion;
	}

	occlusionSum /= g_SampleCount;

	float access = 1.0f - occlusionSum;

	// Sharpen the contrast of the SSAO map to make the SSAO affect more dramatic.
	return saturate(pow(access, g_SsaoPower));
}