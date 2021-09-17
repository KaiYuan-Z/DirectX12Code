#include "PbrModelViewer.h"
#include "../../GraphicFramework/Core/RootSignature.h"
#include "../../GraphicFramework/Core/GraphicsCore.h"
#include "../../GraphicFramework/Core/GraphicsCommon.h"

#include "CompiledShaders/ModelViewerPS.h"
#include "CompiledShaders/ModelViewerVS.h"

using Microsoft::WRL::ComPtr;
using namespace std;
using namespace DirectX;

ModelViewer::ModelViewer(std::wstring name) : CDXSample(name)
{
}

void ModelViewer::OnInit()
{
	__BuildScene();
	__BuildRootSignature();
	__BuildPSO();

	GraphicsCore::IdleGPU();
}

void ModelViewer::OnKeyDown(UINT8 key)
{
	/*switch (key)
	{
	default:_ASSERT(false); break;
	}*/

	m_pCamera->OnKeyDown(key);
}

void ModelViewer::OnMouseMove(UINT x, UINT y)
{
	m_pCamera->OnMouseMove(x, y);
}

void ModelViewer::OnRightButtonDown(UINT x, UINT y)
{
	m_pCamera->OnMouseButtonDown(x, y);
}

// Update frame-based values.
void ModelViewer::OnUpdate()
{
	m_Scene.OnFrameUpdate();
}

// Render the scene.
void ModelViewer::OnRender()
{
	if (!GraphicsCore::IsWindowVisible())
	{
		return;
	}

	CGraphicsCommandList* pGraphicsCommandList = GraphicsCore::BeginGraphicsCommandList();
	_ASSERTE(pGraphicsCommandList);

	m_GBufferEffect.Render(pGraphicsCommandList, GraphicsCommon::EShaderingType::kPbr);

	pGraphicsCommandList->TransitionResource(GraphicsCore::GetCurrentRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, false);
	pGraphicsCommandList->TransitionResource(GraphicsCore::GetDepthStencil(), D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
	pGraphicsCommandList->ClearColor(GraphicsCore::GetCurrentRenderTarget());
	pGraphicsCommandList->ClearDepth(GraphicsCore::GetDepthStencil());
	pGraphicsCommandList->SetRootSignature(m_RootSignature);
	pGraphicsCommandList->SetPipelineState(m_PSO);
	pGraphicsCommandList->SetRenderTarget(GraphicsCore::GetCurrentRenderTarget().GetRTV(), GraphicsCore::GetDepthStencil().GetDSV());
	m_DeferredRender.DrawQuad({
		m_GBufferEffect.GetCurPosBuffer(),
		m_GBufferEffect.GetCurNormBuffer(),
		m_GBufferEffect.GetCurDiffBuffer(),
		m_GBufferEffect.GetCurSpecBuffer(),
		m_GBufferEffect.GetCurIdBuffer()},
		pGraphicsCommandList);

	GraphicsCore::FinishCommandListAndPresent(pGraphicsCommandList);
}

void ModelViewer::__BuildRootSignature()
{
	m_RootSignature.Reset(m_DeferredRender.GetRootParameterCount(), m_DeferredRender.GetStaticSamplerCount());
	m_DeferredRender.ConfigurateRootSignature(m_RootSignature);
	m_RootSignature.Finalize(L"ModelViewer", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
}

void ModelViewer::__BuildPSO()
{
	m_PSO.SetInputLayout((UINT)GraphicsCommon::InputLayout::PosTexNormTanBitan.size(), GraphicsCommon::InputLayout::PosTexNormTanBitan.data());
	m_PSO.SetRootSignature(m_RootSignature);
	m_PSO.SetVertexShader(g_pModelViewerVS, sizeof(g_pModelViewerVS));
	m_PSO.SetPixelShader(g_pModelViewerPS, sizeof(g_pModelViewerPS));
	auto RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;
	m_PSO.SetRasterizerState(RasterizerState);
	m_PSO.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
	m_PSO.SetDepthStencilState(CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT));
	m_PSO.SetSampleMask(UINT_MAX);
	m_PSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	DXGI_FORMAT RenderTargetFormat = GraphicsCore::GetBackBufferFormat();
	m_PSO.SetRenderTargetFormats(1, &RenderTargetFormat, GraphicsCore::GetDepthBufferFormat());
	m_PSO.Finalize();
}

void ModelViewer::__BuildScene()
{
	// Load Scene
	m_Scene.LoadScene(L"scene.txt");

	// Get Camera
	CCameraBasic* pCamera = m_Scene.GetCamera(m_Scene.QueryCameraIndex(L"camera0"));
	_ASSERTE(pCamera);
	m_pCamera = pCamera->QueryWalkthroughCamera();
	_ASSERTE(m_pCamera);

	// Init GBuffer Effect
	SGBufferConfig gBufferConfig;
	gBufferConfig.OpenBackFaceCulling = true;
	gBufferConfig.SceneRenderConfig.ActiveCameraIndex = m_Scene.QueryCameraIndex(L"camera0");
	gBufferConfig.SceneRenderConfig.pScene = &m_Scene;
	gBufferConfig.SceneRenderConfig.EnableNormalMap = true;
	gBufferConfig.SceneRenderConfig.EnableNormalMapCompression = true;
	gBufferConfig.Width = GraphicsCore::GetWindowWidth();
	gBufferConfig.Height = GraphicsCore::GetWindowHeight();
	m_GBufferEffect.Init(gBufferConfig);

	// Init Deferred Scene Render
	SSceneLightDeferredRenderConfig deferredConfig;
	deferredConfig.GBufferWidth = GraphicsCore::GetWindowWidth();
	deferredConfig.GBufferHeight = GraphicsCore::GetWindowHeight();
	deferredConfig.pScene = &m_Scene;
	deferredConfig.ActiveCameraIndex = m_Scene.QueryCameraIndex(L"camera0");
	deferredConfig.AmbientLight = { 0.3f, 0.3f,0.3f,1.0f };
	m_DeferredRender.InitRender(deferredConfig);
}
