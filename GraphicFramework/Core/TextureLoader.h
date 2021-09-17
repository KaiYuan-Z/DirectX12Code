#pragma once
#include "Utility.h"
#include "GpuResource.h"
#include "Texture.h"

class CManagedTexture : public CTexture
{
public:
	CManagedTexture(const std::wstring& FileName) : m_MapKey(FileName), m_IsValid(true) {}

    void operator= ( const CTexture& Texture );

	//Ensure safe use under multi-thread.
    void WaitForLoad(void) const;
    void Unload(void);

    void SetToInvalidTexture(void);
    bool IsValid(void) const { return m_IsValid; }

private:
    std::wstring m_MapKey;		// For deleting from the map later
    bool m_IsValid;
};

namespace TextureLoader
{
    void SetTextureLibRoot( const std::wstring& TextureLibRoot );
    void ClearTextureCache(void);

    const CManagedTexture* LoadFromFile( const std::wstring& FileName, bool sRGB = false );
    const CManagedTexture* LoadDDSFromFile( const std::wstring& FileName, bool sRGB = false );
    const CManagedTexture* LoadTGAFromFile( const std::wstring& FileName, bool sRGB = false );
    const CManagedTexture* LoadPIXImageFromFile( const std::wstring& FileName );

    inline const CManagedTexture* LoadFromFile( const std::string& FileName, bool sRGB = false )
    {
        return LoadFromFile(MakeWStr(FileName), sRGB);
    }

    inline const CManagedTexture* LoadDDSFromFile( const std::string& FileName, bool sRGB = false )
    {
        return LoadDDSFromFile(MakeWStr(FileName), sRGB);
    }

    inline const CManagedTexture* LoadTGAFromFile( const std::string& FileName, bool sRGB = false )
    {
        return LoadTGAFromFile(MakeWStr(FileName), sRGB);
    }

    inline const CManagedTexture* LoadPIXImageFromFile( const std::string& FileName )
    {
        return LoadPIXImageFromFile(MakeWStr(FileName));
    }

    const CTexture& GetBlackTex2D(void);
    const CTexture& GetWhiteTex2D(void);
}
