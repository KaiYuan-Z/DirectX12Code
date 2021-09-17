#include "SimpleAccumulationEffect.h"
#include "../Core/GraphicsCommon.h"

#include "CompiledShaders/SimpleAccumulationEffectRedVS.h"
#include "CompiledShaders/SimpleAccumulationEffectRedPS.h"


CSimpleAccumulationEffect::CSimpleAccumulationEffect()
{
}

CSimpleAccumulationEffect::~CSimpleAccumulationEffect()
{
}

void CSimpleAccumulationEffect::Init(DXGI_FORMAT frameFormat, UINT frameWidth, UINT frameHeight, UINT maxAccumCount)
{
	m_FrameFormat = frameFormat;

	_ASSERTE(frameWidth > 0 && frameHeight > 0 && maxAccumCount > 0);

	m_Viewport.TopLeftX = m_Viewport.TopLeftY = 0;
	m_Viewport.Width = (float)frameWidth;
	m_Viewport.Height = (float)frameHeight;

	m_ScissorRect.left = (LONG)m_Viewport.TopLeftX;
	m_ScissorRect.top = (LONG)m_Viewport.TopLeftY;
	m_ScissorRect.right = (LONG)m_Viewport.TopLeftX + (LONG)m_Viewport.Width;
	m_ScissorRect.bottom = (LONG)m_Viewport.TopLeftY + (LONG)m_Viewport.Height;

	m_MaxAccumCount = maxAccumCount;

	__InitQuadGeomtry();
	__BuildRootSignature();
	__BuildPSO();
}

void CSimpleAccumulationEffect::Render(CColorBuffer* pLastFrame, CColorBuffer* pCurFrame, CColorBuffer* pOutputTarget, CGraphicsCommandList* pGraphicsCommandList)
{
	_ASSERTE(pLastFrame && m_Viewport.Width == pLastFrame->GetWidth() && m_Viewport.Height == pLastFrame->GetHeight() && pLastFrame->GetFormat() == m_FrameFormat);
	_ASSERTE(pCurFrame && m_Viewport.Width == pCurFrame->GetWidth() && m_Viewport.Height == pCurFrame->GetHeight() && pCurFrame->GetFormat() == m_FrameFormat);
	_ASSERTE(pOutputTarget && m_Viewport.Width == pOutputTarget->GetWidth() && m_Viewport.Height == pOutputTarget->GetHeight() && pOutputTarget->GetFormat() == m_FrameFormat);
	_ASSERTE(pGraphicsCommandList);

	pGraphicsCommandList->TransitionResource(*pLastFrame, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pGraphicsCommandList->TransitionResource(*pCurFrame, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pGraphicsCommandList->TransitionResource(*pOutputTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	pGraphicsCommandList->ClearColor(*pOutputTarget);
	pGraphicsCommandList->SetViewportAndScissor(m_Viewport, m_ScissorRect);
	pGraphicsCommandList->SetPipelineState(m_Pso);
	pGraphicsCommandList->SetRootSignature(m_RootSignature);
	pGraphicsCommandList->SetRenderTarget(pOutputTarget->GetRTV());
	pGraphicsCommandList->SetVertexBuffer(0, m_QuadVertexBufferView);
	pGraphicsCommandList->SetIndexBuffer(m_QuadIndexBufferView);
	pGraphicsCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pGraphicsCommandList->SetConstant(0, m_AccumulationCB.AccumCount);
	pGraphicsCommandList->SetDynamicDescriptor(1, 0, pLastFrame->GetSRV());
	pGraphicsCommandList->SetDynamicDescriptor(2, 0, pCurFrame->GetSRV());
	pGraphicsCommandList->DrawIndexed(6, 0, 0);

	if(m_AccumulationCB.AccumCount < m_MaxAccumCount) m_AccumulationCB.AccumCount++;
}

void CSimpleAccumulationEffect::__InitQuadGeomtry()
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

void CSimpleAccumulationEffect::__BuildRootSignature()
{
	m_RootSignature.Reset(3, 1);
	m_RootSignature.InitStaticSampler(0, 0, GraphicsCommon::Sampler::SamplerPointClampDesc, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature[0].InitAsConstants(0, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature.Finalize(L"SimpleAccumulationEffectRootSignature", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
}

void CSimpleAccumulationEffect::__BuildPSO()
{
	m_Pso.SetInputLayout((UINT)GraphicsCommon::InputLayout::PosTex.size(), GraphicsCommon::InputLayout::PosTex.data());
	m_Pso.SetRootSignature(m_RootSignature);
	D3D12_RASTERIZER_DESC DisableDepthTest = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	DisableDepthTest.DepthClipEnable = FALSE;
	m_Pso.SetRasterizerState(DisableDepthTest);
	m_Pso.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
	m_Pso.SetSampleMask(UINT_MAX);
	switch (m_FrameFormat)
	{
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R16_FLOAT:
		m_Pso.SetVertexShader(g_pSimpleAccumulationEffectRedVS, sizeof(g_pSimpleAccumulationEffectRedVS));
		m_Pso.SetPixelShader(g_pSimpleAccumulationEffectRedPS, sizeof(g_pSimpleAccumulationEffectRedPS));
		break;
	default:_ASSERTE(false);
	}
	m_Pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	m_Pso.SetRenderTargetFormat(m_FrameFormat, DXGI_FORMAT_UNKNOWN);
	m_Pso.Finalize();
}