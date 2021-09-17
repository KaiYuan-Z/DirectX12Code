#pragma once
#include "../Core/MathUtility.h"
#include "../Core/ColorBuffer.h"
#include "../Core/GraphicsCoreBase.h"
#include "../Core/GraphicsCore.h"
#include "../Core/UserDescriptorHeap.h"

#include "DecodeNormalEffect.h"

//==========================================================
//Note：HBAOPlus的模块并没有使用本框架自己的管线，HBAOPlus
// 由Nvidia的DLL内部自己实现的绘制，它的绘制使用的Buffer没有
// 做多缓冲。因此导致目前HBAOPlusEffect自己申请CommandList自
// 己提交CommandList。当HBAOPlusEffect和其他绘制代码混合使用
// 时注意多个CommandList提交指令的顺序问题。千万别出现G-Buffer
// 的CommandList还未执行，HBAOPlus的CommandList就已经提交。
//==========================================================

struct SHBAOPlusParams
{
	SHBAOPlusParams()
		: EnableHBAOPlus(true)
		, EnableNormals(true)
		, MetersToViewSpaceUnits(1.0f)
		, Radius(20.0f)
		, Bias(0.15f)
		, PowerExponent(2.0f)
		, SmallScaleAO(1.0f)
		, LargeScaleAO(1.0f)
		, EnableBlur(true)
		, BlurSharpness(16.0f)
		, NumSteps(16)
	{
	}

	bool	EnableHBAOPlus;
	bool	EnableNormals;

	float	MetersToViewSpaceUnits;

	float	Radius;
	float	Bias;
	float	PowerExponent;
	float	SmallScaleAO;
	float	LargeScaleAO;
	bool	EnableBlur;
	float	BlurSharpness;

	int NumSteps;
};

class GFSDK_SSAO_Context_D3D12;

class CHBAOPlus
{
public:
	CHBAOPlus();

	void Initialize(UINT Width, UINT Height, const SHBAOPlusParams& HBAOPlusParams);
	void Cleanup();

	float Render(const XMMATRIX& matProj, const XMMATRIX& matView, CDepthBuffer* pDepthBuffer, CColorBuffer* pNormBuffer, bool debugTimer);

	CColorBuffer& GetAoMap() { return m_HBAOPlusRT; }
	SHBAOPlusParams& FetchHBAOParams() { return m_HBAOParams; }

private:
	GFSDK_SSAO_Context_D3D12* m_pAOContext = nullptr;
	CColorBuffer m_NormMap;
	CColorBuffer m_HBAOPlusRT;
	SHBAOPlusParams m_HBAOParams;
	CUserDescriptorHeap m_HBAOPlusCBVSRVUAVHeap;
	CUserDescriptorHeap m_HBAOPlusRTVHeap;

	CDecodeNormalEffect m_DecodeNormalEffect;
};