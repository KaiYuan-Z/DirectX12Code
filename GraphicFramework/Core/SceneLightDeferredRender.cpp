#include "SceneLightDeferredRender.h"
#include "GraphicsCommon.h"


CSceneLightDeferredRender::CSceneLightDeferredRender()
{
}

CSceneLightDeferredRender::~CSceneLightDeferredRender()
{
}

void CSceneLightDeferredRender::InitRender(const SSceneLightDeferredRenderConfig& renderConfig)
{
	_ASSERTE(renderConfig.pScene);
	_ASSERTE(renderConfig.ActiveCameraIndex != INVALID_OBJECT_INDEX);
	m_RenderConfig = renderConfig;

	m_ViewPort.TopLeftX = m_ViewPort.TopLeftY = 0;
	m_ViewPort.Width = (float)renderConfig.GBufferWidth;
	m_ViewPort.Height = (float)renderConfig.GBufferHeight;
	m_ViewPort.MinDepth = D3D12_MIN_DEPTH;
	m_ViewPort.MaxDepth = D3D12_MAX_DEPTH;
	m_ScissorRect.left = (LONG)m_ViewPort.TopLeftX;
	m_ScissorRect.top = (LONG)m_ViewPort.TopLeftY;
	m_ScissorRect.right = (LONG)m_ViewPort.TopLeftX + (LONG)m_ViewPort.Width;
	m_ScissorRect.bottom = (LONG)m_ViewPort.TopLeftY + (LONG)m_ViewPort.Height;

	m_FrameParameter.AmbientLight = renderConfig.AmbientLight;
	m_FrameParameter.TexWidth = renderConfig.GBufferWidth;
	m_FrameParameter.TexHeight = renderConfig.GBufferHeight;

	CScene* pScene = renderConfig.pScene;
	UINT lightCount = pScene->GetLightCount();
	lightCount = lightCount > renderConfig.MaxSceneLightCount ? renderConfig.MaxSceneLightCount : lightCount;
	std::vector<SLightData> lightDataSet(lightCount);
	for (UINT i = 0; i < lightCount; i++)
	{
		CLight* pLight = pScene->GetLight(i);
		_ASSERTE(pLight);
		lightDataSet[i] = pLight->GetLightData();
	}
	m_LightDataBuffer.Create(L"LightData", lightCount, sizeof(SLightData), lightDataSet.data());

	__InitQuadGeomtry();
}

void CSceneLightDeferredRender::ConfigurateRootSignature(CRootSignature& rootSignature)
{
	_ASSERTE(rootSignature.GetRootParameterCount() >= nSceneRenderRootParameterNum);
	rootSignature[kGBufferSrvs].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, kGBufferSrvRegister, 5, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kLightDataSrv].InitAsBufferSRV(kLightDataSrvRegister, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kLightRenderIndecesSrv].InitAsBufferSRV(kLightRenderIndecesSrvRegister, 1, D3D12_SHADER_VISIBILITY_ALL);
	rootSignature[kPerFrameCB].InitAsConstantBuffer(kPerFrameCbRegister, 1, D3D12_SHADER_VISIBILITY_ALL);
}

void CSceneLightDeferredRender::__InitQuadGeomtry()
{
	std::vector<GraphicsCommon::Vertex::SPosTex> QuadVertex = {
		{{-1.0f, -1.0f, 0.0f},{0.0f, 1.0f}},
		{{-1.0f,  1.0f, 0.0f},{0.0f, 0.0f}},
		{{1.0f,  1.0f, 0.0f},{1.0f, 0.0f}},
		{{1.0f, -1.0f, 0.0f},{1.0f, 1.0f}}
	};
	m_QuadVertexBuffer.Create(L"Deferred Scene Render Quad Vertex Buffer", 4, sizeof(GraphicsCommon::Vertex::SPosTex), QuadVertex.data());
	m_QuadVertexBufferView = m_QuadVertexBuffer.CreateVertexBufferView();

	std::vector<uint16_t> QuadIndex = { 0, 1, 2, 0, 2, 3 };
	m_QuadIndexBuffer.Create(L"Deferred Scene Render Quad Index Buffer", 6, sizeof(uint16_t), QuadIndex.data());
	m_QuadIndexBufferView = m_QuadIndexBuffer.CreateIndexBufferView();
}

void CSceneLightDeferredRender::DrawQuad(const SDeferredRenderInputDesc& inputDesc, CGraphicsCommandList* pCommandList)
{
	_ASSERTE(pCommandList);
	_ASSERTE(inputDesc.pPosMap);
	_ASSERTE(inputDesc.pNormMap);
	_ASSERTE(inputDesc.pDiffMap);
	_ASSERTE(inputDesc.pSpecMap);
	_ASSERTE(inputDesc.pIdMap);

	CScene* pScene = m_RenderConfig.pScene;
	_ASSERTE(pScene);
	
	pScene->SetActiveCamera(m_RenderConfig.ActiveCameraIndex);
	auto& lightRenderSet = pScene->GetActiveCameraLightRenderSet().LightIndexSet;

	CCameraBasic* pCamera = pScene->GetCamera(m_RenderConfig.ActiveCameraIndex);
	_ASSERTE(pCamera);
	XMFLOAT3 CamPos = pCamera->GetPositionXMF3();
	m_FrameParameter.EyePosW = { CamPos.x, CamPos.y, CamPos.z, 1.0f };
	m_FrameParameter.LightRenderCount = (UINT)lightRenderSet.size();

	D3D12_CPU_DESCRIPTOR_HANDLE gBufferHandles[5] = {
		inputDesc.pPosMap->GetSRV(),
		inputDesc.pNormMap->GetSRV(),
		inputDesc.pDiffMap->GetSRV(),
		inputDesc.pSpecMap->GetSRV(),
		inputDesc.pIdMap->GetSRV()
	};

	pCommandList->TransitionResource(*inputDesc.pPosMap, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pCommandList->TransitionResource(*inputDesc.pNormMap, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pCommandList->TransitionResource(*inputDesc.pDiffMap, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pCommandList->TransitionResource(*inputDesc.pSpecMap, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pCommandList->TransitionResource(*inputDesc.pIdMap, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, true);
	pCommandList->SetViewportAndScissor(m_ViewPort, m_ScissorRect);
	pCommandList->SetDynamicDescriptors(kGBufferSrvs, 0, 5, gBufferHandles);
	pCommandList->SetBufferSRV(kLightDataSrv, m_LightDataBuffer);
	pCommandList->SetDynamicSRV(kLightRenderIndecesSrv, lightRenderSet.size()*sizeof(UINT), lightRenderSet.data());
	pCommandList->SetDynamicConstantBufferView(kPerFrameCB, sizeof(SFrameParameter), &m_FrameParameter);
	pCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->SetVertexBuffer(0, m_QuadVertexBufferView);
	pCommandList->SetIndexBuffer(m_QuadIndexBufferView);
	pCommandList->DrawIndexed(6, 0, 0);
}