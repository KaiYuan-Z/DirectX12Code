#include "FxaaEffect.h"
#include "CompiledShaders/FxaaEffectVS.h"
#include "CompiledShaders/FxaaEffectPS.h"


CFxaaEffect::CFxaaEffect()
{
}

CFxaaEffect::~CFxaaEffect()
{
}

void CFxaaEffect::Init(DXGI_FORMAT colorBufferFormat)
{
	__InitQuadGeomtry();
	__BuildRootSignature();
	__BuildPSO(colorBufferFormat);
}

void CFxaaEffect::RenderFxaa(CColorBuffer* pInColorBuffer, CColorBuffer* pOutColorBuffer, CGraphicsCommandList* pGraphicsCommandList)
{
	_ASSERTE(pInColorBuffer);
	_ASSERTE(pOutColorBuffer);
	_ASSERTE(pGraphicsCommandList);

	D3D12_VIEWPORT viewPort;
	D3D12_RECT scissorRect;
	viewPort.TopLeftX = viewPort.TopLeftY = 0;
	viewPort.Width = (float)pInColorBuffer->GetWidth();
	viewPort.Height = (float)pInColorBuffer->GetHeight();
	viewPort.MinDepth = D3D12_MIN_DEPTH;
	viewPort.MaxDepth = D3D12_MAX_DEPTH;
	scissorRect.left = (LONG)viewPort.TopLeftX;
	scissorRect.top = (LONG)viewPort.TopLeftY;
	scissorRect.right = (LONG)viewPort.TopLeftX + (LONG)viewPort.Width;
	scissorRect.bottom = (LONG)viewPort.TopLeftY + (LONG)viewPort.Height;

	m_ParamtersCB.InvTexDim = XMFLOAT2(1.0f / (float)pInColorBuffer->GetWidth(), 1.0f / (float)pInColorBuffer->GetHeight());

	pGraphicsCommandList->TransitionResource(*pInColorBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pGraphicsCommandList->TransitionResource(*pOutColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	pGraphicsCommandList->ClearColor(*pOutColorBuffer);
	pGraphicsCommandList->SetViewportAndScissor(viewPort, scissorRect);
	pGraphicsCommandList->SetPipelineState(m_PSO);
	pGraphicsCommandList->SetRootSignature(m_RootSignature);
	pGraphicsCommandList->SetRenderTarget(pOutColorBuffer->GetRTV());
	pGraphicsCommandList->SetVertexBuffer(0, m_QuadVertexBufferView);
	pGraphicsCommandList->SetIndexBuffer(m_QuadIndexBufferView);
	pGraphicsCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pGraphicsCommandList->SetDynamicConstantBufferView(0, sizeof(m_ParamtersCB), &m_ParamtersCB);
	D3D12_CPU_DESCRIPTOR_HANDLE Srvs[1] = {
		pInColorBuffer->GetSRV()
	};
	pGraphicsCommandList->SetDynamicDescriptors(1, 0, 1, Srvs);
	pGraphicsCommandList->DrawIndexed(6, 0, 0);
}

void CFxaaEffect::__InitQuadGeomtry()
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

void CFxaaEffect::__BuildRootSignature()
{
	m_RootSignature.Reset(2, 1);
	m_RootSignature.InitStaticSampler(0, 0, GraphicsCommon::Sampler::SamplerLinearClampDesc, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature[0].InitAsConstantBuffer(0, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature.Finalize(L"TemporalEffect", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
}

void CFxaaEffect::__BuildPSO(DXGI_FORMAT colorBufferFormat)
{
	_ASSERTE(colorBufferFormat == DXGI_FORMAT_R32G32B32A32_FLOAT || colorBufferFormat == DXGI_FORMAT_R16G16B16A16_FLOAT);
	
	m_PSO.SetInputLayout((UINT)GraphicsCommon::InputLayout::PosTex.size(), GraphicsCommon::InputLayout::PosTex.data());
	m_PSO.SetRootSignature(m_RootSignature);
	D3D12_RASTERIZER_DESC DisableDepthTest = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	DisableDepthTest.DepthClipEnable = FALSE;
	m_PSO.SetRasterizerState(DisableDepthTest);
	m_PSO.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
	m_PSO.SetSampleMask(UINT_MAX);
	m_PSO.SetVertexShader(g_pFxaaEffectVS, sizeof(g_pFxaaEffectVS));
	m_PSO.SetPixelShader(g_pFxaaEffectPS, sizeof(g_pFxaaEffectPS));
	m_PSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	m_PSO.SetRenderTargetFormat(colorBufferFormat, DXGI_FORMAT_UNKNOWN);
	m_PSO.Finalize();
}