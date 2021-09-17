#include "TemporalEffect.h"
#include "CompiledShaders/TAAEffectVS.h"
#include "CompiledShaders/TAAEffectPS.h"


CTemporalEffect::CTemporalEffect()
{
}

CTemporalEffect::~CTemporalEffect()
{
}

void CTemporalEffect::Init(UINT width, UINT height, CCameraBasic* pHostCamera)
{
	_ASSERTE(width > 0 && height > 0);
	m_WinWidth = width;
	m_WinHeight = height;
	_ASSERTE(pHostCamera);
	m_pHostCamera = pHostCamera;

	m_ViewPort.TopLeftX = m_ViewPort.TopLeftY = 0;
	m_ViewPort.Width = (float)width;
	m_ViewPort.Height = (float)height;
	m_ViewPort.MinDepth = D3D12_MIN_DEPTH;
	m_ViewPort.MaxDepth = D3D12_MAX_DEPTH;
	m_ScissorRect.left = (LONG)m_ViewPort.TopLeftX;
	m_ScissorRect.top = (LONG)m_ViewPort.TopLeftY;
	m_ScissorRect.right = (LONG)m_ViewPort.TopLeftX + (LONG)m_ViewPort.Width;
	m_ScissorRect.bottom = (LONG)m_ViewPort.TopLeftY + (LONG)m_ViewPort.Height;

	m_AccumColorBuffer.Create2D(L"TAA-AccumColorBuffer", width, height, 1, DXGI_FORMAT_R32G32B32A32_FLOAT);

	__InitQuadGeomtry();
	__BuildRootSignature();
	__BuildPSO();

	// Register GUI Callbak Function
	GraphicsCore::RegisterGuiCallbakFunction(std::bind(&CTemporalEffect::__UpdateGUI, this));
}

void CTemporalEffect::OnFrameUpdate(UINT frameID)
{
	_ASSERTE(m_pHostCamera);
	float xOff = m_MSAA[frameID % 8][0] * 0.0625f;
	float yOff = m_MSAA[frameID % 8][1] * 0.0625f;
	m_pHostCamera->SetJitter(xOff/float(m_WinWidth), yOff/float(m_WinHeight));
}

void CTemporalEffect::AccumFrame(CColorBuffer* pCurColorBuffer, CColorBuffer* pCurMotionBuffer, CGraphicsCommandList* pGraphicsCommandList)
{
	_ASSERTE(pCurColorBuffer);
	_ASSERTE(pCurMotionBuffer);
	_ASSERTE(pGraphicsCommandList);

	m_AccumColorBuffer.SwapBuffer();

	pGraphicsCommandList->TransitionResource(*pCurColorBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pGraphicsCommandList->TransitionResource(*pCurMotionBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pGraphicsCommandList->TransitionResource(m_AccumColorBuffer.GetFrontBuffer(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pGraphicsCommandList->TransitionResource(m_AccumColorBuffer.GetBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	pGraphicsCommandList->ClearColor(m_AccumColorBuffer.GetBackBuffer());
	pGraphicsCommandList->SetViewportAndScissor(m_ViewPort, m_ScissorRect);
	pGraphicsCommandList->SetPipelineState(m_PSO);
	pGraphicsCommandList->SetRootSignature(m_RootSignature);
	pGraphicsCommandList->SetRenderTarget(m_AccumColorBuffer.GetBackBuffer().GetRTV());
	pGraphicsCommandList->SetVertexBuffer(0, m_QuadVertexBufferView);
	pGraphicsCommandList->SetIndexBuffer(m_QuadIndexBufferView);
	pGraphicsCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pGraphicsCommandList->SetDynamicConstantBufferView(0, sizeof(m_PerFrameCB), &m_PerFrameCB);
	D3D12_CPU_DESCRIPTOR_HANDLE Srvs[3] = {
		pCurColorBuffer->GetSRV(),
		pCurMotionBuffer->GetSRV(), 
		m_AccumColorBuffer.GetFrontBuffer().GetSRV()
	};
	pGraphicsCommandList->SetDynamicDescriptors(1, 0, 3, Srvs);
	pGraphicsCommandList->DrawIndexed(6, 0, 0);
}

void CTemporalEffect::__InitQuadGeomtry()
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

void CTemporalEffect::__BuildRootSignature()
{
	m_RootSignature.Reset(2, 1);
	m_RootSignature.InitStaticSampler(0, 0, GraphicsCommon::Sampler::SamplerPointClampDesc, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature[0].InitAsConstantBuffer(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 3, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature.Finalize(L"TemporalEffect", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
}

void CTemporalEffect::__BuildPSO()
{
	m_PSO.SetInputLayout((UINT)GraphicsCommon::InputLayout::PosTex.size(), GraphicsCommon::InputLayout::PosTex.data());
	m_PSO.SetRootSignature(m_RootSignature);
	D3D12_RASTERIZER_DESC DisableDepthTest = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	DisableDepthTest.DepthClipEnable = FALSE;
	m_PSO.SetRasterizerState(DisableDepthTest);
	m_PSO.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
	m_PSO.SetSampleMask(UINT_MAX);
	m_PSO.SetVertexShader(g_pTAAEffectVS, sizeof(g_pTAAEffectVS));
	m_PSO.SetPixelShader(g_pTAAEffectPS, sizeof(g_pTAAEffectPS));
	m_PSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	m_PSO.SetRenderTargetFormat(DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_UNKNOWN);
	m_PSO.Finalize();
}

void CTemporalEffect::__UpdateGUI()
{
}
