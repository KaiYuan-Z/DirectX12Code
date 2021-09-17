#pragma once

#include "GpuResource.h"

class CPixelBuffer : public CGpuResource
{
public:
    CPixelBuffer() : m_Width(0), m_Height(0), m_DepthOrArraySize(0), m_Format(DXGI_FORMAT_UNKNOWN) {}

    uint32_t GetWidth(void) const { return m_Width; }
    uint32_t GetHeight(void) const { return m_Height; }
    uint32_t GetDepthOrArraySize(void) const { return m_DepthOrArraySize; }
    const DXGI_FORMAT& GetFormat(void) const { return m_Format; }

    // Write the raw pixel buffer contents to a file
    // Note that data is preceded by a 16-byte header:  { DXGI_FORMAT, Pitch (in pixels), Width (in pixels), Height }
    void ExportToFile( const std::wstring& FilePath );

protected:
    D3D12_RESOURCE_DESC _DescribeTex2D(uint32_t Width, uint32_t Height, uint32_t ArraySize, uint32_t NumMips, DXGI_FORMAT Format, UINT Flags);
	D3D12_RESOURCE_DESC _DescribeTex3D(uint32_t Width, uint32_t Height, uint32_t Depth, uint32_t NumMips, DXGI_FORMAT Format, UINT Flags);

    void _AssociateWithResource( ID3D12Device* Device, const std::wstring& Name, ID3D12Resource* Resource, D3D12_RESOURCE_STATES CurrentState );

    void _CreateTextureResource( ID3D12Device* Device, const std::wstring& Name, const D3D12_RESOURCE_DESC& ResourceDesc,
        D3D12_CLEAR_VALUE ClearValue, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN );

    static DXGI_FORMAT _GetBaseFormat( DXGI_FORMAT Format );
    static DXGI_FORMAT _GetUAVFormat( DXGI_FORMAT Format );
    static DXGI_FORMAT _GetDSVFormat( DXGI_FORMAT Format );
    static DXGI_FORMAT _GetDepthFormat( DXGI_FORMAT Format );
    static DXGI_FORMAT _GetStencilFormat( DXGI_FORMAT Format );
    static size_t _GetBytesPerPixel( DXGI_FORMAT Format );

    uint32_t m_Width;
    uint32_t m_Height;
    uint32_t m_DepthOrArraySize;
    DXGI_FORMAT m_Format;
};
