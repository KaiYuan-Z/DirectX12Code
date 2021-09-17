#pragma once
#include "PixelBuffer.h"

class CDepthBuffer : public CPixelBuffer
{
public:
    CDepthBuffer( float clearDepth = 0.0f, uint8_t clearStencil = 0 )
        : m_ClearDepth(clearDepth), m_ClearStencil(clearStencil) 
    {
        m_hDSV[0].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        m_hDSV[1].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        m_hDSV[2].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        m_hDSV[3].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        m_hDepthSRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        m_hStencilSRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    }

    // Create a depth buffer.  If an address is supplied, memory will not be allocated.
    // The vmem address allows you to alias buffers (which can be especially useful for
    // reusing ESRAM across a frame.)
    void Create2D( const std::wstring& Name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format,
        D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN );

    void Create2D(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumSamples, DXGI_FORMAT Format,
        D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN );

    // Get pre-created CPU-visible descriptor handles
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV() const { return m_hDSV[0]; }
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV_DepthReadOnly() const { return m_hDSV[1]; }
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV_StencilReadOnly() const { return m_hDSV[2]; }
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV_ReadOnly() const { return m_hDSV[3]; }
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetDepthSRV() const { return m_hDepthSRV; }
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetStencilSRV() const { return m_hStencilSRV; }

	void SetClearDepth(float ClearDepthValue) { m_ClearDepth = ClearDepthValue; }
	void SetClearStencil(uint8_t ClearStencilValue) { m_ClearStencil = ClearStencilValue; }

    float GetClearDepth() const { return m_ClearDepth; }
    uint8_t GetClearStencil() const { return m_ClearStencil; }

private:
    void __CreateDerivedViews( ID3D12Device* Device, DXGI_FORMAT Format);

    float m_ClearDepth;
    uint8_t m_ClearStencil;
    D3D12_CPU_DESCRIPTOR_HANDLE m_hDSV[4];
    D3D12_CPU_DESCRIPTOR_HANDLE m_hDepthSRV;
    D3D12_CPU_DESCRIPTOR_HANDLE m_hStencilSRV;
};
