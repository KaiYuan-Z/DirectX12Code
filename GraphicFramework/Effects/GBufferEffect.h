#pragma once
#include "../Core/GraphicsCoreBase.h"
#include "../Core/ColorBuffer.h"
#include "../Core/DepthBuffer.h"
#include "../Core/SceneGBufferRender.h"
#include "../Core/GraphicsCommon.h"


struct SGBufferConfig
{	
	UINT Width = 0;
	UINT Height = 0;
	DXGI_FORMAT DepthStencilBufferFormat = DXGI_FORMAT_D32_FLOAT;
	bool OpenBackFaceCulling = true;
	SGBufferRenderConfig SceneRenderConfig;
};

class CGBufferEffect
{
public:
	CGBufferEffect();
	~CGBufferEffect();

	void Init(const SGBufferConfig& config);

	void Render(CGraphicsCommandList *pCommandList, GraphicsCommon::EShaderingType Type = GraphicsCommon::EShaderingType::kPbr);

	CColorBuffer* GetPrePosBuffer() const { return m_pPrePosBuffer; }
	CColorBuffer* GetPreNormBuffer() const { return m_pPreNormBuffer; }
	CColorBuffer* GetPreDiffBuffer() const { return m_pPreDiffBuffer; }
	CColorBuffer* GetPreSpecBuffer() const { return m_pPreSpecBuffer; }
	CColorBuffer* GetPreMotionBuffer() const { return m_pPreMotionBuffer; }
	CColorBuffer* GetPreDdxyBuffer() const { return m_pPreDdxyBuffer; }
	CColorBuffer* GetPreLinearDepthBuffer() const { return m_pPreLinearDepthBuffer; }
	CColorBuffer* GetPreIdBuffer() const { return m_pPreIdBuffer; }

	CColorBuffer* GetCurPosBuffer() const { return m_pCurPosBuffer; }
	CColorBuffer* GetCurNormBuffer() const { return m_pCurNormBuffer; }
	CColorBuffer* GetCurDiffBuffer() const { return m_pCurDiffBuffer; }
	CColorBuffer* GetCurSpecBuffer() const { return m_pCurSpecBuffer; }
	CColorBuffer* GetCurMotionBuffer() const { return m_pCurMotionBuffer; }
	CColorBuffer* GetCurDdxyBuffer() const { return m_pCurDdxyBuffer; }
	CColorBuffer* GetCurLinearDepthBuffer() const { return m_pCurLinearDepthBuffer; }
	CColorBuffer* GetCurIdBuffer() const { return m_pCurIdBuffer; }

	CDepthBuffer* GetPreDepthBuffer() const { return m_pPreDepthBuffer; }
	CDepthBuffer* GetCurDepthBuffer() const { return m_pCurDepthBuffer; }

private:
	CColorBuffer* m_pPrePosBuffer = nullptr;
	CColorBuffer* m_pPreNormBuffer = nullptr;
	CColorBuffer* m_pPreDiffBuffer = nullptr;
	CColorBuffer* m_pPreSpecBuffer = nullptr;
	CColorBuffer* m_pPreMotionBuffer = nullptr;
	CColorBuffer* m_pPreDdxyBuffer = nullptr;
	CColorBuffer* m_pPreLinearDepthBuffer = nullptr;
	CColorBuffer* m_pPreIdBuffer = nullptr;

	CColorBuffer* m_pCurPosBuffer = nullptr;
	CColorBuffer* m_pCurNormBuffer = nullptr;
	CColorBuffer* m_pCurDiffBuffer = nullptr;
	CColorBuffer* m_pCurSpecBuffer = nullptr;
	CColorBuffer* m_pCurMotionBuffer = nullptr;
	CColorBuffer* m_pCurDdxyBuffer = nullptr;
	CColorBuffer* m_pCurLinearDepthBuffer = nullptr;
	CColorBuffer* m_pCurIdBuffer = nullptr;

	CDepthBuffer* m_pPreDepthBuffer = nullptr;
	CDepthBuffer* m_pCurDepthBuffer = nullptr;

	UINT m_FirstRootParamIndex = 0;

	CGraphicsPSO m_PSO;
	CRootSignature m_RootSignature;

	D3D12_VIEWPORT m_ViewPort;
	D3D12_RECT m_ScissorRect;

	CSceneGBufferRender m_GBufferRender;

	SGBufferConfig m_GBufferConfig;

	void __InitBuffers();
	void __InitRootRootSignature();
	void __InitPSO();
	void __ReleaseBuffers();
};

