#pragma once
#include "MathUtility.h"

#define DYNAMIC_INSTANCE_TAG 0x00004000
#define DEFORMABLE_INSTANCE_TAG 0x00001000

struct SInstanceData
{
	XMMATRIX World = XMMatrixIdentity();
	UINT SceneModelIndex = 0;
	UINT InstanceIndex = 0;
	UINT InstanceTag = 0;
	float Pad;
};

struct SMeshMaterial
{
	XMFLOAT4 DiffuseAlbedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	XMFLOAT3 FresnelR0 = { 0.1f, 0.1f, 0.1f };
	float Shininess = 0.1f;
};
