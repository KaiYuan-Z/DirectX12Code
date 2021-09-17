#pragma once

#include <dxgi1_6.h>
#include <d3d12.h>
#include <D3Dcompiler.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

#define MY_IID_PPV_ARGS IID_PPV_ARGS
#define D3D12_GPU_VIRTUAL_ADDRESS_NULL      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

#include "d3dx12.h"

#include <DirectXMath.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

