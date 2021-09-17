#pragma once
#include "GraphicsCoreBase.h"
#include "PixelBuffer.h"
#include "Color.h"

class CColorBuffer : public CPixelBuffer
{
public:
    CColorBuffer( CColor ClearColor = CColor(0.0f, 0.0f, 0.0f, 0.0f)  )
        : m_ClearColor(ClearColor), m_NumMipMaps(0), m_FragmentCount(1), m_SampleCount(1)
    {
        m_SRVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        m_RTVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        std::memset(m_UAVHandle, 0xFF, sizeof(m_UAVHandle));
    }

    // Create a color buffer from a swap chain buffer.  Unordered access is restricted.
    void Create2DFromSwapChain( const std::wstring& Name, ID3D12Resource* BaseResource );

    // Create a color buffer.  If an address is supplied, memory will not be allocated.
    // The vmem address allows you to alias buffers (which can be especially useful for
    // reusing ESRAM across a frame.).  
    void Create2D(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumMips,
        DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

    // Create a color buffer.  If an address is supplied, memory will not be allocated.
    // The vmem address allows you to alias buffers 
    void Create2DArray(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount,
        DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

	// Create a color buffer.  If an address is supplied, memory will not be allocated.
// The vmem address allows you to alias buffers 
	void Create3D(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t Depth,
		DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

    // Get pre-created CPU-visible descriptor handles
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV(void) const { return m_SRVHandle; }
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetRTV(void) const { return m_RTVHandle; }
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetUAV(void) const { return m_UAVHandle[0]; }

    void SetClearColor( CColor ClearColor ) { m_ClearColor = ClearColor; }

    void SetMsaaMode( uint32_t NumColorSamples, uint32_t NumCoverageSamples )
    {
        ASSERT(NumCoverageSamples >= NumColorSamples);
        m_FragmentCount = NumColorSamples;
        m_SampleCount = NumCoverageSamples;
    }

    CColor GetClearColor(void) const { return m_ClearColor; }

    // This will work for all texture sizes, but it's recommended for speed and quality
    // that you use dimensions with powers of two (but not necessarily square.)  Pass
    // 0 for ArrayCount to reserve space for mips at creation time.
    //void GenerateMipMaps(CCommandList& CommandList);

protected:

    D3D12_RESOURCE_FLAGS _CombineResourceFlags( void ) const
    {
        D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE;

        if (Flags == D3D12_RESOURCE_FLAG_NONE && m_FragmentCount == 1)
            Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        return D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | Flags;
    }

    // Compute the number of texture levels needed to reduce to 1x1.  This uses
    // _BitScanReverse to find the highest set bit.  Each dimension reduces by
    // half and truncates bits.  The dimension 256 (0x100) has 9 mip levels, same
    // as the dimension 511 (0x1FF).
    static inline uint32_t _ComputeNumMips(uint32_t Width, uint32_t Height)
    {
        uint32_t HighBit;
        _BitScanReverse((unsigned long*)&HighBit, Width | Height);
        return HighBit + 1;
    }

	void _CreateTexture2DViews(DXGI_FORMAT Format, uint32_t NumMips = 1);
	void _CreateTexture2DArrayViews(DXGI_FORMAT Format, uint32_t ArraySize, uint32_t NumMips = 1);
	void _CreateTexture3DViews(DXGI_FORMAT Format, uint32_t Depth, uint32_t NumMips = 1);

    CColor m_ClearColor;
    D3D12_CPU_DESCRIPTOR_HANDLE m_SRVHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE m_RTVHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE m_UAVHandle[12];
    uint32_t m_NumMipMaps; // number of texture sublevels
    uint32_t m_FragmentCount;
    uint32_t m_SampleCount;
};
