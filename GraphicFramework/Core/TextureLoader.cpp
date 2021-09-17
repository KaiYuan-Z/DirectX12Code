#include "TextureLoader.h"
#include "FileUtility.h"
#include "DDSTextureLoader.h"
#include "GraphicsCore.h"
#include <map>
#include <thread>

using namespace std;

namespace TextureLoader
{
	wstring s_RootPath = L"";
    map< wstring, unique_ptr<CManagedTexture> > s_TextureCache;

    void SetTextureLibRoot( const std::wstring& TextureLibRoot )
    {
		s_RootPath = TextureLibRoot;
    }

    void ClearTextureCache( void )
    {
		s_TextureCache.clear();
    }

    pair<CManagedTexture*, bool> FindOrLoadTexture( const wstring& FileName )
    {
        static mutex s_Mutex;
        lock_guard<mutex> Guard(s_Mutex);

        auto iter = s_TextureCache.find(FileName);

        // If it's found, it has already been loaded or the load process has begun
        if (iter != s_TextureCache.end())
            return make_pair(iter->second.get(), false);

        CManagedTexture* NewTexture = new CManagedTexture(FileName);
        s_TextureCache[FileName].reset( NewTexture );

        // This was the first time it was requested, so indicate that the caller must read the file
        return make_pair(NewTexture, true);
    }

    const CTexture& GetBlackTex2D(void)
    {
        auto ManagedTex = FindOrLoadTexture(L"DefaultBlackTexture");

        CManagedTexture* ManTex = ManagedTex.first;
        const bool RequestsLoad = ManagedTex.second;

        if (!RequestsLoad)
        {
            ManTex->WaitForLoad();
            return *ManTex;
        }

        uint32_t BlackPixel = 0;
        ManTex->Create2D(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &BlackPixel);
        return *ManTex;
    }

    const CTexture& GetWhiteTex2D(void)
    {
        auto ManagedTex = FindOrLoadTexture(L"DefaultWhiteTexture");

        CManagedTexture* ManTex = ManagedTex.first;
        const bool RequestsLoad = ManagedTex.second;

        if (!RequestsLoad)
        {
            ManTex->WaitForLoad();
            return *ManTex;
        }

        uint32_t WhitePixel = 0xFFFFFFFFul;
        ManTex->Create2D(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &WhitePixel);
        return *ManTex;
    }

    const CTexture& GetMagentaTex2D(void)
    {
        auto ManagedTex = FindOrLoadTexture(L"DefaultMagentaTexture");

        CManagedTexture* ManTex = ManagedTex.first;
        const bool RequestsLoad = ManagedTex.second;

        if (!RequestsLoad)
        {
            ManTex->WaitForLoad();
            return *ManTex;
        }

        uint32_t MagentaPixel = 0x00FF00FF;
        ManTex->Create2D(1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, &MagentaPixel);
        return *ManTex;
    }

} // namespace TextureLoader

void CManagedTexture::WaitForLoad( void ) const
{
    volatile D3D12_CPU_DESCRIPTOR_HANDLE& VolHandle = (volatile D3D12_CPU_DESCRIPTOR_HANDLE&)m_hCpuDescriptorHandle;
    volatile bool& VolValid = (volatile bool&)m_IsValid;
    while (VolHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN && VolValid)
        this_thread::yield();
}

void CManagedTexture::SetToInvalidTexture( void )
{
    m_hCpuDescriptorHandle = TextureLoader::GetMagentaTex2D().GetSRV();
    m_IsValid = false;
}

const CManagedTexture* TextureLoader::LoadFromFile( const std::wstring& FileName, bool sRGB )
{
    std::wstring CatPath = FileName;

    const CManagedTexture* Tex = LoadDDSFromFile( CatPath + L".dds", sRGB );
    if (!Tex->IsValid())
        Tex = LoadTGAFromFile( CatPath + L".tga", sRGB );

    return Tex;
}

const CManagedTexture* TextureLoader::LoadDDSFromFile( const std::wstring& FileName, bool sRGB )
{
    auto ManagedTex = FindOrLoadTexture(FileName);

    CManagedTexture* ManTex = ManagedTex.first;
    const bool RequestsLoad = ManagedTex.second;

    if (!RequestsLoad)
    {
        ManTex->WaitForLoad();
        return ManTex;
    }

    Utility::ByteArray ba = Utility::ReadFileSync( s_RootPath + FileName );
    if (ba->size() == 0 || !ManTex->CreateDDSFromMemory( ba->data(), ba->size(), sRGB ))
        ManTex->SetToInvalidTexture();
    else
        ManTex->ReSetResourceName(FileName.c_str());

    return ManTex;
}

const CManagedTexture* TextureLoader::LoadTGAFromFile( const std::wstring& FileName, bool sRGB )
{
    auto ManagedTex = FindOrLoadTexture(FileName);

    CManagedTexture* ManTex = ManagedTex.first;
    const bool RequestsLoad = ManagedTex.second;

    if (!RequestsLoad)
    {
        ManTex->WaitForLoad();
        return ManTex;
    }

    Utility::ByteArray ba = Utility::ReadFileSync( s_RootPath + FileName );
    if (ba->size() > 0)
    {
        ManTex->CreateTGAFromMemory( ba->data(), ba->size(), sRGB );
        ManTex->ReSetResourceName(FileName.c_str());
    }
    else
        ManTex->SetToInvalidTexture();

    return ManTex;
}


const CManagedTexture* TextureLoader::LoadPIXImageFromFile( const std::wstring& FileName )
{
    auto ManagedTex = FindOrLoadTexture(FileName);

    CManagedTexture* ManTex = ManagedTex.first;
    const bool RequestsLoad = ManagedTex.second;

    if (!RequestsLoad)
    {
        ManTex->WaitForLoad();
        return ManTex;
    }

    Utility::ByteArray ba = Utility::ReadFileSync( s_RootPath + FileName );
    if (ba->size() > 0)
    {
        ManTex->CreatePIXImageFromMemory(ba->data(), ba->size());
        ManTex->ReSetResourceName(FileName.c_str());
    }
    else
        ManTex->SetToInvalidTexture();

    return ManTex;
}
