#include "HBAOPlusEffect.h"
#include "Include/GFSDK_SSAO.h"
#include "../Core/GpuTimeManager.h"

CHBAOPlus::CHBAOPlus()
{
}

void CHBAOPlus::Initialize(UINT Width, UINT Height, const SHBAOPlusParams& HBAOPlusParams)
{
	m_HBAOParams = HBAOPlusParams;

	auto pDevice = GraphicsCore::GetD3DDevice();
	_ASSERTE(pDevice);

	m_HBAOPlusCBVSRVUAVHeap.Create(pDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, GFSDK_SSAO_NUM_DESCRIPTORS_CBV_SRV_UAV_HEAP_D3D12 + 2, L"HBAOPlusHeap1");
	m_HBAOPlusCBVSRVUAVHeap.RequestFreeHandle(GFSDK_SSAO_NUM_DESCRIPTORS_CBV_SRV_UAV_HEAP_D3D12 + 2);
	m_HBAOPlusRTVHeap.Create(pDevice, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, GFSDK_SSAO_NUM_DESCRIPTORS_RTV_HEAP_D3D12, L"HBAOPlusHeap2");
	m_HBAOPlusRTVHeap.RequestFreeHandle(GFSDK_SSAO_NUM_DESCRIPTORS_RTV_HEAP_D3D12);

	GFSDK_SSAO_DescriptorHeaps_D3D12 GFSDK_SSAO_DescriptorHeaps;
	GFSDK_SSAO_DescriptorHeaps.CBV_SRV_UAV.BaseIndex = 2;
	GFSDK_SSAO_DescriptorHeaps.CBV_SRV_UAV.NumDescriptors = GFSDK_SSAO_NUM_DESCRIPTORS_CBV_SRV_UAV_HEAP_D3D12;
	GFSDK_SSAO_DescriptorHeaps.CBV_SRV_UAV.pDescHeap = m_HBAOPlusCBVSRVUAVHeap.GetHeapPointer();
	GFSDK_SSAO_DescriptorHeaps.RTV.BaseIndex = 0;
	GFSDK_SSAO_DescriptorHeaps.RTV.NumDescriptors = GFSDK_SSAO_NUM_DESCRIPTORS_RTV_HEAP_D3D12;
	GFSDK_SSAO_DescriptorHeaps.RTV.pDescHeap = m_HBAOPlusRTVHeap.GetHeapPointer();
	
	GFSDK_SSAO_CustomHeap CustomHeap;
	CustomHeap.new_ = ::operator new;
	CustomHeap.delete_ = ::operator delete;

	GFSDK_SSAO_Status status;
	status = GFSDK_SSAO_CreateContext_D3D12(pDevice, 1, GFSDK_SSAO_DescriptorHeaps, &m_pAOContext, &CustomHeap);
	_ASSERTE(status == GFSDK_SSAO_OK);

	m_NormMap.Create2D(L"HBAOPlusNorm", Width, Height, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
	m_HBAOPlusRT.Create2D(L"HBAOPlusRT", Width, Height, 1, DXGI_FORMAT_R32_FLOAT);

	m_DecodeNormalEffect.Init();
}

void CHBAOPlus::Cleanup()
{
	SAFE_RELEASE(m_pAOContext);
}

float CHBAOPlus::Render(const XMMATRIX& matProj, const XMMATRIX& matView, CDepthBuffer* pDepthBuffer, CColorBuffer* pNormBuffer, bool debugTimer)
{
	_ASSERTE(pDepthBuffer);
	_ASSERTE(pNormBuffer);

	float Time = 0.0f;

	//
	// Decode Normal
	//
	auto pComputeCommandList = GraphicsCore::BeginComputeCommandList();
	m_DecodeNormalEffect.DecodeNormal(pNormBuffer, &m_NormMap, pComputeCommandList);
	GraphicsCore::FinishCommandList(pComputeCommandList, true);

	//
	// Calc HBAO
	//
	auto pGraphicsCommandList = GraphicsCore::BeginGraphicsCommandList();
	auto& CommandQueue = GraphicsCore::GetCommandQueue(pGraphicsCommandList->GetType());

	if (debugTimer)
	{
		GpuTimeManager::StartTimer(pGraphicsCommandList, 0);
	}

	m_HBAOPlusCBVSRVUAVHeap.ResetHandleInHeap(0, pDepthBuffer->GetDepthSRV());
	m_HBAOPlusCBVSRVUAVHeap.ResetHandleInHeap(1, m_NormMap.GetSRV());

	pGraphicsCommandList->TransitionResource(*pDepthBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pGraphicsCommandList->TransitionResource(m_NormMap, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pGraphicsCommandList->TransitionResource(m_HBAOPlusRT, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	pGraphicsCommandList->ClearColor(m_HBAOPlusRT);

	pGraphicsCommandList->SetViewportAndScissor(GraphicsCore::GetScreenViewport(), GraphicsCore::GetScissorRect());
	pGraphicsCommandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_HBAOPlusCBVSRVUAVHeap.GetHeapPointer());

	GFSDK_SSAO_InputData_D3D12 InputData;
	InputData.DepthData.FullResDepthTextureSRV.GpuHandle = m_HBAOPlusCBVSRVUAVHeap.GetHandleAtOffset(0).GetGpuHandle().ptr;
	InputData.DepthData.FullResDepthTextureSRV.pResource = pDepthBuffer->GetResource();
	InputData.DepthData.DepthTextureType = GFSDK_SSAO_HARDWARE_DEPTHS;
	InputData.DepthData.MetersToViewSpaceUnits = m_HBAOParams.MetersToViewSpaceUnits;
	auto Viewport = GraphicsCore::GetScreenViewport();
	InputData.DepthData.Viewport.Enable = true;
	InputData.DepthData.Viewport.Width = (UINT)Viewport.Width;
	InputData.DepthData.Viewport.Height = (UINT)Viewport.Height;
	InputData.DepthData.Viewport.MinDepth = Viewport.MinDepth;
	InputData.DepthData.Viewport.MaxDepth = Viewport.MaxDepth;
	InputData.DepthData.Viewport.TopLeftX = (UINT)Viewport.TopLeftX;
	InputData.DepthData.Viewport.TopLeftY = (UINT)Viewport.TopLeftY;
	InputData.DepthData.ProjectionMatrix.Data = GFSDK_SSAO_Float4x4((GFSDK_SSAO_FLOAT*)& matProj);
	InputData.DepthData.ProjectionMatrix.Layout = GFSDK_SSAO_ROW_MAJOR_ORDER;
	InputData.NormalData.Enable = true;
	InputData.NormalData.FullResNormalTextureSRV.GpuHandle = m_HBAOPlusCBVSRVUAVHeap.GetHandleAtOffset(1).GetGpuHandle().ptr;
	InputData.NormalData.FullResNormalTextureSRV.pResource = m_NormMap.GetResource();
	InputData.NormalData.WorldToViewMatrix.Data = GFSDK_SSAO_Float4x4((GFSDK_SSAO_FLOAT*)& matView);
	InputData.NormalData.WorldToViewMatrix.Layout = GFSDK_SSAO_ROW_MAJOR_ORDER;
	InputData.NormalData.DecodeScale = 1.0f;
	InputData.NormalData.DecodeBias = 0.0f;

	GFSDK_SSAO_Parameters AOParams;
	AOParams.Radius = m_HBAOParams.Radius;
	AOParams.Bias = m_HBAOParams.Bias;
	AOParams.SmallScaleAO = m_HBAOParams.SmallScaleAO;
	AOParams.LargeScaleAO = m_HBAOParams.LargeScaleAO;
	AOParams.PowerExponent = m_HBAOParams.PowerExponent;
	AOParams.Blur.Enable = m_HBAOParams.EnableBlur;
	AOParams.Blur.Sharpness = m_HBAOParams.BlurSharpness;
	AOParams.Blur.Radius = GFSDK_SSAO_BLUR_RADIUS_4;
	AOParams.NumSteps = m_HBAOParams.NumSteps;

	GFSDK_SSAO_Output_D3D12 Output;
	GFSDK_SSAO_RenderTargetView_D3D12 GFSDK_SSAO_RenderTargetView;
	GFSDK_SSAO_RenderTargetView.CpuHandle = m_HBAOPlusRT.GetRTV().ptr;
	GFSDK_SSAO_RenderTargetView.pResource = m_HBAOPlusRT.GetResource();
	Output.pRenderTargetView = &GFSDK_SSAO_RenderTargetView;
	Output.Blend.Mode = GFSDK_SSAO_OVERWRITE_RGB;

	GFSDK_SSAO_Status Status;
	Status = m_pAOContext->RenderAO(CommandQueue.GetCommandQueue(), pGraphicsCommandList->GetD3D12CommandList(), InputData, AOParams, Output, GFSDK_SSAO_RENDER_AO);
	_ASSERTE(Status == GFSDK_SSAO_OK);

	if (debugTimer)
	{
		GpuTimeManager::StopTimer(pGraphicsCommandList, 0);
	}

	GraphicsCore::FinishCommandList(pGraphicsCommandList, true);

	if (debugTimer)
	{
		GpuTimeManager::BeginReadBack();
		Time = GpuTimeManager::GetTime(0);
		GpuTimeManager::EndReadBack();
	}

	return Time;
}
