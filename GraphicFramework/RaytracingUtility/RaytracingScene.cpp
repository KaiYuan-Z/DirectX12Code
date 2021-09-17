#include "RaytracingScene.h"
#include "../Core/GraphicsCommon.h"
#include"../Core/GraphicsCore.h"

using namespace RaytracingUtility;

CRaytracingScene::CRaytracingScene()
{
}

CRaytracingScene::CRaytracingScene(const std::wstring& name) : CScene(name)
{
}


CRaytracingScene::~CRaytracingScene()
{
}

void CRaytracingScene::InitRaytracingScene()
{
	_ASSERTE(GetModelCountInScene() > 0 && GetInstanceCount() > 0 && !m_IsInitialized);
	
	__BuildBottomLevelAccelerationStructure();
	__BuildVertexAndIndexData();
	__BuildLightData();
	__BuildMeshData();
	__BuildInstanceData();

	m_IsInitialized = true;
}

UINT RaytracingUtility::CRaytracingScene::AddTopLevelAccelerationStructure(const std::wstring& TLAName, bool enableUpdate, UINT hitProgCount)
{
	_ASSERTE(m_IsInitialized);
	
	UINT TLAIndex = m_RtTLAs.EmplaceObject(TLAName);
	
	if(TLAIndex == INVALID_OBJECT_INDEX)
	{
		return INVALID_OBJECT_INDEX;
	}

	bool HasAnimations = false;
	auto& TLA = m_RtTLAs[TLAIndex];
	UINT instanceCount = GetInstanceCount();
	UINT grassPatchCount = m_GrassManager.GetGrarssPatchCount();
	std::vector<SRayTracingInstanceDesc> RtInstanceDescs(instanceCount + grassPatchCount);
	UINT instanceContributionToHitGroupIndex = 0;

	// For Models.
	for (UINT instancecIndex = 0; instancecIndex < instanceCount; instancecIndex++)
	{
		CModelInstance* pInstance = GetInstance(instancecIndex); _ASSERTE(pInstance);
		CModel* pHostModel = pInstance->GetModel(); _ASSERTE(pHostModel);
		UINT hostModelIndex = QueryModelIndexInScene(pHostModel->GetModelName()); _ASSERTE(hostModelIndex != INVALID_OBJECT_INDEX);
		UINT hostModelMeshCount = pHostModel->GetHeader().meshCount;
		RtInstanceDescs[instancecIndex].SetTransformMatrix(pInstance->GetTransformMatrix());
		RtInstanceDescs[instancecIndex].AccelerationStructure = m_RtBLAs[hostModelIndex].GetAccelerationStructureGpuVirtualAddr();
		RtInstanceDescs[instancecIndex].InstanceMask = 0xff;
		RtInstanceDescs[instancecIndex].InstanceID = instancecIndex;
		RtInstanceDescs[instancecIndex].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		RtInstanceDescs[instancecIndex].InstanceContributionToHitGroupIndex = instanceContributionToHitGroupIndex;
		instanceContributionToHitGroupIndex += hostModelMeshCount * hitProgCount;
	}

	// For Grass.
	UINT grassPatchOffset = instanceCount;
	UINT grassModelIndex = GetModelCountInScene();
	UINT grassModelMeshCount = 1;
	for (UINT grassPatchIndex = 0; grassPatchIndex < grassPatchCount; grassPatchIndex++)
	{	
		RtInstanceDescs[grassPatchOffset + grassPatchIndex].SetTransformMatrix(m_GrassManager.GetGrarssPatchTransform(grassPatchIndex));
		RtInstanceDescs[grassPatchOffset + grassPatchIndex].AccelerationStructure = m_RtBLAs[grassModelIndex].GetAccelerationStructureGpuVirtualAddr();
		RtInstanceDescs[grassPatchOffset + grassPatchIndex].InstanceMask = 0xff;
		RtInstanceDescs[grassPatchOffset + grassPatchIndex].InstanceID = grassPatchOffset + grassPatchIndex;
		RtInstanceDescs[grassPatchOffset + grassPatchIndex].Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		RtInstanceDescs[grassPatchOffset + grassPatchIndex].InstanceContributionToHitGroupIndex = instanceContributionToHitGroupIndex;
		instanceContributionToHitGroupIndex += grassModelMeshCount * hitProgCount;
	}
	if (grassPatchCount > 0) HasAnimations = true;

	TLA.Create(RtInstanceDescs, enableUpdate, HasAnimations);

	m_TLAHitProgCounts.push_back(hitProgCount);
	m_TLAHitProgRecordCounts.push_back(instanceContributionToHitGroupIndex);

	m_RtSceneInfo.TotalTLACount++;

	return TLAIndex;
}

void CRaytracingScene::OnFrameUpdate()
{
	_ASSERTE(m_IsInitialized);

	CScene::OnFrameUpdate();

	// Update Grass BLA.
	UINT grassPatchCount = m_GrassManager.GetGrarssPatchCount();
	if (grassPatchCount > 0)
	{
		UINT grassBLAIndex = GetModelCountInScene();		
		GraphicsCore::BufferCopy(m_RtVertexBuffer, m_GrassManager.FetchGrarssPatchVertexBuffer(), m_GrassVertexDataSize, m_GrassVertexDataOffset);
		m_RtBLAs[grassBLAIndex].UpdateAccelerationStructure();
	}

	// Update Model BLA.
	for (UINT TLAIndex = 0; TLAIndex < m_RtSceneInfo.TotalTLACount; TLAIndex++)
	{
		auto& TLA = m_RtTLAs[TLAIndex];
		if (TLA.IsUpdatable())
		{
			UINT dirtyInstanceCount = (UINT)m_RecentDirtyInstanceSet.InstanceSet.size();
			for (size_t i = 0; i < dirtyInstanceCount; i++)
			{
				UINT instanceIndex = m_RecentDirtyInstanceSet.InstanceSet[i];
				CModelInstance* pInstance = GetInstance(instanceIndex); _ASSERTE(pInstance);
				TLA.SetInstanceTransform(instanceIndex, pInstance->GetTransformMatrix());
				m_RtInstanceWorldSet[instanceIndex] = pInstance->GetTransformMatrix();
				if (pInstance->IsDynamic()) m_RtInstanceUpdateFrameIdSet[instanceIndex] = (UINT)m_RecentDirtyInstanceSet.FrameID;
			}

			TLA.UpdateAccelerationStructure();
		}
	}

	// Prev Transform Matrix For Model.
	UINT intanceCount = GetInstanceCount();
	for (UINT instIndex = 0; instIndex < intanceCount; instIndex++)
	{
		CModelInstance* pInstance = GetInstance(instIndex); _ASSERTE(pInstance);
		m_RtInstanceWorldSet[instIndex + intanceCount + grassPatchCount] = pInstance->IsUpdatedInCurrentFrame() ? pInstance->GetPrevTransformMatrix() : pInstance->GetTransformMatrix();
	}

	// Prev Transform Matrix For Grass.
	for (UINT grassPatchIndex = 0; grassPatchIndex < grassPatchCount; grassPatchIndex++)
	{
		m_RtInstanceWorldSet[grassPatchIndex + intanceCount + grassPatchCount + intanceCount] = m_GrassManager.GetGrarssPatchTransform(grassPatchIndex);
	}
}

UINT RaytracingUtility::CRaytracingScene::QueryTLAIndexByName(const std::wstring& TLAName)
{
	return m_RtTLAs.QueryObjectEntry(TLAName);
}

void CRaytracingScene::__BuildBottomLevelAccelerationStructure()
{
	UINT modelCount = GetModelCountInScene();
	UINT grassPatchCount = m_GrassManager.GetGrarssPatchCount();
	if (grassPatchCount > 0)
	{
		m_RtBLAs.resize(modelCount + 1);
	}
	else
	{
		m_RtBLAs.resize(modelCount);
	}	

	// For Models.
	for (UINT modelIndex = 0; modelIndex < modelCount; modelIndex++)
	{
		CModel* pModel = GetModelInScene(modelIndex); _ASSERTE(pModel);
		m_RtBLAs[modelIndex].Create(*pModel);
	}
	// For Grass.
	if(grassPatchCount > 0)
	{
		UINT grassModelIndex = modelCount;
		std::vector<SGeometryInfo> grassMeshInfo; grassMeshInfo.resize(1);
		grassMeshInfo[0].IndexBufferGPUAddr = m_GrassManager.FetchGrarssPatchIndexBuffer().GetGpuVirtualAddress();
		grassMeshInfo[0].IndexCount = m_GrassManager.FetchGrarssPatchIndexBuffer().GetElementCount();
		grassMeshInfo[0].IndexFormat = DXGI_FORMAT_R32_UINT;
		grassMeshInfo[0].VertexBufferGPUAddr = m_GrassManager.FetchGrarssPatchVertexBuffer().GetGpuVirtualAddress();
		grassMeshInfo[0].VertexBufferStrideInBytes = m_GrassManager.c_VertexStride;
		grassMeshInfo[0].VertexCount = m_GrassManager.FetchGrarssPatchVertexBuffer().GetElementCount();
		grassMeshInfo[0].VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		m_RtBLAs[grassModelIndex].Create(grassMeshInfo, true);
	}
}

void CRaytracingScene::__BuildVertexAndIndexData()
{	
	UINT totalIndexCount = 0;
	UINT totalVertexCount = 0;
	UINT totalIndexDataSize = 0;
	UINT totalVertexDataSize = 0;

	UINT grassPatchCount = m_GrassManager.GetGrarssPatchCount();

	// For Models.
	UINT modelCount = GetModelCountInScene();
	for (UINT modelIndex = 0; modelIndex < modelCount; modelIndex++)
	{
		CModel* pModel = GetModelInScene(modelIndex); _ASSERTE(pModel);
		_ASSERTE(pModel->GetIndexDataTypeSize() == sizeof(UINT)); // Note: We only support uint32_t index now.
		const CModel::SHeader& header = pModel->GetHeader();
		totalIndexCount += header.indexDataByteSize / sizeof(UINT);
		totalIndexDataSize += header.indexDataByteSize;
		totalVertexCount += header.vertexDataByteSize / sizeof(GraphicsCommon::Vertex::PosTexNormTanBitan);
		totalVertexDataSize += header.vertexDataByteSize;
	}
	// For Grass.
	if (grassPatchCount > 0)
	{
		totalIndexCount += m_GrassManager.FetchGrarssPatchIndexBuffer().GetElementCount();
		totalIndexDataSize += (UINT)m_GrassManager.FetchGrarssPatchIndexBuffer().GetBufferSize();
		totalVertexCount += m_GrassManager.FetchGrarssPatchVertexBuffer().GetElementCount();
		totalVertexDataSize += (UINT)m_GrassManager.FetchGrarssPatchVertexBuffer().GetBufferSize();
	}
	
	UINT indexDataOffset = 0;
	UINT vertexDataOffset = 0;
	std::vector<byte> indexInitData(totalIndexDataSize);
	std::vector<byte> vertexInitData(totalVertexDataSize);

	// For Models.
	for (UINT modelIndex = 0; modelIndex < modelCount; modelIndex++)
	{
		CModel* pModel = GetModelInScene(modelIndex); _ASSERTE(pModel);
		const CModel::SHeader& header = pModel->GetHeader();
		memcpy(indexInitData.data() + indexDataOffset, pModel->GetIndexDataCpuCopy(), header.indexDataByteSize);
		indexDataOffset += header.indexDataByteSize;
		memcpy(vertexInitData.data() + vertexDataOffset, pModel->GetVertexDataCpuCopy(), header.vertexDataByteSize);
		vertexDataOffset += header.vertexDataByteSize;
	}
	// For Grass.
	if (grassPatchCount > 0)
	{
		m_GrassVertexDataOffset = vertexDataOffset;
		m_GrassVertexDataSize = (UINT)m_GrassManager.FetchGrarssPatchVertexBuffer().GetBufferSize();

		memcpy(indexInitData.data() + indexDataOffset, m_GrassManager.GetGrarssPatchIndexBufferCpuCopy().data(), m_GrassManager.GetGrarssPatchIndexBufferCpuCopy().size());
		indexDataOffset += (UINT)m_GrassManager.GetGrarssPatchIndexBufferCpuCopy().size();
		memset(vertexInitData.data() + vertexDataOffset, 0, m_GrassManager.FetchGrarssPatchVertexBuffer().GetBufferSize());
		vertexDataOffset += (UINT)m_GrassManager.FetchGrarssPatchVertexBuffer().GetBufferSize();
	}

	m_RtIndexBuffer.Create(L"RtIndexData", totalIndexCount, sizeof(UINT), indexInitData.data());
	m_RtVertexBuffer.Create(L"RtVertexBuffer", totalVertexCount, sizeof(GraphicsCommon::Vertex::PosTexNormTanBitan), vertexInitData.data());
}

void CRaytracingScene::__BuildInstanceData()
{
	UINT instanceCount = GetInstanceCount();
	UINT grassPatchCount = m_GrassManager.GetGrarssPatchCount();
	m_RtSceneInfo.TotalInstanceCount = instanceCount + grassPatchCount;
	m_RtInstanceDataSet.resize(instanceCount + grassPatchCount);
	m_RtInstanceWorldSet.resize((instanceCount + grassPatchCount) * 2);// CurrentInstanceWorld/PreInstanceWorld
	m_RtInstanceUpdateFrameIdSet.resize(instanceCount + grassPatchCount);

	// For Models.
	for (UINT instanceIndex = 0; instanceIndex < instanceCount; instanceIndex++)
	{
		CModelInstance* pInstance = GetInstance(instanceIndex); _ASSERTE(pInstance);
		CModel* pHostModel = pInstance->GetModel(); _ASSERTE(pHostModel);
		UINT hostModelIndex = QueryModelIndexInScene(pHostModel->GetModelName()); _ASSERTE(hostModelIndex != INVALID_OBJECT_INDEX);
		m_RtInstanceDataSet[instanceIndex].InstanceTag = pInstance->IsDynamic() ? DYNAMIC_INSTANCE_TAG : 0;
		m_RtInstanceDataSet[instanceIndex].InstanceIndex = instanceIndex;
		m_RtInstanceDataSet[instanceIndex].ModelIndex = hostModelIndex;	
		m_RtInstanceDataSet[instanceIndex].FirstMeshGlobalIndex = m_RtModelFirstMeshGlobalIndexSet[hostModelIndex];
		m_RtInstanceWorldSet[instanceIndex] = pInstance->GetTransformMatrix();
	}
	// For Grass.
	UINT grassPatchInstanceOffset = instanceCount;
	UINT grassPatchModelOffset = GetModelCountInScene();
	for (UINT grassPatchIndex = 0; grassPatchIndex < grassPatchCount; grassPatchIndex++)
	{
		m_RtInstanceDataSet[grassPatchInstanceOffset + grassPatchIndex].InstanceTag = DEFORMABLE_INSTANCE_TAG | GRASS_INST_TAG | DYNAMIC_INSTANCE_TAG;
		m_RtInstanceDataSet[grassPatchInstanceOffset + grassPatchIndex].InstanceIndex = grassPatchInstanceOffset + grassPatchIndex;
		m_RtInstanceDataSet[grassPatchInstanceOffset + grassPatchIndex].ModelIndex = grassPatchModelOffset + 0;
		m_RtInstanceDataSet[grassPatchInstanceOffset + grassPatchIndex].FirstMeshGlobalIndex = m_RtModelFirstMeshGlobalIndexSet[grassPatchModelOffset + 0];
		m_RtInstanceWorldSet[grassPatchInstanceOffset + grassPatchIndex] = m_GrassManager.GetGrarssPatchTransform(grassPatchIndex);
	}

	m_RtInstanceDataBuffer.Create(L"RtInstanceData", (UINT)m_RtInstanceDataSet.size(), sizeof(SRtInstanceData), m_RtInstanceDataSet.data());
}

void CRaytracingScene::__BuildLightData()
{
	const UINT c_MaxLightCount = 256;

	std::vector<SLightData> lightDataSet(c_MaxLightCount);

	UINT lightCount = GetLightCount();
	_ASSERTE(lightCount <= c_MaxLightCount);
	m_RtSceneInfo.TotalLightCount = lightCount;	
	for (UINT i = 0; i < lightCount; i++)
	{
		CLight* pLight = GetLight(i);
		_ASSERTE(pLight);
		lightDataSet[i] = pLight->GetLightData();
	}

	m_RtLightDataBuffer.Create(L"LightData", c_MaxLightCount, sizeof(SLightData), lightDataSet.data());
}

void CRaytracingScene::__BuildMeshData()
{
	UINT totalMaterialCount = 0;
	UINT totalMeshCount = 0;
	UINT modelCount = GetModelCountInScene();

	for (UINT modelIndex = 0; modelIndex < modelCount; modelIndex++)
	{
		CModel* pModel = GetModelInScene(modelIndex); _ASSERTE(pModel);
		totalMaterialCount += pModel->GetHeader().materialCount;
		totalMeshCount += pModel->GetHeader().meshCount;
	}

	std::vector<SRtMaterial> rtMaterialSet(totalMaterialCount + 1);
	std::vector<SRtMeshData> rtMeshDataSet(totalMeshCount + 1);
	m_RtTexDescSet.resize((totalMaterialCount + 1) * 6);

	UINT rtTexDescIndex = 0;
	UINT modelIndexDataStartOffset = 0;
	UINT modelVertexDataStartOffset = 0;
	UINT modelMaterialStartIndex = 0;
	UINT modelMeshStartIndex = 0;

	m_RtModelFirstMeshGlobalIndexSet.resize(modelCount + 1);
	for (UINT modelIndex = 0; modelIndex < modelCount; modelIndex++)
	{
		m_RtModelFirstMeshGlobalIndexSet[modelIndex] = modelMeshStartIndex;
		
		CModel* pModel = GetModelInScene(modelIndex); _ASSERTE(pModel);
		UINT materialCount = pModel->GetHeader().materialCount;
		UINT meshCount = pModel->GetHeader().meshCount;
		for (UINT materialIndex = 0; materialIndex < materialCount; materialIndex++)
		{
			const D3D12_CPU_DESCRIPTOR_HANDLE* pSRVs = pModel->GetSRVs(materialIndex);
			UINT globalMaterialIndex = modelMaterialStartIndex + materialIndex;
			rtMaterialSet[globalMaterialIndex].TexDiffuseIndex = rtTexDescIndex;
			m_RtTexDescSet[rtTexDescIndex] = pSRVs[0];
			rtMaterialSet[globalMaterialIndex].TexSpecularIndex = rtTexDescIndex + 1;
			m_RtTexDescSet[rtTexDescIndex + 1] = pSRVs[1];
			rtMaterialSet[globalMaterialIndex].TexEmissiveIndex = rtTexDescIndex + 2;
			m_RtTexDescSet[rtTexDescIndex + 2] = pSRVs[2];
			rtMaterialSet[globalMaterialIndex].TexNormalIndex = rtTexDescIndex + 3;
			m_RtTexDescSet[rtTexDescIndex + 3] = pSRVs[3];
			rtMaterialSet[globalMaterialIndex].TexLightmapIndex = rtTexDescIndex + 4;
			m_RtTexDescSet[rtTexDescIndex + 4] = pSRVs[4];
			rtMaterialSet[globalMaterialIndex].TexReflectionIndex = rtTexDescIndex + 5;
			m_RtTexDescSet[rtTexDescIndex + 5] = pSRVs[5];
			rtTexDescIndex += 6;

			const CModel::SMaterial& material = pModel->GetMaterial(materialIndex);
			const XMFLOAT3& DiffuseAlbedo = material.diffuse;
			rtMaterialSet[globalMaterialIndex].DiffuseAlbedo = { DiffuseAlbedo.x, DiffuseAlbedo.y, DiffuseAlbedo.z, 1.0 };
			rtMaterialSet[globalMaterialIndex].Shininess = material.shininess;
			rtMaterialSet[globalMaterialIndex].FresnelR0 = material.specular;
		}

		for (UINT meshIndex = 0; meshIndex < meshCount; meshIndex++)
		{
			const CModel::SMesh& mesh = pModel->GetMesh(meshIndex);
			UINT globalMeshIndex = modelMeshStartIndex + meshIndex;
			rtMeshDataSet[globalMeshIndex].MaterialIndex = modelMaterialStartIndex + mesh.materialIndex;
			rtMeshDataSet[globalMeshIndex].IndexOffsetBytes = modelIndexDataStartOffset + mesh.indexDataByteOffset;
			rtMeshDataSet[globalMeshIndex].IndexTypeSize = sizeof(UINT);
			rtMeshDataSet[globalMeshIndex].VertexOffsetBytes = modelVertexDataStartOffset + mesh.vertexDataByteOffset;
			rtMeshDataSet[globalMeshIndex].VertexStride = sizeof(GraphicsCommon::Vertex::PosTexNormTanBitan);
		}

		modelIndexDataStartOffset += pModel->GetHeader().indexDataByteSize;
		modelVertexDataStartOffset += pModel->GetHeader().vertexDataByteSize;
		modelMaterialStartIndex += materialCount;
		modelMeshStartIndex += meshCount;
	}

	// Grass MeshData.
	{
		UINT grassModelIndex = modelCount;
		UINT grassGlobalMeshIndex = modelMeshStartIndex + 0;
		UINT grassGlobalMaterialIndex = modelMaterialStartIndex + 0;
		UINT grassGlobalTexDescIndex = rtTexDescIndex + 0;

		m_RtModelFirstMeshGlobalIndexSet[grassModelIndex] = grassGlobalMeshIndex;

		rtMeshDataSet[grassGlobalMeshIndex].MaterialIndex = 0;
		rtMeshDataSet[grassGlobalMeshIndex].IndexOffsetBytes = modelIndexDataStartOffset + 0;
		rtMeshDataSet[grassGlobalMeshIndex].IndexTypeSize = sizeof(UINT);
		rtMeshDataSet[grassGlobalMeshIndex].VertexOffsetBytes = modelVertexDataStartOffset + 0;
		rtMeshDataSet[grassGlobalMeshIndex].VertexStride = sizeof(GraphicsCommon::Vertex::PosTexNormTanBitan);

		const auto& material = m_GrassManager.FetchGrassPatchMaterial();
		rtMaterialSet[grassGlobalMaterialIndex].DiffuseAlbedo = material.DiffuseAlbedo;
		rtMaterialSet[grassGlobalMaterialIndex].Shininess = material.Shininess;
		rtMaterialSet[grassGlobalMaterialIndex].FresnelR0 = material.FresnelR0;

		m_RtTexDescSet[grassGlobalTexDescIndex + 0] = m_GrassManager.GetGrarssPatchTextureSRVs()[0];
		rtMaterialSet[grassGlobalMaterialIndex].TexDiffuseIndex = grassGlobalTexDescIndex + 0;
		m_RtTexDescSet[grassGlobalTexDescIndex + 1] = m_GrassManager.GetGrarssPatchTextureSRVs()[1];
		rtMaterialSet[grassGlobalMaterialIndex].TexSpecularIndex = grassGlobalTexDescIndex + 1;
		m_RtTexDescSet[grassGlobalTexDescIndex + 2] = m_GrassManager.GetGrarssPatchTextureSRVs()[2];
		rtMaterialSet[grassGlobalMaterialIndex].TexEmissiveIndex = grassGlobalTexDescIndex + 2;
		m_RtTexDescSet[grassGlobalTexDescIndex + 3] = m_GrassManager.GetGrarssPatchTextureSRVs()[3];
		rtMaterialSet[grassGlobalMaterialIndex].TexNormalIndex = grassGlobalTexDescIndex + 3;
		m_RtTexDescSet[grassGlobalTexDescIndex + 4] = m_GrassManager.GetGrarssPatchTextureSRVs()[4];
		rtMaterialSet[grassGlobalMaterialIndex].TexLightmapIndex = grassGlobalTexDescIndex + 4;
		m_RtTexDescSet[grassGlobalTexDescIndex + 5] = m_GrassManager.GetGrarssPatchTextureSRVs()[5];
		rtMaterialSet[grassGlobalMaterialIndex].TexReflectionIndex = grassGlobalTexDescIndex + 5;
	}

	m_RtMaterialBuffer.Create(L"RtMaterialBuffer", (UINT)rtMaterialSet.size(), sizeof(SRtMaterial), rtMaterialSet.data());
	m_RtMeshDataBuffer.Create(L"RtMeshDataBuffer", (UINT)rtMeshDataSet.size(), sizeof(SRtMeshData), rtMeshDataSet.data());
}

void RaytracingUtility::CRaytracingScene::__BuildHitProgLocalRootSignature(CRootSignature& rootSignature)
{
	_ASSERTE(m_IsInitialized);

	rootSignature.Reset(1, 0);
	rootSignature[0].InitAsConstants(0, 2, 2, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature.Finalize(L"HitProgLocalRoot", D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);
}

void CRaytracingScene::__BuildMeshHitShaderTable(UINT TLAIndex, CRayTracingPipelineStateObject* pRtPSO, const std::vector<std::wstring>& hitGroupNameSet, CRaytracingShaderTable& outShaderTable)
{
	//=============================================================================================================================================================================
	// HitGroupRecordAddress =
	// D3D12_DISPATCH_RAYS_DESC.HitGroupTable.StartAddress +					// from: DispatchRays()     
	// D3D12_DISPATCH_RAYS_DESC.HitGroupTable.StrideInBytes *					// from: DispatchRays()
	// (         
	//  	RayContributionToHitGroupIndex +									// from shader: TraceRay()   Note: hitProgIndex      
	//  	(MultiplierForGeometryContributionToHitGroupIndex *					// from shader: TraceRay()   Note: hitProgCount   
	//  	GeometryContributionToHitGroupIndex) +								// system generated index of geometry in bottom level acceleration structure (0,1,2,3..)        
	//  	D3D12_RAYTRACING_INSTANCE_DESC.InstanceContributionToHitGroupIndex	// from instance
	// )                                                  
	//
	// for each [Instance]
	//	    for each [InstanceMesh]
	//			for each [hitProg]
	//=============================================================================================================================================================================

	_ASSERTE(m_IsInitialized && pRtPSO);

	UINT meshIndex = 0;
	UINT modelCount = GetModelCountInScene();
	std::vector<UINT> modelFirstMeshIndexTable(modelCount + 1);

	// For Models.
	for (UINT modelIndex = 0; modelIndex < modelCount; modelIndex++)
	{
		CModel* pModel = GetModelInScene(modelIndex); _ASSERTE(pModel);
		modelFirstMeshIndexTable[modelIndex] = meshIndex;
		meshIndex += pModel->GetHeader().meshCount;
	}
	// For Grass.
	modelFirstMeshIndexTable[modelCount] = meshIndex;

	_ASSERTE(TLAIndex < m_RtSceneInfo.TotalTLACount);
	UINT hitProgCount = m_TLAHitProgCounts[TLAIndex];
	UINT hitProgRecordCount = m_TLAHitProgRecordCounts[TLAIndex];
	outShaderTable.ResetShaderTable(hitProgRecordCount);

	UINT recordIndex = 0;
	
	// For Models.
	UINT instanceCount = GetInstanceCount();
	for (UINT instancecIndex = 0; instancecIndex < instanceCount; instancecIndex++)
	{
		CModelInstance* pInstance = GetInstance(instancecIndex); _ASSERTE(pInstance);
		CModel* pHostModel = pInstance->GetModel(); _ASSERTE(pHostModel);
		UINT hostModelIndex = QueryModelIndexInScene(pHostModel->GetModelName()); _ASSERTE(hostModelIndex != INVALID_OBJECT_INDEX);
		UINT hostModelMeshCount = pHostModel->GetHeader().meshCount;
		for (UINT modelMeshIndex = 0; modelMeshIndex < hostModelMeshCount; modelMeshIndex++)
		{
			for (UINT hitProgIndex = 0; hitProgIndex < hitProgCount; hitProgIndex++)
			{
				UINT meshGlobalIndex = modelFirstMeshIndexTable[hostModelIndex] + modelMeshIndex;
				UINT meshLocalIndex = modelMeshIndex;
				UINT paramArray[2] = { meshGlobalIndex, meshLocalIndex };
				outShaderTable[recordIndex].ResetShaderRecord(pRtPSO->QueryShaderIdentifier(hitGroupNameSet[hitProgIndex]), 1);
				outShaderTable[recordIndex].SetLocalRoot32BitConstants(0, 2, paramArray);
				outShaderTable[recordIndex].FinalizeShaderRecord();
				recordIndex++;
			}
		}
	}
	// For Grass.
	UINT grassPatchCount = m_GrassManager.GetGrarssPatchCount();
	UINT grassModelIndex = GetModelCountInScene();
	UINT grassModelMeshCount = 1;
	for (UINT grassPatchIndex = 0; grassPatchIndex < grassPatchCount; grassPatchIndex++)
	{	
		for (UINT modelMeshIndex = 0; modelMeshIndex < grassModelMeshCount; modelMeshIndex++)
		{
			for (UINT hitProgIndex = 0; hitProgIndex < hitProgCount; hitProgIndex++)
			{
				UINT meshGlobalIndex = modelFirstMeshIndexTable[grassModelIndex] + modelMeshIndex;
				UINT meshLocalIndex = modelMeshIndex;
				UINT paramArray[2] = { meshGlobalIndex, meshLocalIndex };
				outShaderTable[recordIndex].ResetShaderRecord(pRtPSO->QueryShaderIdentifier(hitGroupNameSet[hitProgIndex]), 1);
				outShaderTable[recordIndex].SetLocalRoot32BitConstants(0, 2, paramArray);
				outShaderTable[recordIndex].FinalizeShaderRecord();
				recordIndex++;
			}
		}
	}

	outShaderTable.FinalizeShaderTable();
}