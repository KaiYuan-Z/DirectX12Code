#pragma once
#include "D3D12Base.h"
#include "Win32Base.h"

using Microsoft::WRL::ComPtr;

namespace Utility
{
    inline void Print( const char* msg ) { printf("%s", msg); }
    inline void Print( const wchar_t* msg ) { wprintf(L"%ws", msg); }

    inline void Printf( const char* format, ... )
    {
        char buffer[256];
        va_list ap;
        va_start(ap, format);
        vsprintf_s(buffer, 256, format, ap);
        Print(buffer);
    }

    inline void Printf( const wchar_t* format, ... )
    {
        wchar_t buffer[256];
        va_list ap;
        va_start(ap, format);
        vswprintf(buffer, 256, format, ap);
        Print(buffer);
    }

#ifndef RELEASE
    inline void PrintSubMessage( const char* format, ... )
    {
        Print("--> ");
        char buffer[256];
        va_list ap;
        va_start(ap, format);
        vsprintf_s(buffer, 256, format, ap);
        Print(buffer);
        Print("\n");
    }
    inline void PrintSubMessage( const wchar_t* format, ... )
    {
        Print("--> ");
        wchar_t buffer[256];
        va_list ap;
        va_start(ap, format);
        vswprintf(buffer, 256, format, ap);
        Print(buffer);
        Print("\n");
    }
    inline void PrintSubMessage( void )
    {
    }
#endif

} // namespace Utility

#ifdef ERROR
#undef ERROR
#endif
#ifdef ASSERT
#undef ASSERT
#endif
#ifdef HALT
#undef HALT
#endif

#define HALT( ... ) ERROR( __VA_ARGS__ ) __debugbreak();

#define __EXCEPTION_SITE__    __FUNCTION__, __FILE__, __LINE__
#define __FUNCTION_SITE__     __FUNCTION__, __FILE__, __LINE__

#ifdef RELEASE

    #define ASSERT( isTrue, ... ) (void)(isTrue)
    #define WARN_ONCE_IF( isTrue, ... ) (void)(isTrue)
    #define WARN_ONCE_IF_NOT( isTrue, ... ) (void)(isTrue)
    #define ERROR( msg, ... )
    #define DEBUGPRINT( msg, ... ) do {} while(0)
    #define ASSERT_SUCCEEDED( hr, ... ) (void)(hr)

#else	// !RELEASE

    #define STRINGIFY(x) #x
    #define STRINGIFY_BUILTIN(x) STRINGIFY(x)
    #define ASSERT( isFalse, ... ) \
        if (!(bool)(isFalse)) { \
            Utility::Print("\nAssertion failed in " STRINGIFY_BUILTIN(__FILE__) " @ " STRINGIFY_BUILTIN(__LINE__) "\n"); \
            Utility::PrintSubMessage("\'" #isFalse "\' is false"); \
            Utility::PrintSubMessage(__VA_ARGS__); \
            Utility::Print("\n"); \
            __debugbreak(); \
        }

    #define ASSERT_SUCCEEDED( hr, ... ) \
        if (FAILED(hr)) { \
            Utility::Print("\nHRESULT failed in " STRINGIFY_BUILTIN(__FILE__) " @ " STRINGIFY_BUILTIN(__LINE__) "\n"); \
            Utility::PrintSubMessage("hr = 0x%08X", hr); \
            Utility::PrintSubMessage(__VA_ARGS__); \
            Utility::Print("\n"); \
            __debugbreak(); \
        }


    #define WARN_ONCE_IF( isTrue, ... ) \
    { \
        static bool s_TriggeredWarning = false; \
        if ((bool)(isTrue) && !s_TriggeredWarning) { \
            s_TriggeredWarning = true; \
            Utility::Print("\nWarning issued in " STRINGIFY_BUILTIN(__FILE__) " @ " STRINGIFY_BUILTIN(__LINE__) "\n"); \
            Utility::PrintSubMessage("\'" #isTrue "\' is true"); \
            Utility::PrintSubMessage(__VA_ARGS__); \
            Utility::Print("\n"); \
        } \
    }

    #define WARN_ONCE_IF_NOT( isTrue, ... ) WARN_ONCE_IF(!(isTrue), __VA_ARGS__)

    #define ERROR( ... ) \
        Utility::Print("\nError reported in " STRINGIFY_BUILTIN(__FILE__) " @ " STRINGIFY_BUILTIN(__LINE__) "\n"); \
        Utility::PrintSubMessage(__VA_ARGS__); \
        Utility::Print("\n");

    #define DEBUG_PRINT( msg, ... ) \
    Utility::Printf( msg "\n", ##__VA_ARGS__ );

#endif

void SIMDMemCopy( void* __restrict Dest, const void* __restrict Source, size_t NumQuadwords );
void SIMDMemFill( void* __restrict Dest, __m128 FillVector, size_t NumQuadwords );

void MemCopy(void* __restrict Dest, const void* __restrict Source, size_t SizeInBytes);

std::wstring MakeWStr( const std::string& str );
std::string MakeStr(const std::wstring& wstr);

class CHrException : public std::runtime_error
{
	inline std::string ToString(const std::string& source, const std::string& file, int line, HRESULT hr)
	{
		_com_error err(hr);
		return "Function [" + source + "] failed in file [" + file + "], line [" + std::to_string(line)+"], error message ["+ MakeStr(err.ErrorMessage())+"]";
	}

public:

	CHrException(const std::string& source, const std::string& file, int line, HRESULT hr)
		: std::runtime_error(ToString(source, file, line, hr)), m_hr(hr)
	{
	}

	HRESULT Error() const { return m_hr; }

private:

	const HRESULT m_hr;
};

#define SAFE_RELEASE(p) if (p) (p)->Release()
#define SAFE_DELETE(p) if (p) delete p
#define SAFE_DELETE_ARRAY(p) if (p) delete[] p

#define BREAK_IF_FAILED( hr ) if (FAILED(hr)) __debugbreak()

#define THROW_IF_FAILED(hr) if(FAILED(hr)) throw CHrException(__EXCEPTION_SITE__, hr)
#define THROW_MSG_IF_FAILED(hr, msg) if(FAILED(hr)) { OutputDebugString(msg); throw CHrException(__EXCEPTION_SITE__, hr); }
#define THROW_IF_FALSE(value) THROW_IF_FAILED(value ? S_OK : E_FAIL)
#define THROW_MSG_IF_FALSE(value, msg) THROW_MSG_IF_FAILED(value ? S_OK : E_FAIL, msg)

inline void GetAssetsPath(_Out_writes_(pathSize) WCHAR* path, UINT pathSize)
{
	if (path == nullptr) throw std::exception();

	DWORD size = GetModuleFileName(nullptr, path, pathSize);
	if (size == 0 || size == pathSize)
	{
		// Method failed or path was truncated.
		throw std::exception();
	}

	WCHAR* lastSlash = wcsrchr(path, L'\\');
	if (lastSlash)
	{
		*(lastSlash + 1) = L'\0';
	}
}

inline HRESULT ReadDataFromFile(LPCWSTR filename, byte** data, UINT* size)
{
	using namespace Microsoft::WRL;

	CREATEFILE2_EXTENDED_PARAMETERS extendedParams = {};
	extendedParams.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
	extendedParams.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
	extendedParams.dwFileFlags = FILE_FLAG_SEQUENTIAL_SCAN;
	extendedParams.dwSecurityQosFlags = SECURITY_ANONYMOUS;
	extendedParams.lpSecurityAttributes = nullptr;
	extendedParams.hTemplateFile = nullptr;

	Wrappers::FileHandle file(CreateFile2(filename, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, &extendedParams));
	if (file.Get() == INVALID_HANDLE_VALUE)
	{
		throw std::exception();
	}

	FILE_STANDARD_INFO fileInfo = {};
	if (!GetFileInformationByHandleEx(file.Get(), FileStandardInfo, &fileInfo, sizeof(fileInfo)))
	{
		throw std::exception();
	}

	if (fileInfo.EndOfFile.HighPart != 0)
	{
		throw std::exception();
	}

	*data = reinterpret_cast<byte*>(malloc(fileInfo.EndOfFile.LowPart));
	*size = fileInfo.EndOfFile.LowPart;

	if (!ReadFile(file.Get(), *data, fileInfo.EndOfFile.LowPart, nullptr, nullptr))
	{
		throw std::exception();
	}

	return S_OK;
}

// Assign a name to the object to aid with debugging.
#if defined(_DEBUG) || defined(DBG)
inline void SetName(ID3D12Object* pObject, LPCWSTR name)
{
	pObject->SetName(name);
}
inline void SetNameIndexed(ID3D12Object* pObject, LPCWSTR name, UINT index)
{
	WCHAR fullName[50];
	if (swprintf_s(fullName, L"%s[%u]", name, index) > 0)
	{
		pObject->SetName(fullName);
	}
}
#else
inline void SetName(ID3D12Object*, LPCWSTR)
{
}
inline void SetNameIndexed(ID3D12Object*, LPCWSTR, UINT)
{
}
#endif

// Naming helper for ComPtr<T>.
// Assigns the name of the variable as the name of the object.
// The indexed variant will include the index in the name of the object.
#define NAME_D3D12_OBJECT(x) SetName((x).Get(), L#x)
#define NAME_D3D12_OBJECT_INDEXED(x, n) SetNameIndexed((x)[n].Get(), L#x, n)

inline UINT Align(UINT size, UINT alignment)
{
	return (size + (alignment - 1)) & ~(alignment - 1);
}

inline UINT CalculateConstantBufferByteSize(UINT byteSize)
{
	// Constant buffer size is required to be aligned.
	return Align(byteSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
}

inline Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
	const std::wstring& filename,
	const D3D_SHADER_MACRO* defines,
	const std::string& entrypoint,
	const std::string& target)
{
	UINT compileFlags = 0;
#if defined(_DEBUG) || defined(DBG)
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	HRESULT hr;

	Microsoft::WRL::ComPtr<ID3DBlob> byteCode = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errors;
	hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

	if (errors != nullptr)
	{
		OutputDebugStringA((char*)errors->GetBufferPointer());
	}
	THROW_IF_FAILED(hr);

	return byteCode;
}

// Resets all elements in a ComPtr array.
template<class T>
inline void ResetComPtrArray(T* comPtrArray)
{
	for (auto &i : *comPtrArray)
	{
		i.Reset();
	}
}


// Resets all elements in a unique_ptr array.
template<class T>
inline void ResetUniquePtrArray(T* uniquePtrArray)
{
	for (auto &i : *uniquePtrArray)
	{
		i.reset();
	}
}

inline UINT CeilDivide(UINT value, UINT divisor)
{ 
	return (value + divisor - 1) / divisor;
}

inline UINT NumMantissaBitsInFloatFormat(UINT FloatFormatBitLength)
{
	switch (FloatFormatBitLength)
	{
	case 32: return 23;
	case 16: return 10;
	case 11: return 6;
	case 10: return 5;
	}

	return 0;
}