#include "PipelineState.h"
#include "RootSignature.h"
#include "Hash.h"
#include <map>
#include <thread>
#include <mutex>

using MathUtility::IsAligned;
using Microsoft::WRL::ComPtr;
using namespace std;

static bool s_IsInitialized = false;
static ID3D12Device* s_pDevice = nullptr;
static map< size_t, ComPtr<ID3D12PipelineState> > s_GraphicsPSOHashMap;
static map< size_t, ComPtr<ID3D12PipelineState> > s_ComputePSOHashMap;

void CPipelineStateObject::sInitialize(ID3D12Device* pDevice)
{
	if (!s_IsInitialized)
	{
		_ASSERTE(pDevice);
		s_pDevice = pDevice;
		s_IsInitialized = true;
	}
}

void CPipelineStateObject::sShutdown()
{
	if (s_IsInitialized)
	{
		s_GraphicsPSOHashMap.clear();
		s_ComputePSOHashMap.clear();
		s_IsInitialized = false;
	}
}

CGraphicsPSO::CGraphicsPSO()
{
    ZeroMemory(&m_PSODesc, sizeof(m_PSODesc));
    m_PSODesc.NodeMask = 1;
    m_PSODesc.SampleMask = 0xFFFFFFFFu;
    m_PSODesc.SampleDesc.Count = 1;
    m_PSODesc.InputLayout.NumElements = 0;
}

void CGraphicsPSO::SetBlendState( const D3D12_BLEND_DESC& BlendDesc )
{
    m_PSODesc.BlendState = BlendDesc;
}

void CGraphicsPSO::SetRasterizerState( const D3D12_RASTERIZER_DESC& RasterizerDesc )
{
    m_PSODesc.RasterizerState = RasterizerDesc;
}

void CGraphicsPSO::SetDepthStencilState( const D3D12_DEPTH_STENCIL_DESC& DepthStencilDesc )
{
    m_PSODesc.DepthStencilState = DepthStencilDesc;
}

void CGraphicsPSO::SetSampleMask( UINT SampleMask )
{
    m_PSODesc.SampleMask = SampleMask;
}

void CGraphicsPSO::SetPrimitiveTopologyType( D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType )
{
    ASSERT(TopologyType != D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED, "Can't draw with undefined topology");
    m_PSODesc.PrimitiveTopologyType = TopologyType;
}

void CGraphicsPSO::SetPrimitiveRestart( D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBProps )
{
    m_PSODesc.IBStripCutValue = IBProps;
}

void CGraphicsPSO::SetRenderTargetFormat( DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, UINT MsaaCount, UINT MsaaQuality )
{
    SetRenderTargetFormats(1, &RTVFormat, DSVFormat, MsaaCount, MsaaQuality );
}

void CGraphicsPSO::SetRenderTargetFormats( UINT NumRTVs, const DXGI_FORMAT* RTVFormats, DXGI_FORMAT DSVFormat, UINT MsaaCount, UINT MsaaQuality )
{
    ASSERT(NumRTVs == 0 || RTVFormats != nullptr, "Null format array conflicts with non-zero length");
    for (UINT i = 0; i < NumRTVs; ++i)
        m_PSODesc.RTVFormats[i] = RTVFormats[i];
    for (UINT i = NumRTVs; i < m_PSODesc.NumRenderTargets; ++i)
        m_PSODesc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
    m_PSODesc.NumRenderTargets = NumRTVs;
    m_PSODesc.DSVFormat = DSVFormat;
    m_PSODesc.SampleDesc.Count = MsaaCount;
    m_PSODesc.SampleDesc.Quality = MsaaQuality;
}

void CGraphicsPSO::SetInputLayout( UINT NumElements, const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs )
{
    m_PSODesc.InputLayout.NumElements = NumElements;

    if (NumElements > 0)
    {
        D3D12_INPUT_ELEMENT_DESC* NewElements = (D3D12_INPUT_ELEMENT_DESC*)malloc(sizeof(D3D12_INPUT_ELEMENT_DESC) * NumElements);
        memcpy(NewElements, pInputElementDescs, NumElements * sizeof(D3D12_INPUT_ELEMENT_DESC));
        m_InputLayouts.reset((const D3D12_INPUT_ELEMENT_DESC*)NewElements);
    }
    else
        m_InputLayouts = nullptr;
}

void CGraphicsPSO::Finalize()
{
    // Make sure the root signature is finalized first
    m_PSODesc.pRootSignature = m_RootSignature->GetSignature();
    ASSERT(m_PSODesc.pRootSignature != nullptr);

    m_PSODesc.InputLayout.pInputElementDescs = nullptr;
    size_t HashCode = Utility::HashState(&m_PSODesc);
    HashCode = Utility::HashState(m_InputLayouts.get(), m_PSODesc.InputLayout.NumElements, HashCode);
    m_PSODesc.InputLayout.pInputElementDescs = m_InputLayouts.get();

    ID3D12PipelineState** PSORef = nullptr;
    bool firstCompile = false;
    {
        static mutex s_HashMapMutex;
        lock_guard<mutex> CS(s_HashMapMutex);
        auto iter = s_GraphicsPSOHashMap.find(HashCode);

        // Reserve space so the next inquiry will find that someone got here first.
        if (iter == s_GraphicsPSOHashMap.end())
        {
            firstCompile = true;
            PSORef = s_GraphicsPSOHashMap[HashCode].GetAddressOf();
        }
        else
            PSORef = iter->second.GetAddressOf();
    }

    if (firstCompile)
    {
        ASSERT_SUCCEEDED( s_pDevice->CreateGraphicsPipelineState(&m_PSODesc, MY_IID_PPV_ARGS(&m_PSO)) );
        s_GraphicsPSOHashMap[HashCode].Attach(m_PSO);
    }
    else
    {
        while (*PSORef == nullptr)
            this_thread::yield();
        m_PSO = *PSORef;
    }
}

void CComputePSO::Finalize()
{
    // Make sure the root signature is finalized first
    m_PSODesc.pRootSignature = m_RootSignature->GetSignature();
    ASSERT(m_PSODesc.pRootSignature != nullptr);

    size_t HashCode = Utility::HashState(&m_PSODesc);

    ID3D12PipelineState** PSORef = nullptr;
    bool firstCompile = false;
    {
        static mutex s_HashMapMutex;
        lock_guard<mutex> CS(s_HashMapMutex);
        auto iter = s_ComputePSOHashMap.find(HashCode);

        // Reserve space so the next inquiry will find that someone got here first.
        if (iter == s_ComputePSOHashMap.end())
        {
            firstCompile = true;
            PSORef = s_ComputePSOHashMap[HashCode].GetAddressOf();
        }
        else
            PSORef = iter->second.GetAddressOf();
    }

    if (firstCompile)
    {
        ASSERT_SUCCEEDED( s_pDevice->CreateComputePipelineState(&m_PSODesc, MY_IID_PPV_ARGS(&m_PSO)) );
        s_ComputePSOHashMap[HashCode].Attach(m_PSO);
    }
    else
    {
        while (*PSORef == nullptr)
            this_thread::yield();
        m_PSO = *PSORef;
    }
}

CComputePSO::CComputePSO()
{
    ZeroMemory(&m_PSODesc, sizeof(m_PSODesc));
    m_PSODesc.NodeMask = 1;
}