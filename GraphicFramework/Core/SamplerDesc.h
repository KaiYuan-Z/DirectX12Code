#pragma once
#include "D3D12Base.h"
#include "Color.h"

class CSamplerDesc : public D3D12_SAMPLER_DESC
{
public:
    // These defaults match the default values for HLSL-defined root
    // signature static samplers.  So not overriding them here means
    // you can safely not define them in HLSL.
    CSamplerDesc()
    {
        Filter = D3D12_FILTER_ANISOTROPIC;
        AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        MipLODBias = 0.0f;
        MaxAnisotropy = 16;
        ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        BorderColor[0] = 1.0f;
        BorderColor[1] = 1.0f;
        BorderColor[2] = 1.0f;
        BorderColor[3] = 1.0f;
        MinLOD = 0.0f;
        MaxLOD = D3D12_FLOAT32_MAX;
    }

    void SetTextureAddressMode( D3D12_TEXTURE_ADDRESS_MODE AddressMode )
    {
        AddressU = AddressMode;
        AddressV = AddressMode;
        AddressW = AddressMode;
    }

    void SetBorderColor( CColor Border )
    {
        BorderColor[0] = Border.R();
        BorderColor[1] = Border.G();
        BorderColor[2] = Border.B();
        BorderColor[3] = Border.A();
    }

    // __Allocate new descriptor as needed; return handle to existing descriptor when possible
    D3D12_CPU_DESCRIPTOR_HANDLE CreateDescriptor( void );

    // Create descriptor in place (no deduplication)
    void CreateDescriptor( D3D12_CPU_DESCRIPTOR_HANDLE& Handle );
};
