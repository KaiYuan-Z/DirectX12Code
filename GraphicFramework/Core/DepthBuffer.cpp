#include "DepthBuffer.h"
#include "GraphicsCore.h"

void CDepthBuffer::Create2D( const std::wstring& Name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr )
{
    D3D12_RESOURCE_DESC ResourceDesc = _DescribeTex2D(Width, Height, 1, 1, Format, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    D3D12_CLEAR_VALUE ClearValue = {};
    ClearValue.Format = Format;
	ClearValue.DepthStencil.Depth = 1.0f;
	ClearValue.DepthStencil.Stencil = 0;
    _CreateTextureResource(GraphicsCore::GetD3DDevice(), Name, ResourceDesc, ClearValue, VidMemPtr);
    __CreateDerivedViews(GraphicsCore::GetD3DDevice(), Format);
}

void CDepthBuffer::Create2D( const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t Samples, DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr )
{
    D3D12_RESOURCE_DESC ResourceDesc = _DescribeTex2D(Width, Height, 1, 1, Format, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    ResourceDesc.SampleDesc.Count = Samples;

    D3D12_CLEAR_VALUE ClearValue = {};
    ClearValue.Format = Format;
	ClearValue.DepthStencil.Depth = 1.0f;
	ClearValue.DepthStencil.Stencil = 0;
    _CreateTextureResource(GraphicsCore::GetD3DDevice(), Name, ResourceDesc, ClearValue, VidMemPtr);
    __CreateDerivedViews(GraphicsCore::GetD3DDevice(), Format);
}

void CDepthBuffer::__CreateDerivedViews( ID3D12Device* Device, DXGI_FORMAT Format )
{
    ID3D12Resource* Resource = m_pResource.Get();

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    dsvDesc.Format = _GetDSVFormat(Format);
    if (Resource->GetDesc().SampleDesc.Count == 1)
    {
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;
    }
    else
    {
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
    }

    if (m_hDSV[0].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
    {
        m_hDSV[0] = GraphicsCore::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        m_hDSV[1] = GraphicsCore::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    }

    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSV[0]);

    dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
    Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSV[1]);

    DXGI_FORMAT stencilReadFormat = _GetStencilFormat(Format);
    if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
    {
        if (m_hDSV[2].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
        {
            m_hDSV[2] = GraphicsCore::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
            m_hDSV[3] = GraphicsCore::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        }

        dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_STENCIL;
        Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSV[2]);

        dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL;
        Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSV[3]);
    }
    else
    {
        m_hDSV[2] = m_hDSV[0];
        m_hDSV[3] = m_hDSV[1];
    }

    if (m_hDepthSRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
        m_hDepthSRV = GraphicsCore::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Create the shader resource view
    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.Format = _GetDepthFormat(Format);
    if (dsvDesc.ViewDimension == D3D12_DSV_DIMENSION_TEXTURE2D)
    {
        SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        SRVDesc.Texture2D.MipLevels = 1;
    }
    else
    {
        SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
    }
    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    Device->CreateShaderResourceView( Resource, &SRVDesc, m_hDepthSRV );

    //if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
    //{
    //    if (m_hStencilSRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
    //        m_hStencilSRV = GraphicsCore::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    //    SRVDesc.Format = stencilReadFormat;
    //    Device->CreateShaderResourceView( Resource, &SRVDesc, m_hStencilSRV );
    //}
}
