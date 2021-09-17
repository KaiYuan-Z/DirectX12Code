#include "SkyEffect.h"
#include "../Core/GraphicsCommon.h"
#include "../Core/Texture.h"
#include "../Core/GeometryGenerator.h"

#include "CompiledShaders/SkyEffectVS.h"
#include "CompiledShaders/SkyEffectPS.h"

CSkyEffect::CSkyEffect()
{
}

CSkyEffect::~CSkyEffect()
{
}

void CSkyEffect::Init(const SSkyEffectConfig& config)
{
	_ASSERTE(config.pMainCamera);

	m_Config = config;

	const std::string CubeMapRootAddr = "../Textures/CubeMap/" + MakeStr(m_Config.CubeMapName);
	auto* pCubeMapTex = TextureLoader::LoadFromFile(CubeMapRootAddr, false);
	_ASSERTE(pCubeMapTex);
	m_CubeMapSrv = pCubeMapTex->GetSRV();

	__InitSphereGeomtry();
	__BuildRootSignature();
	__BuildPSO();
}

void CSkyEffect::Render(bool clearRenderTarget, CColorBuffer* pRenderTarget, CGraphicsCommandList* pCommandList)
{
	_ASSERTE(pRenderTarget);
	_ASSERTE(pCommandList);

	XMMATRIX View = m_Config.pMainCamera->GetViewXMM();
	XMMATRIX Proj = m_Config.pMainCamera->GetProjectXMM();
	m_SkyCB.ViewProj = View * Proj;
	m_SkyCB.CamPosW = m_Config.pMainCamera->GetPositionXMF3();

	pCommandList->TransitionResource(*pRenderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	if(clearRenderTarget) pCommandList->ClearColor(*pRenderTarget);
	pCommandList->SetViewportAndScissor(GraphicsCore::GetScreenViewport(), GraphicsCore::GetScissorRect());
	pCommandList->SetPipelineState(m_PSO);
	pCommandList->SetRootSignature(m_RootSignature);
	pCommandList->SetRenderTarget(pRenderTarget->GetRTV());
	pCommandList->SetVertexBuffer(0, m_SphereVertexBufferView);
	pCommandList->SetIndexBuffer(m_SphereIndexBufferView);
	pCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->SetDynamicConstantBufferView(0, sizeof(m_SkyCB), &m_SkyCB);
	pCommandList->SetDynamicDescriptor(1, 0, m_CubeMapSrv);
	pCommandList->DrawIndexed(m_SphereIndexBuffer.GetElementCount(), 0, 0);

}

void CSkyEffect::__InitSphereGeomtry()
{
	CGeometryGenerator GeoGen;
	CGeometryGenerator::SMeshData Sphere = GeoGen.CreateSphere(0.5f, 20, 20);
	std::vector<XMFLOAT3> SphereVertexSet(Sphere.Vertices.size());
	for (UINT i = 0; i < Sphere.Vertices.size(); i++)
	{
		SphereVertexSet[i] = Sphere.Vertices[i].Position;
	}
	
	m_SphereVertexBuffer.Create(L"Sky Sphere Vertex Buffer", (UINT)SphereVertexSet.size(), sizeof(XMFLOAT3), SphereVertexSet.data());
	m_SphereVertexBufferView = m_SphereVertexBuffer.CreateVertexBufferView();

	m_SphereIndexBuffer.Create(L"Sky Sphere Index Buffer", (UINT)Sphere.Indices32.size(), sizeof(UINT), Sphere.Indices32.data());
	m_SphereIndexBufferView = m_SphereIndexBuffer.CreateIndexBufferView();
}

void CSkyEffect::__BuildRootSignature()
{
	m_RootSignature.Reset(2, 1);
	m_RootSignature.InitStaticSampler(0, 0, GraphicsCommon::Sampler::SamplerLinearWrapDesc, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature[0].InitAsConstantBuffer(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	m_RootSignature[1].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature.Finalize(L"SkyEffectRootSignature", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
}

void CSkyEffect::__BuildPSO()
{
	m_PSO.SetInputLayout((UINT)GraphicsCommon::InputLayout::Pos.size(), GraphicsCommon::InputLayout::Pos.data());
	m_PSO.SetRootSignature(m_RootSignature);
	D3D12_RASTERIZER_DESC RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	D3D12_DEPTH_STENCIL_DESC DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	DepthStencilState.DepthEnable = FALSE;
	m_PSO.SetDepthStencilState(DepthStencilState);
	m_PSO.SetRasterizerState(RasterizerState);
	m_PSO.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
	m_PSO.SetSampleMask(UINT_MAX);
	m_PSO.SetVertexShader(g_pSkyEffectVS, sizeof(g_pSkyEffectVS));
	m_PSO.SetPixelShader(g_pSkyEffectPS, sizeof(g_pSkyEffectPS));
	m_PSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	m_PSO.SetRenderTargetFormat(m_Config.RenderTargetFormat, DXGI_FORMAT_UNKNOWN);
	m_PSO.Finalize();
}