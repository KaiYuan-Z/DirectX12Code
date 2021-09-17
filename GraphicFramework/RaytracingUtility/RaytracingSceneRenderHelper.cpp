#include "RaytracingSceneRenderHelper.h"
#include "Include/halton_enum.h"
#include "Include/halton_sampler.h"

using namespace RaytracingUtility;

CRaytracingSceneRenderHelper::CRaytracingSceneRenderHelper()
{
}

CRaytracingSceneRenderHelper::~CRaytracingSceneRenderHelper()
{
}

void CRaytracingSceneRenderHelper::InitRender(CRaytracingScene* pRtScene, UINT activeTLAIndex, UINT activeCameraIndex)
{
	_ASSERTE(pRtScene);
	_ASSERTE(activeTLAIndex < pRtScene->m_RtSceneInfo.TotalTLACount);

	m_pRtScene = pRtScene;
	m_ActiveTLAIndex = activeTLAIndex;
	//m_pRtScene->SetActiveCamera(activeCameraIndex);

	m_RtTexDescCount = (UINT)pRtScene->m_RtTexDescSet.size();

	m_PerSceneCB.InstanceCount = pRtScene->m_RtSceneInfo.TotalInstanceCount;
	m_PerSceneCB.LightCount = pRtScene->m_RtSceneInfo.TotalLightCount;
	m_PerSceneCB.HitProgCount = pRtScene->m_TLAHitProgCounts[m_ActiveTLAIndex];
	m_PerSceneConstantBuffer.Create(L"PerSceneConstantBuffer", 1, sizeof(m_PerSceneCB), &m_PerSceneCB);

	ResetHaltonParams(GraphicsCore::GetWindowWidth(), GraphicsCore::GetWindowHeight());

	m_SwappedHeap.Create(GraphicsCore::GetD3DDevice(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_RtTexDescCount + 128, &pRtScene->m_RtTexDescSet);
}

void CRaytracingSceneRenderHelper::ResetHaltonParams(UINT DispatchSizeX, UINT DispatchSizeY)
{
	Halton_Enum* pHaltonEnum = new Halton_Enum(DispatchSizeX, DispatchSizeY);
	_ASSERTE(pHaltonEnum);
	Halton_Enum_Paramters HaltonEnumParamters;
	pHaltonEnum->dump_halton_enum_paramters(HaltonEnumParamters);
	m_HaltonParamConstantBuffer.Create(L"HaltonParamsConstantBuffer", 1, sizeof(HaltonEnumParamters), &HaltonEnumParamters);
	delete pHaltonEnum;

	Halton_sampler* pHaltonSampler = new Halton_sampler();
	_ASSERTE(pHaltonSampler);
	pHaltonSampler->init_faure();
	std::vector<UINT> perm3(243);
	for (int i = 0; i < 243; i++)
	{
		perm3[i] = pHaltonSampler->m_perm3[i];
	}
	m_HaltonPerm3StructuredBuffer.Create(L"Halton_Perm3", 243, sizeof(UINT), perm3.data());
	delete pHaltonSampler;
}

void CRaytracingSceneRenderHelper::ConfigurateGlobalRootSignature(CRootSignature& rootSignature)
{
	rootSignature.InitStaticSampler(kLinearSampler, 1, GraphicsCommon::Sampler::SamplerLinearWrapDesc);

	UINT SrvsCount = 7;
	rootSignature[kAccelerationStructureSrv].InitAsBufferSRV(kAccelerationStructureSrvRegister, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kRtSceneRenderSrvs].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, kMaterialBufferSrvRegister, SrvsCount, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kDynamicInstanceUpdateFrameIdSrv].InitAsBufferSRV(kDynamicInstanceUpdateFrameIdSrvRegister, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kInstanceWorldSrv].InitAsBufferSRV(kInstanceWorldSrvRegister, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kTextureDescTable].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, kTextureDescTableRegister, m_RtTexDescCount, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kPerSceneCB].InitAsConstantBuffer(kPreSceneCbRegister, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kPerFrameCB].InitAsConstantBuffer(kPerFrameCbRegister, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kHaltonParamCB].InitAsConstantBuffer(kHaltonParamCbRegister, 1, D3D12_SHADER_VISIBILITY_ALL);
}

void CRaytracingSceneRenderHelper::SetGlobalRootParameter(CComputeCommandList* pCommandList, bool setGeometryDataOnly)
{
	_ASSERTE(pCommandList);

	pCommandList->SetBufferSRV(kAccelerationStructureSrvRegister, m_pRtScene->m_RtTLAs[m_ActiveTLAIndex].GetAccelerationStructureBufferData());
	D3D12_CPU_DESCRIPTOR_HANDLE Srvs[7] = {
		m_pRtScene->m_RtMaterialBuffer.GetSRV(),
		m_pRtScene->m_RtMeshDataBuffer.GetSRV(),
		m_pRtScene->m_RtInstanceDataBuffer.GetSRV(),
		m_pRtScene->m_RtLightDataBuffer.GetSRV(),
		m_pRtScene->m_RtVertexBuffer.GetSRV(),
		m_pRtScene->m_RtIndexBuffer.GetSRV(),
		m_HaltonPerm3StructuredBuffer.GetSRV()
	};
	pCommandList->SetDynamicDescriptors(kRtSceneRenderSrvs, 0, 7, Srvs);
	pCommandList->SetDynamicSRV(kDynamicInstanceUpdateFrameIdSrv, sizeof(UINT)*m_pRtScene->m_RtInstanceUpdateFrameIdSet.size(), m_pRtScene->m_RtInstanceUpdateFrameIdSet.data());
	pCommandList->SetDynamicSRV(kInstanceWorldSrv, sizeof(XMMATRIX)*m_pRtScene->m_RtInstanceWorldSet.size(), m_pRtScene->m_RtInstanceWorldSet.data());
	pCommandList->SetConstantBuffer(kPerSceneCB, m_PerSceneConstantBuffer.GetGpuVirtualAddress());
	pCommandList->SetConstantBuffer(kHaltonParamCB, m_HaltonParamConstantBuffer.GetGpuVirtualAddress());

	UINT activeCameraIndex = m_pRtScene->m_ActiveCameraIndex;
	CCameraBasic* pCamera = m_pRtScene->GetCamera(activeCameraIndex);
	_ASSERTE(pCamera);
	m_PerFrameCB.CameraPosition = pCamera->GetPositionXMF3();
	m_PerFrameCB.ViewProj = pCamera->GetViewXMM()*pCamera->GetProjectXMM();
	m_PerFrameCB.InvViewProj = XMMatrixInverse(nullptr, m_PerFrameCB.ViewProj);
	pCamera->DumpJitter(m_PerFrameCB.Jitter.x, m_PerFrameCB.Jitter.y);
	m_PerFrameCB.FrameID = (UINT)GraphicsCore::GetFrameID();
	pCommandList->SetDynamicConstantBufferView(kPerFrameCB, sizeof(m_PerFrameCB), &m_PerFrameCB);

	if (!setGeometryDataOnly)
	{
		CUserDescriptorHeap& userHeap = m_SwappedHeap.GetCurUserHeap();
		pCommandList->SetDynamicViewDescriptorTemporaryUserHeap(userHeap.GetHeapPointer(), userHeap.GetHeapSize(), m_RtTexDescCount);
		pCommandList->SetDescriptorTable(kTextureDescTable, userHeap.GetHandleAtOffset(0).GetGpuHandle());
		CCommandQueue& commandQueue = GraphicsCore::GetCommandQueue(pCommandList->GetType());
		uint64_t fence = commandQueue.IncrementFence();
		m_SwappedHeap.SwapUserHeap(fence);
	}
}

void CRaytracingSceneRenderHelper::BuildHitProgLocalRootSignature(CRootSignature& rootSignature)
{
	_ASSERTE(m_pRtScene);
	m_pRtScene->__BuildHitProgLocalRootSignature(rootSignature);
}

void CRaytracingSceneRenderHelper::BuildMeshHitShaderTable(UINT TLAIndex, CRayTracingPipelineStateObject* pRtPSO, const std::vector<std::wstring>& hitGroupNameSet, CRaytracingShaderTable& outShaderTable)
{
	_ASSERTE(m_pRtScene);
	m_pRtScene->__BuildMeshHitShaderTable(TLAIndex, pRtPSO, hitGroupNameSet, outShaderTable);
}
