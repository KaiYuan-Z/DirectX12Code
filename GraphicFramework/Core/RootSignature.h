#pragma once
#include<map>
#include "Utility.h"

#define INVALID_ROOT_PARAMETER_TYPE (D3D12_ROOT_PARAMETER_TYPE)(-1)

using Microsoft::WRL::ComPtr;

class DescriptorCache;

class CRootParameter
{
    friend class CRootSignature;
public:

    CRootParameter() 
    {
        m_RootParam.ParameterType = INVALID_ROOT_PARAMETER_TYPE;
    }

    ~CRootParameter()
    {
        Clear();
    }

    void Clear()
    {
        if (m_RootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
            delete [] m_RootParam.DescriptorTable.pDescriptorRanges;

        m_RootParam.ParameterType = INVALID_ROOT_PARAMETER_TYPE;
    }

    void InitAsConstants( UINT Register, UINT NumDwords, UINT Space = 0, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL )
    {
        m_RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        m_RootParam.ShaderVisibility = Visibility;
        m_RootParam.Constants.Num32BitValues = NumDwords;
        m_RootParam.Constants.ShaderRegister = Register;
        m_RootParam.Constants.RegisterSpace = Space;
    }

    void InitAsConstantBuffer( UINT Register, UINT Space = 0, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL )
    {
        m_RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        m_RootParam.ShaderVisibility = Visibility;
        m_RootParam.Descriptor.ShaderRegister = Register;
        m_RootParam.Descriptor.RegisterSpace = Space;
    }

    void InitAsBufferSRV( UINT Register, UINT Space = 0, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL )
    {
        m_RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        m_RootParam.ShaderVisibility = Visibility;
        m_RootParam.Descriptor.ShaderRegister = Register;
        m_RootParam.Descriptor.RegisterSpace = Space;
    }

    void InitAsBufferUAV( UINT Register, UINT Space = 0, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL )
    {
        m_RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
        m_RootParam.ShaderVisibility = Visibility;
        m_RootParam.Descriptor.ShaderRegister = Register;
        m_RootParam.Descriptor.RegisterSpace = Space;
    }

    void InitAsDescriptorRange( D3D12_DESCRIPTOR_RANGE_TYPE Type, UINT Register, UINT Count, UINT Space = 0, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL )
    {
        InitAsDescriptorTable(1, Visibility);
        SetTableRange(0, Type, Register, Count, Space);
    }

    void InitAsDescriptorTable( UINT RangeCount, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL )
    {
        m_RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        m_RootParam.ShaderVisibility = Visibility;
        m_RootParam.DescriptorTable.NumDescriptorRanges = RangeCount;
        m_RootParam.DescriptorTable.pDescriptorRanges = new D3D12_DESCRIPTOR_RANGE[RangeCount];
    }

    void SetTableRange( UINT RangeIndex, D3D12_DESCRIPTOR_RANGE_TYPE Type, UINT Register, UINT Count, UINT Space = 0 )
    {
        D3D12_DESCRIPTOR_RANGE* range = const_cast<D3D12_DESCRIPTOR_RANGE*>(m_RootParam.DescriptorTable.pDescriptorRanges + RangeIndex);
        range->RangeType = Type;
        range->NumDescriptors = Count;
        range->BaseShaderRegister = Register;
        range->RegisterSpace = Space;
        range->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    }

    const D3D12_ROOT_PARAMETER& operator() ( void ) const { return m_RootParam; }
        

protected:

    D3D12_ROOT_PARAMETER m_RootParam;
};

// Maximum 64 DWORDS divied up amongst all root parameters.
// Root constants = 1 DWORD * NumConstants
// Root descriptor (CBV, SRV, or UAV) = 2 DWORDs each
// Descriptor table pointer = 1 DWORD
// Static samplers = 0 DWORDS (compiled into shader)
class CRootSignature
{
    friend class CDynamicDescriptorHeap;

public:

	static void sInitialize(ID3D12Device* pDevice);
	static void sShutdown();

    CRootSignature( UINT NumRootParams = 0, UINT NumStaticSamplers = 0 ) : m_Finalized(FALSE), m_NumParameters(NumRootParams)
    {
        Reset(NumRootParams, NumStaticSamplers);
    }

    ~CRootSignature()
    {
    }

    void Reset( UINT NumRootParams, UINT NumStaticSamplers = 0 )
    {
        if (NumRootParams > 0)
            m_ParamArray.reset(new CRootParameter[NumRootParams]);
        else
            m_ParamArray = nullptr;
        m_NumParameters = NumRootParams;

        if (NumStaticSamplers > 0)
            m_SamplerArray.reset(new D3D12_STATIC_SAMPLER_DESC[NumStaticSamplers]);
        else
            m_SamplerArray = nullptr;
        m_NumSamplers = NumStaticSamplers;
        m_NumInitializedStaticSamplers = 0;
    }

    CRootParameter& operator[] ( size_t EntryIndex )
    {
        ASSERT(EntryIndex < m_NumParameters);
        return m_ParamArray.get()[EntryIndex];
    }

    const CRootParameter& operator[] ( size_t EntryIndex ) const
    {
        ASSERT(EntryIndex < m_NumParameters);
        return m_ParamArray.get()[EntryIndex];
    }

    void InitStaticSampler( UINT Register, UINT Space, const D3D12_SAMPLER_DESC& NonStaticSamplerDesc,
        D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL );

    void Finalize(const std::wstring& name, D3D12_ROOT_SIGNATURE_FLAGS Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);

    ID3D12RootSignature* GetSignature() const { return m_Signature; }

	UINT GetRootParameterCount() const { return m_NumParameters; }

	UINT GetStaticSamplerCount() const { return m_NumSamplers; }

protected:

    BOOL m_Finalized;
    UINT m_NumParameters;
    UINT m_NumSamplers;
    UINT m_NumInitializedStaticSamplers;
    uint32_t m_DescriptorTableBitMap;		// One bit is set for root parameters that are non-sampler descriptor tables
    uint32_t m_SamplerTableBitMap;			// One bit is set for root parameters that are sampler descriptor tables
    uint32_t m_DescriptorTableSize[16];		// Non-sampler descriptor tables need to know their descriptor count
    std::unique_ptr<CRootParameter[]> m_ParamArray;
    std::unique_ptr<D3D12_STATIC_SAMPLER_DESC[]> m_SamplerArray;
    ID3D12RootSignature* m_Signature;

	static bool sm_IsInitialized;
	static ID3D12Device* sm_pDevice;
	static std::map<size_t, ComPtr<ID3D12RootSignature>> sm_RootSignatureHashMap;
};
