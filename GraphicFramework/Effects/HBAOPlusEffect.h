#pragma once
#include "../Core/MathUtility.h"
#include "../Core/ColorBuffer.h"
#include "../Core/GraphicsCoreBase.h"
#include "../Core/GraphicsCore.h"
#include "../Core/UserDescriptorHeap.h"

#include "DecodeNormalEffect.h"

//==========================================================
//Note��HBAOPlus��ģ�鲢û��ʹ�ñ�����Լ��Ĺ��ߣ�HBAOPlus
// ��Nvidia��DLL�ڲ��Լ�ʵ�ֵĻ��ƣ����Ļ���ʹ�õ�Bufferû��
// ���໺�塣��˵���ĿǰHBAOPlusEffect�Լ�����CommandList��
// ���ύCommandList����HBAOPlusEffect���������ƴ�����ʹ��
// ʱע����CommandList�ύָ���˳�����⡣ǧ������G-Buffer
// ��CommandList��δִ�У�HBAOPlus��CommandList���Ѿ��ύ��
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