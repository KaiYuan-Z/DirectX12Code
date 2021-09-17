#pragma once
#include "MathUtility.h"

struct SSubSurfaceScatteringParam
{
	float SSSMaxDist;
	float SSSDistortion;
	float SSSPower;
	float SSSScale;
};

struct SSDFParam
{
	XMUINT3		SDFSize;
	float		Scale;
	XMFLOAT3	UVOffset;
	UINT		SDFIndex;
	XMFLOAT3	LocalObjectExtent;
	float		Push;
	float		SurfaceThreshold;
	float		SoftShadowFactor;
	XMFLOAT2	Padding;

	SSDFParam() : SDFSize(XMUINT3(0, 0, 0)), Scale(1.0f), UVOffset(XMFLOAT3(0.5f, 0.5f, 0.5f)), SDFIndex(0),
		LocalObjectExtent(XMFLOAT3(0.0f, 0.0f, 0.0f)), Push(0), SurfaceThreshold(0), SoftShadowFactor(0)
	{}
};

struct SModelSDFData
{
	SSubSurfaceScatteringParam		SSSParam;
	SSDFParam						SDFParam;
	bool							IsUsingSSS;
};