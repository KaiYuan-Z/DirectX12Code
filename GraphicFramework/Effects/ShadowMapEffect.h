#pragma once
#include "../Core/GraphicsCoreBase.h"
#include "../Core/GraphicsCommon.h"
#include "../Core/GraphicsCore.h"
#include "../Core/ColorBuffer.h"
#include "../Core/DepthBuffer.h"
#include "../Core/SceneGeometryOnlyRender.h"

class CScene;
class CSceneGeometryOnlyRender;

struct SShadowMapConfig
{	
	UINT Size = 0;
	UINT LightIndexForCastShadow = 0;
	bool OpenBackFaceCulling = true;
	UINT MaxModelInstanceCount = 100;
	CScene* pScene = nullptr;
};

class CShadowMapEffect
{
	struct SShadowCB
	{
		XMMATRIX ShadowViewProj = XMMatrixIdentity();
	};

public:
	CShadowMapEffect();
	~CShadowMapEffect();

	void Init(const SShadowMapConfig& config);
	void UpdateShadowTransform(const XMFLOAT3& SceneBoundCenter, float SceneBoundRadius);
	void Render(CGraphicsCommandList *pCommandList);

	CDepthBuffer& GetShadowMapBuffer() { return m_ShadowMap; }

	XMMATRIX& GetShadowTransform() { return m_ShadowTransform; }

private:

	CDepthBuffer m_ShadowMap;

	UINT m_FirstRootParamIndex = 0;

	CGraphicsPSO m_PSO;
	CRootSignature m_RootSignature;

	D3D12_VIEWPORT m_ViewPort;
	D3D12_RECT m_ScissorRect;

	SShadowMapConfig m_ShadowMapConfig;

	SShadowCB m_ShadowCB;

	XMMATRIX m_LightView = XMMatrixIdentity();
	XMMATRIX m_LightProj = XMMatrixIdentity();
	XMMATRIX m_ShadowTransform = XMMatrixIdentity();

	CSceneGeometryOnlyRender m_ShadowRender;

	void __InitBuffers();
	void __InitRootRootSignature();
	void __InitPSO();
};

