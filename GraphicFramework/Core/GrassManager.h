#pragma once
#include "GraphicsCore.h"
#include "GraphicsCommon.h"
#include "Texture.h"
#include "RenderCommon.h"
#include "CameraBasic.h"
#include "BoundingObject.h"

/*
* Ref Frome: Microsoft's DirectX12 Samples.
* This manager only has LOD0-grass.
*/

#define GRASS_MODEL_INDEX 0
#define GRASS_INST_TAG 0x00000100

#define N_GRASS_TRIANGLES 5
#define N_GRASS_VERTICES 7
#define MAX_GRASS_STRAWS_1D 30
#define GRASS_PATCH_SIZE_1D 15.0f

#define WIND_MAP_SPEED_U 1.0f
#define WIND_MAP_SPEED_V 1.0f
#define WIND_FREQUENCY 0.04f

namespace RaytracingUtility
{
	class CRaytracingScene;
}

struct SGenerateGrassStrawsConstantBuffer_AppParams
{
	XMUINT2 activePatchDim = { 5, 5 }; // Dimensions of active grass straws.
	const XMUINT2 maxPatchDim = { MAX_GRASS_STRAWS_1D, MAX_GRASS_STRAWS_1D };    // Dimensions of the whole vertex buffer.

	XMFLOAT2 timeOffset = {0.0f, 0.0f};
	float grassHeight = 1.0f;
	const float grassScale = 1.0f;

	const XMFLOAT3 patchSize = { GRASS_PATCH_SIZE_1D, 0.0f, GRASS_PATCH_SIZE_1D };
	float grassThickness = 1.0f;

	XMFLOAT3 windDirection = {0.0f, 0.0f, 0.0f};
	float windStrength = 1.0f;

	float positionJitterStrength = 1.0f;
	float bendStrengthAlongTangent = 1.0f;
	float padding[2];
};

struct SGenerateGrassStrawsConstantBuffer
{
	XMFLOAT2 invActivePatchDim;
	float padding1;
	float padding2;
	SGenerateGrassStrawsConstantBuffer_AppParams p;
};

class CGrassManager
{
	enum ETextureOffset
	{
		kDiffuseOffset = 0,
		kSpecularOffset,
		kEmissiveOffset,
		kNormalOffset,
		kLightmapOffset,
		kReflectionOffset,
		kWindOffset
	};

public:
	void AddGrassPatch(const XMFLOAT3& pos, float scale = 1.0f);

	void DeleteGrassPatch(UINT patchIndex);

	SGenerateGrassStrawsConstantBuffer_AppParams& FetchParamter() { return m_GenerateGrassStrawsCB.p; }

	SMeshMaterial& FetchGrassPatchMaterial() { return m_GrassMaterial; }

	void SetGrarssPatchScale(UINT patchIndex, float scale);
	float GetGrarssPatchScale(UINT patchIndex) const { _ASSERTE(patchIndex < m_InstanceCount); return m_InstanceScaleSet[patchIndex]; }

	void SetGrarssPatchPosition(UINT patchIndex, const XMFLOAT3& pos);
	const XMFLOAT3& GetGrarssPatchPosition(UINT patchIndex) const { _ASSERTE(patchIndex < m_InstanceCount); return m_InstancePositionSet[patchIndex]; }

	void SetGrarssPatchTransform(UINT patchIndex, const XMFLOAT3& pos, float scale);
	const XMMATRIX& GetGrarssPatchTransform(UINT patchIndex) const { _ASSERTE(patchIndex < m_InstanceCount); return m_InstanceTransformSet[patchIndex]; }

	void SetGrarssPatchBoundingObjectScaleFactor(float factor) { m_GrarssPatchBoundingObjectScaleFactor = factor; }
	float GetGrassPatchBoundingObjectScaleFactor() const { return m_GrarssPatchBoundingObjectScaleFactor; }

	const SBoundingObject& GetGrarssPatchBoundingObject(UINT patchIndex) const { _ASSERTE(patchIndex < m_InstanceCount); return m_InstanceBoundingObjectSet[patchIndex]; }

	UINT GetGrarssPatchCount() const { return m_InstanceCount; }

	const std::vector<XMMATRIX>& GetGrarssPatchTransformSet() const { return m_InstanceTransformSet; }

	CStructuredBuffer& FetchGrarssPatchVertexBuffer() { return m_VertexBuffer; }
	CByteAddressBuffer& FetchGrarssPatchIndexBuffer() { return m_IndexBuffer; }

	const std::vector<unsigned char>& GetGrarssPatchIndexBufferCpuCopy() const { return m_IndexBufferCpuCopy; }

	const D3D12_VERTEX_BUFFER_VIEW& GetGrarssPatchVertexBufferView() const { return m_VertexBufferView; }
	const D3D12_INDEX_BUFFER_VIEW& GetGrarssPatchIndexBufferView() const { return m_IndexBufferView;}

	const D3D12_CPU_DESCRIPTOR_HANDLE* GetGrarssPatchTextureSRVs() const { return m_GrassTextureSrvSet; }

	const UINT c_VertexStride = sizeof(GraphicsCommon::Vertex::PosTexNormTanBitan);
	const UINT c_IndexTypeSize = sizeof(UINT);

private:
	void Initialize();
	void UpdateGrassPatch();

	void __InitGrassGeometry();
	void __InitRootSignature();
	void __InitPSO();

	SBoundingObject __CalcBoundingObject(const XMFLOAT3& pos, float scale);

	CRootSignature m_RootSignature;
	CComputePSO m_PSO;
	SGenerateGrassStrawsConstantBuffer m_GenerateGrassStrawsCB;

	D3D12_CPU_DESCRIPTOR_HANDLE m_GrassTextureSrvSet[7] = { 0, 0, 0, 0, 0, 0, 0 };

	CStructuredBuffer m_VertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;

	CByteAddressBuffer m_IndexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;
	std::vector<unsigned char> m_IndexBufferCpuCopy;

	SMeshMaterial m_GrassMaterial;

	UINT m_InstanceCount = 0;
	std::vector<float> m_InstanceScaleSet;
	std::vector<XMFLOAT3> m_InstancePositionSet;
	std::vector<XMMATRIX> m_InstanceTransformSet;
	std::vector<SBoundingObject> m_InstanceBoundingObjectSet;
	float m_GrarssPatchBoundingObjectScaleFactor = 2.0f;

	bool m_IsInitialized = false;

	friend class CScene;
};