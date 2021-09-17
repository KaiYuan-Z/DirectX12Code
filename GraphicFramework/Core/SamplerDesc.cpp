#include "SamplerDesc.h"
#include "GraphicsCore.h"
#include "Hash.h"
#include <map>

using namespace std;

namespace
{
    map< size_t, D3D12_CPU_DESCRIPTOR_HANDLE > s_SamplerCache;
}

D3D12_CPU_DESCRIPTOR_HANDLE CSamplerDesc::CreateDescriptor()
{
    size_t hashValue = Utility::HashState(this);
    auto iter = s_SamplerCache.find(hashValue);
    if (iter != s_SamplerCache.end())
    {
        return iter->second;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE Handle = GraphicsCore::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	GraphicsCore::GetD3DDevice()->CreateSampler(this, Handle);
    return Handle;
}

void CSamplerDesc::CreateDescriptor( D3D12_CPU_DESCRIPTOR_HANDLE& Handle )
{
	GraphicsCore::GetD3DDevice()->CreateSampler(this, Handle);
}
