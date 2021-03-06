#include "SSAOTest.h"
#include "../../GraphicFramework/Core/RootSignature.h"
#include "../../GraphicFramework/Core/GraphicsCore.h"
#include "../../GraphicFramework/Core/GraphicsCommon.h"

using Microsoft::WRL::ComPtr;
using namespace std;
using namespace DirectX;

CSSAOTest::CSSAOTest(std::wstring name) : CDXSample(name)
{
}

void CSSAOTest::OnInit()
{
	GraphicsCore::RegisterGuiCallbakFunction(std::bind(&CSSAOTest::__UpdateGUI, this));
	
	__InitScene();

	__InitEffectResource();

	GraphicsCore::IdleGPU();
}

void CSSAOTest::OnKeyDown(UINT8 key)
{
	/*switch (key)
	{
	default:_ASSERT(false); break;
	}*/

	m_pCamera->OnKeyDown(key);
}

void CSSAOTest::OnMouseMove(UINT x, UINT y)
{
	m_pCamera->OnMouseMove(x, y);
}

void CSSAOTest::OnRightButtonDown(UINT x, UINT y)
{
	m_pCamera->OnMouseButtonDown(x, y);
}

// Update frame-based values.
void CSSAOTest::OnUpdate()
{
	m_Scene.OnFrameUpdate();
}

// Render the scene.
void CSSAOTest::OnRender()
{
	if (!GraphicsCore::IsWindowVisible())
	{
		return;
	}

	//
	//Draw GBuffer
	//
	CGraphicsCommandList* pGraphicsCommandList = GraphicsCore::BeginGraphicsCommandList();
	_ASSERTE(pGraphicsCommandList);	
	m_GBufferEffect.Render(pGraphicsCommandList);
	GraphicsCore::FinishCommandList(pGraphicsCommandList, true);

	//
	// Draw SSAO
	//
	XMMATRIX Proj = m_pCamera->GetProjectXMM();
	XMMATRIX CurView = m_pCamera->GetViewXMM();
	m_AoTime =  m_SsaoEffect.Render(Proj, CurView, m_GBufferEffect.GetCurDepthBuffer(), m_GBufferEffect.GetCurNormBuffer(), true);

	//
	// Show Output Image
	//
	pGraphicsCommandList = GraphicsCore::BeginGraphicsCommandList();
	_ASSERTE(pGraphicsCommandList);
	m_ImageEffect.Render(kRED, &m_SsaoEffect.GetAoMap(), pGraphicsCommandList);
	GraphicsCore::FinishCommandListAndPresent(pGraphicsCommandList, true);
}

void CSSAOTest::OnPostProcess()
{
	m_Scene.OnFramePostProcess();
}

void CSSAOTest::OnDestroy()
{
	m_SsaoEffect.Cleanup();
}

void CSSAOTest::__InitScene()
{
	// Init Scene
	m_Scene.LoadScene(L"scene.txt");
	m_pCamera = m_Scene.GetCamera(m_Scene.QueryCameraIndex(L"camera0"))->QueryWalkthroughCamera();
}

void CSSAOTest::__InitEffectResource()
{
	SGeometryOnlyGBufferConfig GBufferConfig;
	GBufferConfig.OpenBackFaceCulling = false;
	GBufferConfig.SceneRenderConfig.ActiveCameraIndex = 0;
	GBufferConfig.SceneRenderConfig.pScene = &m_Scene;
	GBufferConfig.Width = GraphicsCore::GetWindowWidth();
	GBufferConfig.Height = GraphicsCore::GetWindowHeight();
	GBufferConfig.DepthStencilBufferFormat = DXGI_FORMAT_D32_FLOAT;
	m_GBufferEffect.Init(GBufferConfig);

	SHBAOPlusParams HBAOPlusParams; 
	HBAOPlusParams.NumSteps = 4; // 8 Direction * 4 Step = 32 spp
	HBAOPlusParams.Radius = 20;
	HBAOPlusParams.Bias = 0.25;
	HBAOPlusParams.PowerExponent = 2.0f;
	HBAOPlusParams.MetersToViewSpaceUnits = 1;
	m_SsaoEffect.Initialize(GraphicsCore::GetWindowWidth(), GraphicsCore::GetWindowHeight(), HBAOPlusParams);

	m_ImageEffect.Init(1.0f);
}

void CSSAOTest::__UpdateGUI()
{
	ImGui::SetNextWindowSize(ImVec2(220, 100));
	ImGui::Begin("HBAO PLUS");
	ImGui::Text("HBAO PLUS TIME(%f ms)", m_AoTime*1000.0f);
	ImGui::End();
}
