#include "GrassManager.h"

#include "CompiledShaders/GenerateGrassStrawsCS.h"

void CGrassManager::Initialize()
{
	__InitGrassGeometry();
	__InitRootSignature();
	__InitPSO();
	m_IsInitialized = true;
}

void CGrassManager::AddGrassPatch(const XMFLOAT3& pos, float scale)
{
	XMMATRIX W = XMMatrixScaling(scale, scale, scale)*XMMatrixTranslation(pos.x, pos.y, pos.z);

	m_InstanceScaleSet.push_back(scale);
	m_InstancePositionSet.push_back(pos);
	m_InstanceTransformSet.push_back(W);
	m_InstanceBoundingObjectSet.push_back(__CalcBoundingObject(pos, scale));

	m_InstanceCount++;
}

void CGrassManager::DeleteGrassPatch(UINT patchIndex)
{
	_ASSERTE(patchIndex < m_InstanceCount);

	m_InstanceScaleSet.erase(m_InstanceScaleSet.begin() + patchIndex);
	m_InstancePositionSet.erase(m_InstancePositionSet.begin() + patchIndex);
	m_InstanceTransformSet.erase(m_InstanceTransformSet.begin() + patchIndex);
	m_InstanceBoundingObjectSet.erase(m_InstanceBoundingObjectSet.begin() + patchIndex);

	m_InstanceCount--;
}

void CGrassManager::UpdateGrassPatch()
{
	_ASSERTE(m_IsInitialized);
	
	CComputeCommandList* pComputeCommandList = GraphicsCore::BeginComputeCommandList();
	_ASSERTE(pComputeCommandList);

	pComputeCommandList->TransitionResource(m_VertexBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, true);

	pComputeCommandList->SetPipelineState(m_PSO);
	pComputeCommandList->SetRootSignature(m_RootSignature);

	float totalTime = (float)GraphicsCore::GetTimer().GetTotalSeconds();

	m_GenerateGrassStrawsCB.p.timeOffset = XMFLOAT2(
		totalTime * WIND_MAP_SPEED_U * WIND_FREQUENCY,
		totalTime * WIND_MAP_SPEED_V * WIND_FREQUENCY);
	m_GenerateGrassStrawsCB.invActivePatchDim = XMFLOAT2(1.f / m_GenerateGrassStrawsCB.p.activePatchDim.x, 1.f / m_GenerateGrassStrawsCB.p.activePatchDim.y);

	pComputeCommandList->SetDynamicConstantBufferView(0, sizeof(m_GenerateGrassStrawsCB), &m_GenerateGrassStrawsCB);
	pComputeCommandList->SetDynamicDescriptor(1, 0, m_GrassTextureSrvSet[kWindOffset]);
	pComputeCommandList->SetDynamicDescriptor(2, 0, m_VertexBuffer.GetUAV());

	// Dispatch.
	XMUINT2 dim = m_GenerateGrassStrawsCB.p.maxPatchDim;
	XMUINT2 groupSize(CeilDivide(dim.x, 8), CeilDivide(dim.y, 8));
	pComputeCommandList->Dispatch(groupSize.x, groupSize.y, 1);

	GraphicsCore::FinishCommandList(pComputeCommandList, true);
}

void CGrassManager::SetGrarssPatchScale(UINT patchIndex, float scale)
{
	_ASSERTE(patchIndex < m_InstanceCount); 
	m_InstanceScaleSet[patchIndex] = scale; 
	XMFLOAT3 pos = m_InstancePositionSet[patchIndex];
	XMMATRIX W = XMMatrixScaling(scale, scale, scale)*XMMatrixTranslation(pos.x, pos.y, pos.z);
	m_InstanceTransformSet[patchIndex] = W;
	m_InstanceBoundingObjectSet[patchIndex] = __CalcBoundingObject(pos, scale);
}

void CGrassManager::SetGrarssPatchPosition(UINT patchIndex, const XMFLOAT3& pos)
{
	_ASSERTE(patchIndex < m_InstanceCount); 
	m_InstancePositionSet[patchIndex] = pos;
	float scale = m_InstanceScaleSet[patchIndex];
	XMMATRIX W = XMMatrixScaling(scale, scale, scale)*XMMatrixTranslation(pos.x, pos.y, pos.z);
	m_InstanceTransformSet[patchIndex] = W;
	m_InstanceBoundingObjectSet[patchIndex] = __CalcBoundingObject(pos, scale);
}

void CGrassManager::SetGrarssPatchTransform(UINT patchIndex, const XMFLOAT3& pos, float scale)
{
	_ASSERTE(patchIndex < m_InstanceCount);
	XMMATRIX W = XMMatrixScaling(scale, scale, scale) * XMMatrixTranslation(pos.x, pos.y, pos.z);
	m_InstanceScaleSet[patchIndex] = scale;
	m_InstancePositionSet[patchIndex] = pos;
	m_InstanceTransformSet[patchIndex] = W;
	m_InstanceBoundingObjectSet[patchIndex] = __CalcBoundingObject(pos, scale);
}

void CGrassManager::__InitGrassGeometry()
{
	// Initialize index and vertex buffers.
	{
		const UINT NumStraws = MAX_GRASS_STRAWS_1D * MAX_GRASS_STRAWS_1D;
		const UINT NumTrianglesPerStraw = N_GRASS_TRIANGLES;
		const UINT NumTriangles = NumStraws * NumTrianglesPerStraw;
		const UINT NumVerticesPerStraw = N_GRASS_VERTICES;
		const UINT NumVertices = NumStraws * NumVerticesPerStraw;
		const UINT NumIndicesPerStraw = NumTrianglesPerStraw * 3;
		const UINT NumIndices = NumStraws * NumIndicesPerStraw;
		UINT strawIndices[NumIndicesPerStraw] = { 0, 2, 1, 1, 2, 3, 2, 4, 3, 3, 4, 5, 4, 6, 5 };
		std::vector<UINT> indices;
		indices.resize(NumIndices);

		UINT indexID = 0;
		for (UINT i = 0, indexID = 0; i < NumStraws; i++)
		{
			UINT baseVertexID = i * NumVerticesPerStraw;
			for (auto index : strawIndices)
			{
				indices[indexID++] = baseVertexID + index;
			}
		}

		m_IndexBuffer.Create(L"GrassPatchIndexBuffer", NumIndices, c_IndexTypeSize, &indices[0]);
		m_IndexBufferView = m_IndexBuffer.CreateIndexBufferView();
		m_VertexBuffer.Create(L"GrassPatchVertexBuffer", NumVertices, c_VertexStride);
		m_VertexBufferView = m_VertexBuffer.CreateVertexBufferView();

		m_IndexBufferCpuCopy.resize(NumIndices * c_IndexTypeSize, 0);
		memcpy(m_IndexBufferCpuCopy.data(), indices.data(), NumIndices * c_IndexTypeSize);
	}

	// Load Textures.
	{	
		auto* pDiffuseTexture = TextureLoader::LoadFromFile(L"Grass/albedo", true);
		_ASSERTE(pDiffuseTexture);
		m_GrassTextureSrvSet[kDiffuseOffset] = pDiffuseTexture->GetSRV();

		auto* pSpecularTexture = TextureLoader::LoadFromFile(L"Grass/specular", true);
		_ASSERTE(pSpecularTexture);
		m_GrassTextureSrvSet[kSpecularOffset] = pSpecularTexture->GetSRV();

		auto* pNormalTexture = TextureLoader::LoadFromFile("Grass/normal", false);
		_ASSERTE(pNormalTexture);
		m_GrassTextureSrvSet[kNormalOffset] = pNormalTexture->GetSRV();

		auto* pWindTexture = TextureLoader::LoadFromFile(L"Grass/wind", false);
		_ASSERTE(pWindTexture);
		m_GrassTextureSrvSet[kWindOffset] = pWindTexture->GetSRV();

		m_GrassTextureSrvSet[kEmissiveOffset] = pDiffuseTexture->GetSRV();
		m_GrassTextureSrvSet[kLightmapOffset] = pDiffuseTexture->GetSRV();
		m_GrassTextureSrvSet[kReflectionOffset] = pDiffuseTexture->GetSRV();
	}
}

void CGrassManager::__InitRootSignature()
{
	m_RootSignature.Reset(3, 1);
	m_RootSignature.InitStaticSampler(0, 0, GraphicsCommon::Sampler::SamplerLinearWrapDesc);
	m_RootSignature[0].InitAsConstantBuffer(0, 0);
	m_RootSignature[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0);
	m_RootSignature[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0, 1, 0);
	m_RootSignature.Finalize(L"GrassPatchRootSignature");
}

void CGrassManager::__InitPSO()
{
	m_PSO.SetRootSignature(m_RootSignature);
	m_PSO.SetComputeShader(g_pGenerateGrassStrawsCS, sizeof(g_pGenerateGrassStrawsCS));
	m_PSO.Finalize();
}

SBoundingObject CGrassManager::__CalcBoundingObject(const XMFLOAT3& pos, float scale)
{
	SBoundingObject boundingObject;
	
	float width = GRASS_PATCH_SIZE_1D * scale;
	float height = FetchParamter().grassHeight * scale * m_GrarssPatchBoundingObjectScaleFactor;

	XMFLOAT3 center = { pos.x + width / 2.0f, pos.y + height / 2.0f, pos.z + width / 2.0f };
	XMFLOAT3 extents = { width / 2.0f + height, height / 2.0f, width / 2.0f + height };
	boundingObject.boundingBox.Center = center;
	boundingObject.boundingBox.Extents = extents;

	BoundingSphere::CreateFromBoundingBox(boundingObject.boundingSphere, boundingObject.boundingBox);
	BoundingOrientedBox::CreateFromBoundingBox(boundingObject.boundingOrientedBox, boundingObject.boundingBox);

	return boundingObject;
}
