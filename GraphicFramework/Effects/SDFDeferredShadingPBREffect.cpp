#include "SDFDeferredShadingPBREffect.h"
#include "../Core/GraphicsCore.h"
#include "../Core/GraphicsCommon.h"
#include "CompiledShaders/SDFDeferredShadingPBREffectVS.h"
#include "CompiledShaders/SDFDeferredShadingPBREffectPS.h"

CSDFDeferredShadingPBREffect::CSDFDeferredShadingPBREffect()
{
}


CSDFDeferredShadingPBREffect::~CSDFDeferredShadingPBREffect()
{
}

void CSDFDeferredShadingPBREffect::Init(CScene* pScene, DXGI_FORMAT PBROutputMapFormat, UINT PBRMapWidth, UINT PBRMapHeight, XMFLOAT3 AmbientLight)
{
	_ASSERTE(pScene);
	m_pScene = pScene;
	m_PBROutputMapFormat = PBROutputMapFormat;
	m_CBPerFrame.AmbientLight = AmbientLight;

	m_ViewPort.TopLeftX = m_ViewPort.TopLeftY = 0;
	m_ViewPort.Width = (float)PBRMapWidth;
	m_ViewPort.Height = (float)PBRMapHeight;
	m_ViewPort.MinDepth = D3D12_MIN_DEPTH;
	m_ViewPort.MaxDepth = D3D12_MAX_DEPTH;
	m_ScissorRect.left = (LONG)m_ViewPort.TopLeftX;
	m_ScissorRect.top = (LONG)m_ViewPort.TopLeftY;
	m_ScissorRect.right = (LONG)m_ViewPort.TopLeftX + (LONG)m_ViewPort.Width;
	m_ScissorRect.bottom = (LONG)m_ViewPort.TopLeftY + (LONG)m_ViewPort.Height;

	__InitQuadGeomtry();
	__BuildConstantBuffer();
	__BuildRootSignature();
	__BuildPSO();
}

void CSDFDeferredShadingPBREffect::Render(const std::wstring& cameraName, const SPBRInputBufferDesc* pGBufferOutputDesc, CColorBuffer* pOutputPBRMap, CGraphicsCommandList* pCommandList)
{
	_ASSERTE(pGBufferOutputDesc && pOutputPBRMap && pCommandList);

	CCameraBasic* pCamera = m_pScene->GetCamera(m_pScene->QueryCameraIndex(cameraName));
	_ASSERTE(pCamera);
	XMMATRIX Proj = pCamera->GetProjectXMM();
	m_CBPerFrame.InvProj = XMMatrixInverse(&XMMatrixDeterminant(Proj), Proj);
	m_CBPerFrame.CameraPosW = pCamera->GetPositionXMF3();
	auto m_pShadowAndSSSBuffer = &m_ShadowAndSSSBuffer;
	if (pGBufferOutputDesc->pShadowAndSSSBuffer)
	{
		m_CBPerFrame.SDFEffectEnable = true;
		m_pShadowAndSSSBuffer = pGBufferOutputDesc->pShadowAndSSSBuffer;
	}
	m_pScene->SetActiveCamera(m_pScene->QueryCameraIndex(cameraName));
	auto& lightRenderSet = m_pScene->GetActiveCameraLightRenderSet().LightIndexSet;
	m_CBPerFrame.LightRenderCount = (UINT)lightRenderSet.size();

	pCommandList->TransitionResource(*pGBufferOutputDesc->pPosBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pCommandList->TransitionResource(*pGBufferOutputDesc->pNormBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pCommandList->TransitionResource(*pGBufferOutputDesc->pBaseColorBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pCommandList->TransitionResource(*pGBufferOutputDesc->pSpecularBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pCommandList->TransitionResource(*m_pShadowAndSSSBuffer, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, false);
	pCommandList->TransitionResource(*pOutputPBRMap, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	if (pGBufferOutputDesc->ClearHint)
	{
		pCommandList->ClearColor(*pOutputPBRMap);
	}

	pCommandList->SetViewportAndScissor(m_ViewPort, m_ScissorRect);
	pCommandList->SetPipelineState(m_PSO);
	pCommandList->SetRootSignature(m_RootSignature);
	pCommandList->SetRenderTarget(pOutputPBRMap->GetRTV());
	pCommandList->SetVertexBuffer(0, m_QuadVertexBufferView);
	pCommandList->SetIndexBuffer(m_QuadIndexBufferView);
	pCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	pCommandList->SetDynamicDescriptor(kPositionMapSRV, 0, pGBufferOutputDesc->pPosBuffer->GetSRV());
	pCommandList->SetDynamicDescriptor(kNormMapSRV, 0, pGBufferOutputDesc->pNormBuffer->GetSRV());
	pCommandList->SetDynamicDescriptor(kBaseColorMapSRV, 0, pGBufferOutputDesc->pBaseColorBuffer->GetSRV());
	pCommandList->SetDynamicDescriptor(kSpecularMapSRV, 0, pGBufferOutputDesc->pSpecularBuffer->GetSRV());
	pCommandList->SetDynamicDescriptor(kShadowAndSSSMapSRV, 0, m_pShadowAndSSSBuffer->GetSRV());
	pCommandList->SetBufferSRV(kLightDataSRV, m_LightDataBuffer);
	pCommandList->SetDynamicSRV(kLightIndecesSRV, lightRenderSet.size() * sizeof(UINT), lightRenderSet.data());
	pCommandList->SetDynamicConstantBufferView(kPerFrameCB, sizeof(SPerframe), &m_CBPerFrame);
	pCommandList->DrawIndexed(6, 0, 0);
}

void CSDFDeferredShadingPBREffect::__InitQuadGeomtry()
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

void CSDFDeferredShadingPBREffect::__BuildConstantBuffer()
{
	UINT lightCount = m_pScene->GetLightCount();
	_ASSERTE(lightCount > 0);
	std::vector<SLightData> lightDataSet(lightCount);
	for (UINT i = 0; i < lightCount; i++)
	{
		CLight* pLight = m_pScene->GetLight(i);
		_ASSERTE(pLight);
		lightDataSet[i] = pLight->GetLightData();
	}
	m_LightDataBuffer.Create(L"LightData", lightCount, sizeof(SLightData), lightDataSet.data());

	m_ShadowAndSSSBuffer.Create2D(L"ShadowAndSSSBufferDefault", GraphicsCore::GetWindowWidth(), GraphicsCore::GetWindowHeight(), 1, DXGI_FORMAT_R32G32B32A32_FLOAT);
}

void CSDFDeferredShadingPBREffect::__BuildRootSignature()
{
	m_RootSignature.Reset(nRootParameterNum, nStaticSamplerNum);
	m_RootSignature.InitStaticSampler(kLinearWrapSamplerRegister, 0, GraphicsCommon::Sampler::SamplerLinearWrapDesc, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature[kPositionMapSRV].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, kPositionMapSRVRegister, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature[kNormMapSRV].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, kNormMapSRVRegister, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature[kBaseColorMapSRV].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, kBaseColorMapSRVRegister, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature[kSpecularMapSRV].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, kSpecularMapSRVRegister, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature[kShadowAndSSSMapSRV].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, kShadowAndSSSMapSRVRegister, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature[kLightDataSRV].InitAsBufferSRV(kLightDataSRVRegister, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature[kLightIndecesSRV].InitAsBufferSRV(kLightIndecesSRVRegister, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature[kPerFrameCB].InitAsConstantBuffer(kPerFrameCBRegister, 0, D3D12_SHADER_VISIBILITY_ALL);
	m_RootSignature.Finalize(L"DeferredShadingPBREffectRootSignature", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
}

void CSDFDeferredShadingPBREffect::__BuildPSO()
{
	m_PSO.SetInputLayout((UINT)GraphicsCommon::InputLayout::PosTex.size(), GraphicsCommon::InputLayout::PosTex.data());
	m_PSO.SetRootSignature(m_RootSignature);
	D3D12_RASTERIZER_DESC DisableDepthTest = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	DisableDepthTest.DepthClipEnable = FALSE;
	m_PSO.SetRasterizerState(DisableDepthTest);
	m_PSO.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
	m_PSO.SetSampleMask(UINT_MAX);
	m_PSO.SetVertexShader(g_pSDFDeferredShadingPBREffectVS, sizeof(g_pSDFDeferredShadingPBREffectVS));
	m_PSO.SetPixelShader(g_pSDFDeferredShadingPBREffectPS, sizeof(g_pSDFDeferredShadingPBREffectPS));
	m_PSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	m_PSO.SetRenderTargetFormat(m_PBROutputMapFormat, DXGI_FORMAT_UNKNOWN);
	m_PSO.Finalize();
}
