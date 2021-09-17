#include "ImageEffect.h"
#include "../Core/GraphicsCore.h"
#include "../Core/GraphicsCommon.h"

#include "CompiledShaders/ImageEffectVS.h"
#include "CompiledShaders/ImageEffectRedPS.h"
#include "CompiledShaders/ImageEffectRgbaPS.h"
#include "CompiledShaders/ImageEffectSsaaRedPS.h"
#include "CompiledShaders/ImageEffectSsaaRgbaPS.h"
#include "CompiledShaders/ImageEffectAccumMapPS.h"
#include "CompiledShaders/ImageEffectDynamicMarkPS.h"

CImageEffect::CImageEffect()
{
}

CImageEffect::~CImageEffect()
{
}

void CImageEffect::Init(float WinScale)
{
	_ASSERTE(WinScale > 0.0f && WinScale <= 1.0f);
	m_Viewport.TopLeftX = (1.0f - WinScale)*GraphicsCore::GetCurrentRenderTarget().GetWidth();
	m_Viewport.TopLeftY = (1.0f - WinScale)*GraphicsCore::GetCurrentRenderTarget().GetHeight();
	m_Viewport.Width = GraphicsCore::GetCurrentRenderTarget().GetWidth()*WinScale;
	m_Viewport.Height = GraphicsCore::GetCurrentRenderTarget().GetHeight()*WinScale;
	m_ScissorRect.left = (LONG)m_Viewport.TopLeftX;
	m_ScissorRect.top = (LONG)m_Viewport.TopLeftY;
	m_ScissorRect.right = (LONG)m_Viewport.TopLeftX + (LONG)m_Viewport.Width;
	m_ScissorRect.bottom = (LONG)m_Viewport.TopLeftY + (LONG)m_Viewport.Height;

	__InitQuadGeomtry();
	__BuildRootSignature();
	__BuildPSOs();
}

void CImageEffect::Init(float WinScale, float TopLeftXOffset, float TopLeftYOffset)
{
	_ASSERTE(WinScale > 0.0f && WinScale <= 1.0f);
	_ASSERTE(TopLeftXOffset > 0.0f && TopLeftXOffset <= 1.0f);
	_ASSERTE(TopLeftYOffset > 0.0f && TopLeftYOffset <= 1.0f);
	m_Viewport.TopLeftX = TopLeftXOffset * GraphicsCore::GetCurrentRenderTarget().GetWidth();
	m_Viewport.TopLeftY = TopLeftYOffset * GraphicsCore::GetCurrentRenderTarget().GetHeight();
	m_Viewport.Width = GraphicsCore::GetCurrentRenderTarget().GetWidth() * WinScale;
	m_Viewport.Height = GraphicsCore::GetCurrentRenderTarget().GetHeight() * WinScale;
	m_ScissorRect.left = (LONG)m_Viewport.TopLeftX;
	m_ScissorRect.top = (LONG)m_Viewport.TopLeftY;
	m_ScissorRect.right = (LONG)m_Viewport.TopLeftX + (LONG)m_Viewport.Width;
	m_ScissorRect.bottom = (LONG)m_Viewport.TopLeftY + (LONG)m_Viewport.Height;

	__InitQuadGeomtry();
	__BuildRootSignature();
	__BuildPSOs();
}

void CImageEffect::Render(EImageEffectMode Mode, CColorBuffer* pSRV, CGraphicsCommandList* pGraphicsCommandList)
{
	_ASSERTE(pSRV && pGraphicsCommandList);

	pGraphicsCommandList->TransitionResource(*pSRV, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pGraphicsCommandList->TransitionResource(GraphicsCore::GetCurrentRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	pGraphicsCommandList->SetViewportAndScissor(m_Viewport, m_ScissorRect);
	switch (Mode)
	{
	case kRGBA:
	{
		pGraphicsCommandList->SetPipelineState(m_PsoRgba);
		pGraphicsCommandList->SetRootSignature(m_RootSignatureGeneral);
		break;
	}
	case kRED:
	{
		pGraphicsCommandList->SetPipelineState(m_PsoRed);
		pGraphicsCommandList->SetRootSignature(m_RootSignatureGeneral);
		break;
	}
	case kAccumMap:
	{
		pGraphicsCommandList->SetPipelineState(m_PsoAccumCount);
		pGraphicsCommandList->SetRootSignature(m_RootSignatureAccumCount);
		break;
	}
	case kDynamicMark:
	{
		pGraphicsCommandList->SetPipelineState(m_PsoDynamicMark);
		pGraphicsCommandList->SetRootSignature(m_RootSignatureDynamicMark);
		break;
	}
	default:_ASSERTE(false);
	}	
	pGraphicsCommandList->SetRenderTarget(GraphicsCore::GetCurrentRenderTarget().GetRTV());
	pGraphicsCommandList->SetVertexBuffer(0, m_QuadVertexBufferView);
	pGraphicsCommandList->SetIndexBuffer(m_QuadIndexBufferView);
	pGraphicsCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pGraphicsCommandList->SetDynamicDescriptor(0, 0, pSRV->GetSRV());
	pGraphicsCommandList->DrawIndexed(6, 0, 0);
}

void CImageEffect::RenderSsaa(EImageEffectMode Mode, CColorBuffer* pSRV, CGraphicsCommandList* pGraphicsCommandList)
{
	_ASSERTE(pSRV && pGraphicsCommandList);

	pGraphicsCommandList->TransitionResource(*pSRV, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pGraphicsCommandList->TransitionResource(GraphicsCore::GetCurrentRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	pGraphicsCommandList->SetViewportAndScissor(m_Viewport, m_ScissorRect);
	switch (Mode)
	{
	case kRGBA:pGraphicsCommandList->SetPipelineState(m_PsoSsaaRgba); break;
	case kRED:pGraphicsCommandList->SetPipelineState(m_PsoSsaaRed); break;
	default:_ASSERTE(false);
	}
	pGraphicsCommandList->SetRootSignature(m_RootSignatureSsaa);
	pGraphicsCommandList->SetRenderTarget(GraphicsCore::GetCurrentRenderTarget().GetRTV());
	pGraphicsCommandList->SetVertexBuffer(0, m_QuadVertexBufferView);
	pGraphicsCommandList->SetIndexBuffer(m_QuadIndexBufferView);
	pGraphicsCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	__UpdateSsaaCB(pSRV);
	pGraphicsCommandList->SetDynamicConstantBufferView(0, sizeof(m_SsaaCB), &m_SsaaCB);
	pGraphicsCommandList->SetDynamicDescriptor(1, 0, pSRV->GetSRV());
	pGraphicsCommandList->DrawIndexed(6, 0, 0);
}

void CImageEffect::__InitQuadGeomtry()
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

void CImageEffect::__BuildRootSignature()
{
	m_RootSignatureGeneral.Reset(1, 1);
	m_RootSignatureGeneral.InitStaticSampler(0, 0, GraphicsCommon::Sampler::SamplerLinearWrapDesc, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignatureGeneral[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignatureGeneral.Finalize(L"ImageEffectRootSignatureGeneral", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	m_RootSignatureSsaa.Reset(2, 1);
	m_RootSignatureSsaa.InitStaticSampler(0, 0, GraphicsCommon::Sampler::SamplerPointClampDesc, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignatureSsaa[0].InitAsConstantBuffer(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignatureSsaa[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignatureSsaa.Finalize(L"ImageEffectRootSignatureSsaa", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	m_RootSignatureAccumCount.Reset(1, 0);
	m_RootSignatureAccumCount[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignatureAccumCount.Finalize(L"ImageEffectRootSignatureAccumFlag", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	m_RootSignatureDynamicMark.Reset(1, 0);
	m_RootSignatureDynamicMark[0].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignatureDynamicMark.Finalize(L"ImageEffectRootSignatureAccumFlag", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
}

void CImageEffect::__BuildPSOs()
{
	m_PsoRgba.SetInputLayout((UINT)GraphicsCommon::InputLayout::PosTex.size(), GraphicsCommon::InputLayout::PosTex.data());
	m_PsoRgba.SetRootSignature(m_RootSignatureGeneral);
	D3D12_RASTERIZER_DESC DisableDepthTest = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	DisableDepthTest.DepthClipEnable = FALSE;
	m_PsoRgba.SetRasterizerState(DisableDepthTest);
	m_PsoRgba.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
	m_PsoRgba.SetSampleMask(UINT_MAX);
	m_PsoRgba.SetVertexShader(g_pImageEffectVS, sizeof(g_pImageEffectVS));
	m_PsoRgba.SetPixelShader(g_pImageEffectRgbaPS, sizeof(g_pImageEffectRgbaPS));
	m_PsoRgba.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	m_PsoRgba.SetRenderTargetFormat(GraphicsCore::GetCurrentRenderTarget().GetFormat(), DXGI_FORMAT_UNKNOWN);
	m_PsoRgba.Finalize();

	m_PsoRed = m_PsoRgba;
	m_PsoRed.SetRootSignature(m_RootSignatureGeneral);
	m_PsoRed.SetPixelShader(g_pImageEffectRedPS, sizeof(g_pImageEffectRedPS));
	m_PsoRed.Finalize();

	m_PsoSsaaRed = m_PsoRgba;
	m_PsoSsaaRed.SetRootSignature(m_RootSignatureSsaa);
	m_PsoSsaaRed.SetPixelShader(g_pImageEffectSsaaRedPS, sizeof(g_pImageEffectSsaaRedPS));
	m_PsoSsaaRed.Finalize();

	m_PsoSsaaRgba = m_PsoRgba;
	m_PsoSsaaRgba.SetRootSignature(m_RootSignatureSsaa);
	m_PsoSsaaRgba.SetPixelShader(g_pImageEffectSsaaRgbaPS, sizeof(g_pImageEffectSsaaRgbaPS));
	m_PsoSsaaRgba.Finalize();

	m_PsoAccumCount = m_PsoRgba;
	m_PsoAccumCount.SetRootSignature(m_RootSignatureAccumCount);
	m_PsoAccumCount.SetPixelShader(g_pImageEffectAccumMapPS, sizeof(g_pImageEffectAccumMapPS));
	m_PsoAccumCount.Finalize();

	m_PsoDynamicMark = m_PsoRgba;
	m_PsoDynamicMark.SetRootSignature(m_RootSignatureDynamicMark);
	m_PsoDynamicMark.SetPixelShader(g_pImageEffectDynamicMarkPS, sizeof(g_pImageEffectDynamicMarkPS));
	m_PsoDynamicMark.Finalize();
}

void CImageEffect::__UpdateSsaaCB(CColorBuffer* pSRV)
{
	_ASSERTE(pSRV);
	UINT Multiple = pSRV->GetWidth() / GraphicsCore::GetCurrentRenderTarget().GetWidth();
	_ASSERTE(Multiple > 1 && Multiple == pSRV->GetHeight() / GraphicsCore::GetCurrentRenderTarget().GetHeight());
	m_SsaaCB.HasCenter = Multiple % 2;
	m_SsaaCB.SsaaRadius = Multiple / 2;
	m_SsaaCB.SrvWidth = pSRV->GetWidth();
	m_SsaaCB.SrvHeight = pSRV->GetHeight();
}
