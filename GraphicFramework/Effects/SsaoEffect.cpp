#include "SsaoEffect.h"
#include "../Core/GraphicsCommon.h"

#include "CompiledShaders/SsaoEffectVS.h"
#include "CompiledShaders/SsaoEffectPS.h"

CSsaoEffect::CSsaoEffect()
{
}

CSsaoEffect::~CSsaoEffect()
{
}

void CSsaoEffect::Init(DXGI_FORMAT SsaoMapFormat, UINT SsaoMapWidth, UINT SsaoMapHeight, bool UseTemporalRandom)
{
	m_SsaoMapFormat = SsaoMapFormat;
	m_UseTemporalRandom = UseTemporalRandom;

	m_ViewPort.TopLeftX = m_ViewPort.TopLeftY = 0;
	m_ViewPort.Width = (float)SsaoMapWidth;
	m_ViewPort.Height = (float)SsaoMapHeight;
	m_ViewPort.MinDepth = D3D12_MIN_DEPTH;
	m_ViewPort.MaxDepth = D3D12_MAX_DEPTH;
	m_ScissorRect.left = (LONG)m_ViewPort.TopLeftX;
	m_ScissorRect.top = (LONG)m_ViewPort.TopLeftY;
	m_ScissorRect.right = (LONG)m_ViewPort.TopLeftX + (LONG)m_ViewPort.Width;
	m_ScissorRect.bottom = (LONG)m_ViewPort.TopLeftY + (LONG)m_ViewPort.Height;

	// Register GUI Callbak Function
	GraphicsCore::RegisterGuiCallbakFunction(std::bind(&CSsaoEffect::__UpdateGUI, this));

	__InitQuadGeomtry();
	__BuildConstantBuffer();
	__BuildRootSignature();
	__BuildPSO();
}

void CSsaoEffect::Render(const CCameraBasic* pCamera, CColorBuffer* pInputNormMap, CDepthBuffer* pInputDepthMap, CColorBuffer* pOutputSsaoMap, CGraphicsCommandList* pCommandList)
{
	_ASSERTE(pCamera && pInputNormMap && pInputDepthMap && pOutputSsaoMap && pCommandList);
	
	XMMATRIX View = pCamera->GetViewXMM();
	XMMATRIX Proj = pCamera->GetProjectXMM();
	XMMATRIX InvProj = XMMatrixInverse(&XMMatrixDeterminant(Proj), Proj);
	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	static const XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);
	m_CBSsao.FrameCount = m_UseTemporalRandom ? (UINT)GraphicsCore::GetFrameID() : 0;
	m_CBPerFrame.View = XMMatrixTranspose(View);
	m_CBPerFrame.Proj = XMMatrixTranspose(Proj);
	m_CBPerFrame.InvProj = XMMatrixTranspose(InvProj);
	m_CBPerFrame.ProjTex = XMMatrixTranspose(Proj * T);

	pCommandList->TransitionResource(*pInputNormMap, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pCommandList->TransitionResource(*pInputDepthMap, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pCommandList->TransitionResource(*pOutputSsaoMap, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	pCommandList->ClearColor(*pOutputSsaoMap);
	pCommandList->SetViewportAndScissor(m_ViewPort, m_ScissorRect);
	pCommandList->SetPipelineState(m_PSO);
	pCommandList->SetRootSignature(m_RootSignature);
	pCommandList->SetRenderTarget(pOutputSsaoMap->GetRTV());
	pCommandList->SetVertexBuffer(0, m_QuadVertexBufferView);
	pCommandList->SetIndexBuffer(m_QuadIndexBufferView);
	pCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->SetDynamicConstantBufferView(0, sizeof(m_CBPerFrame), &m_CBPerFrame);
	pCommandList->SetDynamicConstantBufferView(1, sizeof(m_CBSsao), &m_CBSsao);
	pCommandList->SetDynamicDescriptor(2, 0, pInputNormMap->GetSRV());
	pCommandList->SetDynamicDescriptor(3, 0, pInputDepthMap->GetDepthSRV());
	pCommandList->DrawIndexed(6, 0, 0);
}

void CSsaoEffect::__InitQuadGeomtry()
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

void CSsaoEffect::__BuildConstantBuffer()
{
	m_CBSsao.OcclusionRadius = 2.0f;
	m_CBSsao.OcclusionFadeStart = 0.05f;
	m_CBSsao.OcclusionFadeEnd = 4.0f;
	m_CBSsao.SurfaceEpsilon = 0.02f;
	m_CBSsao.SampleCount = 8;
	m_CBSsao.FrameCount = 0;
	m_CBSsao.SsaoPower = 4;
	m_CBSsao.UseTemporalRandom = m_UseTemporalRandom;
}

void CSsaoEffect::__BuildRootSignature()
{
	m_RootSignature.Reset(4, 2);
	GraphicsCommon::Sampler::SamplerLinearBorderDesc.SetBorderColor(CColor(1.0f, 1.0f, 1.0f, 1.0f));
	m_RootSignature.InitStaticSampler(0, 0, GraphicsCommon::Sampler::SamplerPointClampDesc, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature.InitStaticSampler(1, 0, GraphicsCommon::Sampler::SamplerDepthDesc, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature[0].InitAsConstantBuffer(0, 0, D3D12_SHADER_VISIBILITY_ALL);
	m_RootSignature[1].InitAsConstantBuffer(1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature.Finalize(L"SSAOEffectRootSignature", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
}

void CSsaoEffect::__BuildPSO()
{
	m_PSO.SetInputLayout((UINT)GraphicsCommon::InputLayout::PosTex.size(), GraphicsCommon::InputLayout::PosTex.data());
	m_PSO.SetRootSignature(m_RootSignature);
	D3D12_RASTERIZER_DESC DisableDepthTest = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	DisableDepthTest.DepthClipEnable = FALSE;
	m_PSO.SetRasterizerState(DisableDepthTest);
	m_PSO.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
	m_PSO.SetSampleMask(UINT_MAX);
	m_PSO.SetVertexShader(g_pSsaoEffectVS, sizeof(g_pSsaoEffectVS));
	m_PSO.SetPixelShader(g_pSsaoEffectPS, sizeof(g_pSsaoEffectPS));
	m_PSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	m_PSO.SetRenderTargetFormat(m_SsaoMapFormat, DXGI_FORMAT_UNKNOWN);
	m_PSO.Finalize();
}

void CSsaoEffect::__UpdateGUI()
{
	ImGui::SetNextWindowSize(ImVec2(450, 165));
	ImGui::Begin("SsaoEffect");
	ImGui::SliderFloat("AoRadius", &m_CBSsao.OcclusionRadius, 0.0f, 20.0f);
	ImGui::SliderFloat("SurfaceEpsilon", &m_CBSsao.SurfaceEpsilon, 0.0f, 1.0f, "%.3f", 2.0f);
	ImGui::SliderFloat("OcclusionFadeStart", &m_CBSsao.OcclusionFadeStart, 0.0f, 20.0f, "%.3f", 2.0f);
	ImGui::SliderFloat("OcclusionFadeEnd", &m_CBSsao.OcclusionFadeEnd, 0.0f, 20.0f, "%.3f");
	ImGui::SliderInt("AONumRays", &m_CBSsao.SampleCount, 1, 128); 
	ImGui::SliderInt("SsaoPower", &m_CBSsao.SsaoPower, 1, 10);
	ImGui::End();
}