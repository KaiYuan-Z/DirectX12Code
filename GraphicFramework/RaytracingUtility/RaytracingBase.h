#pragma once

#pragma warning(disable:4201) // nonstandard extension used : nameless struct/union
#pragma warning(disable:4328) // nonstandard extension used : class rvalue used as lvalue
#pragma warning(disable:4324) // structure was padded due to __declspec(align())

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
    #define NOMINMAX
#endif
#include <windows.h>

#include <dxgi.h>
#include <dxgi1_2.h>
#include <dxgi1_3.h>
#include <dxgi1_6.h>
#include <atlbase.h>

// C RunTime Header Files
#include <stdlib.h>
#include <sstream>
#include <iomanip>

#include <list>
#include <string>
#include <shellapi.h>
#include <unordered_map>
#include <assert.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <vector>
#include <memory>
#include <exception>
#include <mutex>
#include <thread>

#include <wrl.h>
#include <ppltasks.h>

#include <DirectXMath.h>
#include <DirectXColors.h>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#include "Include/d3d12_1.h"
#include "Include/d3d12video.h"
#include "Include/dxcapi.h"
#include "Include/d3dx12.h"
#include "Include/D3D12RaytracingHelpers.hpp"

#include "../Core/Utility.h"