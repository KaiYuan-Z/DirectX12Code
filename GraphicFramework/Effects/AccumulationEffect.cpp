#include "../Core/GraphicsCommon.h"
#include "AccumulationEffect.h"

#include "CompiledShaders/AccumulationEffectVS.h"
#include "CompiledShaders/AccumulationEffectAccumMapPS.h"
#include "CompiledShaders/AccumulationEffectAccumRedPS.h"


CAccumulationEffect::CAccumulationEffect()
{
}

CAccumulationEffect::~CAccumulationEffect()
{
}

void CAccumulationEffect::Init(DXGI_FORMAT frameFormat, UINT frameWidth, UINT frameHeight)
{
	_ASSERTE(frameWidth > 0 && frameHeight > 0);

	m_FrameFormat = frameFormat;

	m_Viewport.TopLeftX = m_Viewport.TopLeftY = 0;
	m_Viewport.Width = (float)frameWidth;
	m_Viewport.Height = (float)frameHeight;

	m_ScissorRect.left = (LONG)m_Viewport.TopLeftX;
	m_ScissorRect.top = (LONG)m_Viewport.TopLeftY;
	m_ScissorRect.right = (LONG)m_Viewport.TopLeftX + (LONG)m_Viewport.Width;
	m_ScissorRect.bottom = (LONG)m_Viewport.TopLeftY + (LONG)m_Viewport.Height;

	__InitQuadGeomtry();
	__BuildRootSignature();
	__BuildPSO();
	__BuildMaps(frameWidth, frameHeight);
}

void CAccumulationEffect::OnFrameUpdate(const CCameraBasic* pCamera)
{
	m_AccumulationMapCB.ViewUpdateFlag = pCamera->GetLastUpdateFrameID() == (UINT)GraphicsCore::GetFrameID();
	m_DoubleFrameMap.SwapBuffer();
	m_DoubleAccumMap.SwapBuffer();
}

void CAccumulationEffect::CalculateAccumMap(CColorBuffer* pPreLinearDepthBuffer, CColorBuffer* pCurLinearDepthBuffer, CColorBuffer* pMotionBuffer, CGraphicsCommandList * pGraphicsCommandList)
{
	_ASSERTE(pGraphicsCommandList);
	__VerifySRV(pPreLinearDepthBuffer);
	__VerifySRV(pCurLinearDepthBuffer);
	__VerifySRV(pMotionBuffer);

	pGraphicsCommandList->TransitionResource(*pPreLinearDepthBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pGraphicsCommandList->TransitionResource(*pCurLinearDepthBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pGraphicsCommandList->TransitionResource(*pMotionBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pGraphicsCommandList->TransitionResource(m_DoubleAccumMap.GetBackBuffer(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pGraphicsCommandList->TransitionResource(m_DoubleAccumMap.GetFrontBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	pGraphicsCommandList->ClearColor(m_DoubleAccumMap.GetFrontBuffer());

	pGraphicsCommandList->SetViewportAndScissor(m_Viewport, m_ScissorRect);
	pGraphicsCommandList->SetPipelineState(m_PsoAccumMap);
	pGraphicsCommandList->SetRootSignature(m_RootSignatureAccumMap);
	pGraphicsCommandList->SetRenderTarget(m_DoubleAccumMap.GetFrontBuffer().GetRTV());
	pGraphicsCommandList->SetVertexBuffer(0, m_QuadVertexBufferView);
	pGraphicsCommandList->SetIndexBuffer(m_QuadIndexBufferView);
	pGraphicsCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pGraphicsCommandList->SetDynamicConstantBufferView(0, sizeof(m_AccumulationMapCB), &m_AccumulationMapCB);
	pGraphicsCommandList->SetDynamicDescriptor(1, 0, pPreLinearDepthBuffer->GetSRV());
	pGraphicsCommandList->SetDynamicDescriptor(2, 0, pCurLinearDepthBuffer->GetSRV());
	pGraphicsCommandList->SetDynamicDescriptor(3, 0, pMotionBuffer->GetSRV());
	pGraphicsCommandList->SetDynamicDescriptor(4, 0, m_DoubleAccumMap.GetBackBuffer().GetSRV());	
	pGraphicsCommandList->DrawIndexed(6, 0, 0);
}

void CAccumulationEffect::AccumulateFrame(CColorBuffer* pCurFrameBuffer, CColorBuffer* pMotionBuffer, CGraphicsCommandList* pGraphicsCommandList)
{
	_ASSERTE(pGraphicsCommandList);
	__VerifySRV(pCurFrameBuffer);
	__VerifySRV(pMotionBuffer);

	pGraphicsCommandList->TransitionResource(m_DoubleFrameMap.GetBackBuffer(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pGraphicsCommandList->TransitionResource(*pCurFrameBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pGraphicsCommandList->TransitionResource(*pMotionBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pGraphicsCommandList->TransitionResource(m_DoubleAccumMap.GetFrontBuffer(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pGraphicsCommandList->TransitionResource(m_DoubleFrameMap.GetFrontBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	pGraphicsCommandList->ClearColor(m_DoubleFrameMap.GetFrontBuffer());

	pGraphicsCommandList->SetViewportAndScissor(m_Viewport, m_ScissorRect);
	pGraphicsCommandList->SetPipelineState(m_PsoAccumRed);
	pGraphicsCommandList->SetRootSignature(m_RootSignatureAccumRed);
	pGraphicsCommandList->SetRenderTarget(m_DoubleFrameMap.GetFrontBuffer().GetRTV());
	pGraphicsCommandList->SetVertexBuffer(0, m_QuadVertexBufferView);
	pGraphicsCommandList->SetIndexBuffer(m_QuadIndexBufferView);
	pGraphicsCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pGraphicsCommandList->SetDynamicDescriptor(0, 0, m_DoubleFrameMap.GetBackBuffer().GetSRV());
	pGraphicsCommandList->SetDynamicDescriptor(1, 0, pCurFrameBuffer->GetSRV());
	pGraphicsCommandList->SetDynamicDescriptor(2, 0, pMotionBuffer->GetSRV());
	pGraphicsCommandList->SetDynamicDescriptor(3, 0, m_DoubleAccumMap.GetFrontBuffer().GetSRV());
	pGraphicsCommandList->DrawIndexed(6, 0, 0);
}

void CAccumulationEffect::__InitQuadGeomtry()
{
	std::vector<GraphicsCommon::Vertex::SPosTex> QuadVertex = {
	{{-1.0f, -1.0f, 0.0f},{0.0f, 1.0f}},
	{{-1.0f,  1.0f, 0.0f},{0.0f, 0.0f}},
	{{1.0f,  1.0f, 0.0f},{1.0f, 0.0f}},
	{{1.0f, -1.0f, 0.0f},{1.0f, 1.0f}}
	};
	m_QuadVertexBuffer.Create(L"Quad Vertex Buffer", 4, sizeof(GraphicsCommon::Vertex::SPosTex), QuadVertex.data());
	m_QuadVertexBufferView = m_QuadVertexBuffer.CreateVertexBufferView();

	std::vector<uint16_t> QuadIndex = { 0, 1, 2, 0, 2, 3 };
	m_QuadIndexBuffer.Create(L"Quad Index Buffer", 6, sizeof(uint16_t), QuadIndex.data());
	m_QuadIndexBufferView = m_QuadIndexBuffer.CreateIndexBufferView();
}

void CAccumulationEffect::__BuildRootSignature()
{
	m_RootSignatureAccumMap.Reset(5, 2);
	m_RootSignatureAccumMap.InitStaticSampler(0, 0, GraphicsCommon::Sampler::SamplerPointBorderDesc);// g_samplerPointBorder(s0)
	m_RootSignatureAccumMap.InitStaticSampler(1, 0, GraphicsCommon::Sampler::SamplerLinearBorderDesc);// g_samplerLinearBorder(s1)
	m_RootSignatureAccumMap[0].InitAsConstantBuffer(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);// cb_AccumMap(b0)
	m_RootSignatureAccumMap[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);// g_prePosMap(t0)
	m_RootSignatureAccumMap[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);// g_curPosMap(t1)
	m_RootSignatureAccumMap[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);// g_motionVecMap(t2)
	m_RootSignatureAccumMap[4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);// g_preAccumMap(t3)
	m_RootSignatureAccumMap.Finalize(L"AccumulationEffect-RootSignatureAccumMap", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	m_RootSignatureAccumRed.Reset(4, 2);
	m_RootSignatureAccumRed.InitStaticSampler(0, 0, GraphicsCommon::Sampler::SamplerPointBorderDesc);// g_samplerPointBorder(s0)
	m_RootSignatureAccumRed.InitStaticSampler(1, 0, GraphicsCommon::Sampler::SamplerLinearBorderDesc);// g_samplerLinearBorder(s1)
	m_RootSignatureAccumRed[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);// g_preFrame(t0)
	m_RootSignatureAccumRed[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);// g_curFrame(t1)
	m_RootSignatureAccumRed[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);// g_motionVecMap(t1)
	m_RootSignatureAccumRed[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);// g_curAccumMap(t2)
	m_RootSignatureAccumRed.Finalize(L"AccumulationEffect-RootSignatureAccumRed", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
}

void CAccumulationEffect::__BuildPSO()
{
	m_PsoAccumMap.SetInputLayout((UINT)GraphicsCommon::InputLayout::PosTex.size(), GraphicsCommon::InputLayout::PosTex.data());
	m_PsoAccumMap.SetRootSignature(m_RootSignatureAccumMap);
	m_PsoAccumMap.SetVertexShader(g_pAccumulationEffectVS, sizeof(g_pAccumulationEffectVS));
	m_PsoAccumMap.SetPixelShader(g_pAccumulationEffectAccumMapPS, sizeof(g_pAccumulationEffectAccumMapPS));
	D3D12_RASTERIZER_DESC DisableDepthTest = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	DisableDepthTest.DepthClipEnable = FALSE;
	m_PsoAccumMap.SetRasterizerState(DisableDepthTest);
	m_PsoAccumMap.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
	m_PsoAccumMap.SetSampleMask(UINT_MAX);
	m_PsoAccumMap.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	m_PsoAccumMap.SetRenderTargetFormat(DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_UNKNOWN);
	m_PsoAccumMap.Finalize();

	m_PsoAccumRed = m_PsoAccumMap;
	m_PsoAccumRed.SetRootSignature(m_RootSignatureAccumRed);
	switch (m_FrameFormat)
	{
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R16_FLOAT:
		m_PsoAccumRed.SetPixelShader(g_pAccumulationEffectAccumRedPS, sizeof(g_pAccumulationEffectAccumRedPS));
		break;
	default:_ASSERTE(false);
	}
	m_PsoAccumRed.SetRenderTargetFormat(m_FrameFormat, DXGI_FORMAT_UNKNOWN);
	m_PsoAccumRed.Finalize();
}

void CAccumulationEffect::__BuildMaps(UINT frameWidth, UINT frameHeight)
{
	m_DoubleFrameMap.Create2D(L"DoubleFrameMap", frameWidth, frameHeight, 1, m_FrameFormat);

	m_DoubleAccumMap.Create2D(L"DoubleAccumMap", frameWidth, frameHeight, 1, DXGI_FORMAT_R32G32B32A32_FLOAT);
	CGraphicsCommandList* pClearCommandList = GraphicsCore::BeginGraphicsCommandList();
	_ASSERTE(pClearCommandList);
	pClearCommandList->TransitionResource(m_DoubleAccumMap.GetFrontBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, false);
	pClearCommandList->TransitionResource(m_DoubleAccumMap.GetBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	pClearCommandList->ClearColor(m_DoubleAccumMap.GetFrontBuffer());
	pClearCommandList->ClearColor(m_DoubleAccumMap.GetBackBuffer());
	GraphicsCore::FinishCommandList(pClearCommandList, true);
}

void CAccumulationEffect::__VerifySRV(const CPixelBuffer* pSRV)
{
	_ASSERTE(pSRV && m_Viewport.Width == pSRV->GetWidth() && m_Viewport.Height == pSRV->GetHeight());
}