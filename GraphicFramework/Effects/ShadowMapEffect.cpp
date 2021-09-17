#include "ShadowMapEffect.h"

#include "../Core/GraphicsCommon.h"
#include "../Core/Scene.h"
#include "../Core/DirectionalLight.h"

#include "CompiledShaders/ShadowMapEffectPS.h"
#include "CompiledShaders/ShadowMapEffectVS.h"

CShadowMapEffect::CShadowMapEffect()
{
}


CShadowMapEffect::~CShadowMapEffect()
{
}

void CShadowMapEffect::Init(const SShadowMapConfig& config)
{
	_ASSERTE(config.Size > 0);
	_ASSERTE(config.pScene);

	m_ShadowMapConfig = config;

	m_ViewPort = { 0.0f, 0.0f, (float)m_ShadowMapConfig.Size, (float)m_ShadowMapConfig.Size, 0.0f, 1.0f };
	m_ScissorRect = { 0, 0, (LONG)m_ShadowMapConfig.Size,(LONG)m_ShadowMapConfig.Size };

	SSceneGeometryOnlyConfig RenderConfig;
	RenderConfig.ActiveCameraIndex = 0;
	RenderConfig.MaxModelInstanceCount = config.MaxModelInstanceCount;
	RenderConfig.pScene = config.pScene;
	m_ShadowRender.InitRender(RenderConfig);

	__InitBuffers();
	__InitRootRootSignature();
	__InitPSO();
}

void CShadowMapEffect::UpdateShadowTransform(const XMFLOAT3& SceneBoundCenter, float SceneBoundRadius)
{
	auto* pScene = m_ShadowMapConfig.pScene;
	_ASSERTE(pScene);

	auto* pLight =  pScene->GetLight(m_ShadowMapConfig.LightIndexForCastShadow);
	_ASSERTE(pLight);
	auto* pDirLight = pLight->QueryDirectionalLight();
	_ASSERTE(pDirLight);
	
	// Only the first "main" light casts a shadow.
	XMVECTOR lightDir = XMLoadFloat3(&pDirLight->GetLightDirW());
	XMVECTOR lightPos = -2.0f * SceneBoundRadius * lightDir;
	XMVECTOR targetPos = XMLoadFloat3(&SceneBoundCenter);
	XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	m_LightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

	// Transform bounding sphere to light space.
	XMFLOAT3 sphereCenterLS;
	XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, m_LightView));

	// Ortho frustum in light space encloses scene.
	float l = sphereCenterLS.x - SceneBoundRadius;
	float b = sphereCenterLS.y - SceneBoundRadius;
	float n = sphereCenterLS.z - SceneBoundRadius;
	float r = sphereCenterLS.x + SceneBoundRadius;
	float t = sphereCenterLS.y + SceneBoundRadius;
	float f = sphereCenterLS.z + SceneBoundRadius;

	m_LightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	m_ShadowTransform = m_LightView * m_LightProj * T;
}

void CShadowMapEffect::Render(CGraphicsCommandList* pCommandList)
{
	_ASSERTE(pCommandList);

	pCommandList->TransitionResource(m_ShadowMap, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
	pCommandList->ClearDepthAndStencil(m_ShadowMap);

	pCommandList->SetViewportAndScissor(m_ViewPort, m_ScissorRect);
	pCommandList->SetRootSignature(m_RootSignature);
	pCommandList->SetPipelineState(m_PSO);

	pCommandList->SetDepthStencilTarget(m_ShadowMap.GetDSV());

	XMMATRIX ShadowViewProj = m_LightView * m_LightProj;
	pCommandList->SetDynamicConstantBufferView(m_FirstRootParamIndex, sizeof(ShadowViewProj), &ShadowViewProj);


	m_ShadowRender.DrawScene(pCommandList);
}

void CShadowMapEffect::__InitBuffers()
{
	m_ShadowMap.Create2D(L"ShadowMap", m_ShadowMapConfig.Size, m_ShadowMapConfig.Size, DXGI_FORMAT_D32_FLOAT);
	m_ShadowMap.SetClearDepth(1.0f);
	m_ShadowMap.SetClearStencil(0);
}

void CShadowMapEffect::__InitRootRootSignature()
{
	m_FirstRootParamIndex = m_ShadowRender.GetRootParameterCount();
	m_RootSignature.Reset(m_ShadowRender.GetRootParameterCount() + 1, m_ShadowRender.GetStaticSamplerCount());
	m_ShadowRender.ConfigurateRootSignature(m_RootSignature);
	m_RootSignature[m_FirstRootParamIndex].InitAsConstantBuffer(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	m_RootSignature.Finalize(L"ShadowMapEffect", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
}

void CShadowMapEffect::__InitPSO()
{
	m_PSO.SetInputLayout((UINT)GraphicsCommon::InputLayout::PosTexNormTanBitan.size(), GraphicsCommon::InputLayout::PosTexNormTanBitan.data());
	m_PSO.SetRootSignature(m_RootSignature);
	m_PSO.SetVertexShader(g_pShadowMapEffectVS,sizeof(g_pShadowMapEffectVS));
	m_PSO.SetPixelShader(g_pShadowMapEffectPS, sizeof(g_pShadowMapEffectPS));
	auto RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	RasterizerState.DepthBias = 7500;
	RasterizerState.DepthBiasClamp = 0.0f;
	RasterizerState.SlopeScaledDepthBias = 1.0f;
	RasterizerState.CullMode =  m_ShadowMapConfig.OpenBackFaceCulling ?  D3D12_CULL_MODE::D3D12_CULL_MODE_BACK : D3D12_CULL_MODE::D3D12_CULL_MODE_NONE;
	m_PSO.SetRasterizerState(RasterizerState);
	m_PSO.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
	m_PSO.SetDepthStencilState(CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT));
	m_PSO.SetSampleMask(UINT_MAX);
	m_PSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	m_PSO.SetRenderTargetFormats(0, nullptr, DXGI_FORMAT_D32_FLOAT);
	m_PSO.Finalize();
}