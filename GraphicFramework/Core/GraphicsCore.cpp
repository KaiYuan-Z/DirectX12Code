#include "GraphicsCore.h"
#include "DescriptorAllocatorExplorer.h"
#include "DescriptorAllocator.h"
#include "DynamicDescriptorHeap.h"
#include "LinearAllocator.h"
#include "ColorBuffer.h"
#include "TextureLoader.h"
#include "GraphicsCommon.h"
#include "DXSample.h"
#include "DDSTextureLoader.h"
#include "ModelManager.h"
#include "GpuTimeManager.h"

using namespace std;
using namespace GraphicsCore;
using Microsoft::WRL::ComPtr;


UINT												s_AdapterIDoverride = UINT_MAX;
UINT                                                s_CurrentBackBufferIndex = 0;
ComPtr<IDXGIAdapter1>                               s_Adapter;
UINT                                                s_AdapterID = 0;
std::wstring                                        s_AdapterDescription = L"";

// Direct3D objects.
Microsoft::WRL::ComPtr<ID3D12Device>                s_D3dDevice;

// Swap chain objects.
Microsoft::WRL::ComPtr<IDXGIFactory4>               s_DxgiFactory;
Microsoft::WRL::ComPtr<IDXGISwapChain3>             s_SwapChain;
uint64_t											s_RenderTargetFences[MAX_BACK_BUFFER_COUNT] = { 0 };
CColorBuffer										s_RenderTargets[MAX_BACK_BUFFER_COUNT];
CDepthBuffer										s_DepthStencil;

// Direct3D rendering objects.
D3D12_VIEWPORT                                      s_ScreenViewport = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
D3D12_RECT                                          s_ScissorRect = { 0, 0, 1600, 900 };

// Direct3D properties.
DXGI_FORMAT                                         s_BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
DXGI_FORMAT                                         s_DepthBufferFormat = DXGI_FORMAT_D32_FLOAT;
UINT                                                s_BackBufferCount = 3;
D3D_FEATURE_LEVEL                                   s_D3dMinFeatureLevel = D3D_FEATURE_LEVEL_11_0;

// Cached device properties.
D3D_FEATURE_LEVEL                                   s_D3dFeatureLevel = D3D_FEATURE_LEVEL_11_0;
RECT                                                s_WindowRect = { 0, 0, 1600, 900 };
UINT												s_TargetElapsedMicroseconds = 0;
bool                                                s_IsWindowVisible = true;


unsigned int                                        s_Options = 0;// DeviceResources options (see flags above)
HINSTANCE											s_hInstance = 0;
HWND                                                s_Hwnd = 0;
DWORD												s_WindowStyle = WS_OVERLAPPEDWINDOW;
std::wstring										s_WindowTitle = L"Default Title";
long long											s_FrameID = 0;
double												s_FramePerSecond = 0.0;
CStepTimer*											s_pTimer = nullptr;
CDXSample*                                          s_pDXSampleClass = nullptr;
EnableExperimentalFeatureFunctionPointor			s_pFuncEnableExperimentalFeature = nullptr;
CDescriptorAllocatorExplorer*						s_pDescriptorAllocatorExplorer = nullptr;
CCommandManager*									s_pCommandManager = nullptr;
bool												s_IsCoreInitialized = false;
bool												s_EnableGUI = false;
bool												s_ShowFps = false;


// Get formate no SRGB.
extern DXGI_FORMAT NoSRGB(DXGI_FORMAT fmt);
// Initialize D3D12.
extern void InitializeD3D12();
// This method acquires the first available hardware adapter that supports Direct3D 12.If no such adapter can be found, try WARP. Otherwise throw an exception.
extern void InitializeAdapter(IDXGIAdapter1** ppAdapter);
// Configures DXGI Factory and retrieve an adapter.
extern void InitializeDXGIAdapter();
// Configures the Direct3D device, and stores handles to it and the device context.
extern void CreateDeviceResources();
// These resources need to be recreated every time the window size is changed.
extern void CreateWindowSizeDependentResources();
// Prepare to render the next frame.
extern void MoveToNextFrame();
// Destructor for DeviceResources.
extern void ShutdownD3D12();
// Destroy core.
extern void DestroyCore();


// Initialize Timer.
extern void InitializeTimer();
// Create window.
extern void CreateWin32Window();
// Main loop.
extern int MainLoop();
// Window message processer.
extern LRESULT CALLBACK WindowProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam);
// Present the contents of the swap chain to the screen.
extern void Present();
// Calculate frame stats.
extern void CalculateFrameStats();


// Device Accessors.
RECT GraphicsCore::GetWindowRect() { return s_WindowRect; }
UINT GraphicsCore::GetWindowWidth() { return s_WindowRect.right - s_WindowRect.left; }
UINT GraphicsCore::GetWindowHeight() { return s_WindowRect.bottom - s_WindowRect.top; }
bool GraphicsCore::IsWindowVisible() { return s_IsWindowVisible; }
bool GraphicsCore::IsTearingSupported() { return s_Options & c_AllowTearing; }


// Direct3D Accessors.
IDXGIAdapter1*				GraphicsCore::GetAdapter() { return s_Adapter.Get(); }
ID3D12Device*				GraphicsCore::GetD3DDevice() { return s_D3dDevice.Get(); }
IDXGIFactory4*				GraphicsCore::GetDXGIFactory() { return s_DxgiFactory.Get(); }
IDXGISwapChain3*			GraphicsCore::GetSwapChain() { return s_SwapChain.Get(); }
D3D_FEATURE_LEVEL			GraphicsCore::GetDeviceFeatureLevel() { return s_D3dFeatureLevel; }
CColorBuffer&				GraphicsCore::GetCurrentRenderTarget() { return s_RenderTargets[s_CurrentBackBufferIndex]; }
CDepthBuffer&				GraphicsCore::GetDepthStencil() { return s_DepthStencil; }
DXGI_FORMAT					GraphicsCore::GetBackBufferFormat() { return s_BackBufferFormat; }
DXGI_FORMAT					GraphicsCore::GetDepthBufferFormat() { return s_DepthBufferFormat; }
D3D12_VIEWPORT&				GraphicsCore::GetScreenViewport() { return s_ScreenViewport; }
D3D12_RECT&					GraphicsCore::GetScissorRect() { return s_ScissorRect; }
UINT						GraphicsCore::GetCurrentBackBufferIndex() { return s_CurrentBackBufferIndex; }
UINT						GraphicsCore::GetPreviousBackBufferIndex() { return s_CurrentBackBufferIndex == 0 ? s_BackBufferCount - 1 : s_CurrentBackBufferIndex - 1; }
UINT						GraphicsCore::GetBackBufferCount() { return s_BackBufferCount; }
unsigned int				GraphicsCore::GetDeviceOptions() { return s_Options; }
LPCWSTR						GraphicsCore::GetAdapterDescription() { return s_AdapterDescription.c_str(); }
UINT						GraphicsCore::GetAdapterID() { return s_AdapterID; }
long long					GraphicsCore::GetFrameID() { return s_FrameID; }


// Manage CommandQueue and CommandList.
CCommandList* GraphicsCore::BeginCommandList(const std::wstring& ID)
{
	_ASSERTE(s_pCommandManager);
	return s_pCommandManager->BeginCommandList(ID);
}

CGraphicsCommandList* GraphicsCore::BeginGraphicsCommandList(const std::wstring& ID)
{
	_ASSERTE(s_pCommandManager);
	return s_pCommandManager->BeginGraphicsCommandList(ID);
}

CComputeCommandList* GraphicsCore::BeginComputeCommandList(const std::wstring& ID, bool Async)
{
	_ASSERTE(s_pCommandManager);
	return s_pCommandManager->BeginComputeCommandList(ID, Async);
}

uint64_t GraphicsCore::FinishCommandList(CCommandList*& pCommandList, bool WaitForCompletion)
{
	_ASSERTE(s_pCommandManager);
	return s_pCommandManager->FinishCommandList(pCommandList, WaitForCompletion);
}

uint64_t GraphicsCore::FinishCommandList(CGraphicsCommandList*& pCommandList, bool WaitForCompletion)
{
	_ASSERTE(s_pCommandManager);
	return s_pCommandManager->FinishCommandList(pCommandList, WaitForCompletion);
}

uint64_t GraphicsCore::FinishCommandList(CComputeCommandList*& pCommandList, bool WaitForCompletion)
{
	_ASSERTE(s_pCommandManager);
	return s_pCommandManager->FinishCommandList(pCommandList, WaitForCompletion);
}

void GraphicsCore::FinishCommandListAndPresent(CCommandList*& pCommandList, bool WaitForCompletion)
{	
	_ASSERTE(pCommandList);

	if (s_EnableGUI) GuiManager::RenderGui(pCommandList->QueryGraphicsCommandList());

	pCommandList->TransitionResource(GetCurrentRenderTarget(), D3D12_RESOURCE_STATE_PRESENT);
	FinishCommandList(pCommandList, WaitForCompletion);

	Present();
}

void GraphicsCore::FinishCommandListAndPresent(CGraphicsCommandList*& pCommandList, bool WaitForCompletion)
{
	_ASSERTE(pCommandList);

	if (s_EnableGUI) GuiManager::RenderGui(pCommandList);

	pCommandList->TransitionResource(GetCurrentRenderTarget(), D3D12_RESOURCE_STATE_PRESENT);
	FinishCommandList(pCommandList, WaitForCompletion);

	Present();
}

CCommandQueue& GraphicsCore::GetCommandQueue(ECommandQueueType Type)
{
	_ASSERTE(s_pCommandManager);
	return s_pCommandManager->GetCommandQueue(Type);
}

CCommandQueue& GraphicsCore::GetCommandQueue(D3D12_COMMAND_LIST_TYPE Type)
{
	_ASSERTE(s_pCommandManager);
	return s_pCommandManager->GetCommandQueue(Type);
}

void GraphicsCore::InitializeTexture(CGpuResource& Dest, UINT NumSubresources, D3D12_SUBRESOURCE_DATA SubData[])
{
	_ASSERTE(s_pCommandManager);
	s_pCommandManager->InitializeTexture(Dest, NumSubresources, SubData);
}

void GraphicsCore::InitializeTexture(CGpuResource& Dest, const STextureDataDesc& TexData)
{
	ID3D12Resource* pResource = Dest.GetResource();
	_ASSERTE(pResource);
	DXGI_FORMAT DestFormat = pResource->GetDesc().Format;
	_ASSERTE(BitsPerPixel(DestFormat) == BitsPerPixel(TexData.Format));

	size_t NumSubresources = TexData.MipCount*TexData.ArraySize;
	_ASSERTE(NumSubresources > 0);
	std::vector<D3D12_SUBRESOURCE_DATA> SubData(NumSubresources);

	size_t tWidth, tHeight, tDepth, tSkipMip;
	SUCCEEDED(FillInitData(TexData.Width, TexData.Height, TexData.Depth, TexData.MipCount, TexData.ArraySize, TexData.Format, 0, 
		TexData.BitSize, (const uint8_t*)TexData.BitData, tWidth, tHeight, tDepth, tSkipMip, &SubData[0]));
	InitializeTexture(Dest, (UINT)NumSubresources, &SubData[0]);
}

void GraphicsCore::InitializeBuffer(CGpuResource& Dest, const void* pBufferData, size_t NumBytes, size_t Offset)
{
	_ASSERTE(s_pCommandManager);
	s_pCommandManager->InitializeBuffer(Dest, pBufferData, NumBytes, Offset);
}

void GraphicsCore::InitializeTextureArraySlice(CGpuResource& Dest, UINT SliceIndex, CGpuResource& Src)
{
	_ASSERTE(s_pCommandManager);
	s_pCommandManager->InitializeTextureArraySlice(Dest, SliceIndex, Src);
}

void GraphicsCore::ReadbackTexture2D(CGpuResource& ReadbackBuffer, CPixelBuffer& SrcBuffer)
{
	_ASSERTE(s_pCommandManager);
	s_pCommandManager->ReadbackTexture2D(ReadbackBuffer, SrcBuffer);
}

void GraphicsCore::BufferCopy(CGpuResource& Dest, CGpuResource& Src, size_t NumBytes, size_t Offset)
{
	_ASSERTE(s_pCommandManager);
	s_pCommandManager->BufferCopy(Dest, Src, NumBytes, Offset);
}

bool GraphicsCore::IsFenceComplete(uint64_t FenceValue)
{
	_ASSERTE(s_pCommandManager);
	return s_pCommandManager->IsFenceComplete(FenceValue);
}

void GraphicsCore::WaitForFence(uint64_t FenceValue)
{
	_ASSERTE(s_pCommandManager);
	s_pCommandManager->WaitForFence(FenceValue);
}

void GraphicsCore::IdleGPU(void)
{
	_ASSERTE(s_pCommandManager);
	s_pCommandManager->IdleGPU();
}


// Descriptor Allocator.
CD3DX12_CPU_DESCRIPTOR_HANDLE GraphicsCore::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT Count)
{
	_ASSERTE(s_pDescriptorAllocatorExplorer);
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(s_pDescriptorAllocatorExplorer->AllocateDescriptor(Type, Count));
}

CStepTimer& GraphicsCore::GetTimer()
{
	_ASSERTE(s_pTimer);
	return *s_pTimer;
}

double GraphicsCore::GetFps()
{
	return s_FramePerSecond;
}


// GUI.
bool GraphicsCore::IsGuiEnabled()
{
	return s_EnableGUI;
}

void GraphicsCore::RegisterGuiCallbakFunction(GUI_CALLBACK_FUNCTION Function)
{
	if (s_EnableGUI)
	{
		GuiManager::RegisterGuiCallbackFunction(Function);
	}
}


// Constructor for DeviceResources.
void GraphicsCore::InitCore(const SGraphicsCoreInitDesc& InitDesc)
{
	if (s_IsCoreInitialized) return;

	s_WindowRect.left = InitDesc.WinConfig.PosX;
	s_WindowRect.top = InitDesc.WinConfig.PosY;
	s_WindowRect.right = s_WindowRect.left + InitDesc.WinConfig.Width;
	s_WindowRect.bottom = s_WindowRect.top + InitDesc.WinConfig.Height;
	s_BackBufferFormat = InitDesc.D3DConfig.BackBufferFormat;
	s_DepthBufferFormat = InitDesc.D3DConfig.DepthBufferFormat;
	s_BackBufferCount = InitDesc.D3DConfig.BackBufferCount;
	s_D3dMinFeatureLevel = InitDesc.D3DConfig.MinFeatureLevel;
	s_AdapterIDoverride = InitDesc.D3DConfig.AdapterIDoverride;
	s_pFuncEnableExperimentalFeature = InitDesc.D3DConfig.pFuncEnableExperimentalFeature;
	s_Options = InitDesc.Flags;
	s_ShowFps = InitDesc.ShowFps;
	s_EnableGUI = InitDesc.EnableGUI;
	s_pDXSampleClass = InitDesc.pDXSampleClass;
	s_TargetElapsedMicroseconds = InitDesc.TargetElapsedMicroseconds;

	_ASSERTE(s_pDXSampleClass);
	_ASSERTE(s_BackBufferCount <= MAX_BACK_BUFFER_COUNT);
	_ASSERTE(s_D3dMinFeatureLevel >= D3D_FEATURE_LEVEL_11_0);

	if (s_Options & c_RequireTearingSupport) { s_Options |= c_AllowTearing; }

	InitializeTimer();

	CreateWin32Window();

	InitializeD3D12();

	s_pDXSampleClass->OnInit();

	s_IsCoreInitialized = true;
}

int GraphicsCore::RunCore()
{
	int exitValue = MainLoop();
	DestroyCore();
	return exitValue;
}


DXGI_FORMAT NoSRGB(DXGI_FORMAT fmt)
{
	switch (fmt)
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:   return DXGI_FORMAT_R8G8B8A8_UNORM;
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:   return DXGI_FORMAT_B8G8R8A8_UNORM;
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:   return DXGI_FORMAT_B8G8R8X8_UNORM;
	default:                                return fmt;
	}
}

void InitializeD3D12()
{
	InitializeDXGIAdapter();
	if (s_pFuncEnableExperimentalFeature) s_pFuncEnableExperimentalFeature(s_Adapter.Get());
	CreateDeviceResources();
	CreateWindowSizeDependentResources();
	GpuTimeManager::Initialize();
}

void InitializeAdapter(IDXGIAdapter1** ppAdapter)
{
	*ppAdapter = nullptr;

	ComPtr<IDXGIAdapter1> adapter;
	for (UINT adapterID = 0; DXGI_ERROR_NOT_FOUND != s_DxgiFactory->EnumAdapters1(adapterID, &adapter); ++adapterID)
	{
		if (s_AdapterIDoverride != UINT_MAX && adapterID != s_AdapterIDoverride)
		{
			continue;
		}

		DXGI_ADAPTER_DESC1 desc;
		THROW_IF_FAILED(adapter->GetDesc1(&desc));

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// Don't select the Basic Render Driver adapter.
			continue;
		}

		// Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), s_D3dMinFeatureLevel, _uuidof(ID3D12Device), nullptr)))
		{
			s_AdapterID = adapterID;
			s_AdapterDescription = desc.Description;
#ifdef _DEBUG
			wchar_t buff[256] = {};
			swprintf_s(buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", adapterID, desc.VendorId, desc.DeviceId, desc.Description);
			OutputDebugStringW(buff);
#endif
			break;
		}
	}

#if !defined(NDEBUG)
	if (!adapter && s_AdapterIDoverride == UINT_MAX)
	{
		// Try WARP12 instead
		if (FAILED(s_DxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&adapter))))
		{
			throw std::exception("WARP12 not available. Enable the 'Graphics Tools' optional feature");
		}

		OutputDebugStringA("Direct3D Adapter - WARP12\n");
	}
#endif

	if (!adapter)
	{
		if (s_AdapterIDoverride != UINT_MAX)
		{
			throw exception("Unavailable adapter requested.");
		}
		else
		{
			throw exception("Unavailable adapter.");
		}
	}

	*ppAdapter = adapter.Detach();
}

void InitializeDXGIAdapter()
{
	bool debugDXGI = false;

#if defined(_DEBUG)
	// Enable the debug layer (requires the Graphics Tools "optional feature").
	// NOTE: Enabling the debug layer after device creation will invalidate the active device.
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
		}
		else
		{
			OutputDebugStringA("WARNING: Direct3D Debug Device is not available\n");
		}

		ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue))))
		{
			debugDXGI = true;

			THROW_IF_FAILED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&s_DxgiFactory)));

			dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
			dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
		}
	}
#endif

	if (!debugDXGI)
	{
		THROW_IF_FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&s_DxgiFactory)));
	}

	// Determines whether tearing support is available for fullscreen borderless windows.
	if (s_Options & (c_AllowTearing | c_RequireTearingSupport))
	{
		BOOL allowTearing = FALSE;

		ComPtr<IDXGIFactory5> factory5;
		HRESULT hr = s_DxgiFactory.As(&factory5);
		if (SUCCEEDED(hr))
		{
			hr = factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
		}

		if (FAILED(hr) || !allowTearing)
		{
			OutputDebugStringA("WARNING: Variable refresh rate displays are not supported.\n");
			if (s_Options & c_RequireTearingSupport)
			{
				THROW_MSG_IF_FAILED(false, L"Error: Sample must be run on an OS with tearing support.\n");
			}
			s_Options &= ~c_AllowTearing;
		}
	}

	InitializeAdapter(&s_Adapter);
}

void CreateDeviceResources()
{
	// Create the DX12 API device object.
	THROW_IF_FAILED(D3D12CreateDevice(s_Adapter.Get(), s_D3dMinFeatureLevel, IID_PPV_ARGS(&s_D3dDevice)));

#ifndef NDEBUG
	// Configure debug device (if active).
	ComPtr<ID3D12InfoQueue> d3dInfoQueue;
	if (SUCCEEDED(s_D3dDevice.As(&d3dInfoQueue)))
	{
#ifdef _DEBUG
		d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		d3dInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
#endif
		D3D12_MESSAGE_ID hide[] =
		{
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_COPY_DESCRIPTORS_INVALID_RANGES //An incorrect debug layer error message is outputted when run due to an issue in the debug layer on SM 6.0 drivers. This can be ignored. 
		};
		D3D12_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs = _countof(hide);
		filter.DenyList.pIDList = hide;
		d3dInfoQueue->AddStorageFilterEntries(&filter);
	}
#endif

	// Determine maximum supported feature level for this device
	static const D3D_FEATURE_LEVEL s_featureLevels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	D3D12_FEATURE_DATA_FEATURE_LEVELS featLevels =
	{
		_countof(s_featureLevels), s_featureLevels, D3D_FEATURE_LEVEL_11_0
	};

	HRESULT hr = s_D3dDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featLevels, sizeof(featLevels));
	if (SUCCEEDED(hr))
	{
		s_D3dFeatureLevel = featLevels.MaxSupportedFeatureLevel;
	}
	else
	{
		s_D3dFeatureLevel = s_D3dMinFeatureLevel;
	}

	// Initialize basic resources.
	CDescriptorAllocator::sInitialize(s_D3dDevice.Get());
	CDynamicDescriptorHeap::sInitialize(s_D3dDevice.Get());
	CLinearAllocator::sInitialize(s_D3dDevice.Get());
	CPipelineStateObject::sInitialize(s_D3dDevice.Get());
	CRootSignature::sInitialize(s_D3dDevice.Get());
	
	// Create the command manager.
	s_pCommandManager = new CCommandManager;
	_ASSERTE(s_pCommandManager);
	s_pCommandManager->Initialize(s_D3dDevice.Get());

	// Create the descriptor allocator explorer.
	s_pDescriptorAllocatorExplorer = new CDescriptorAllocatorExplorer;
	_ASSERTE(s_pDescriptorAllocatorExplorer);
	s_pDescriptorAllocatorExplorer->Initialize(s_D3dDevice.Get());

	// Initialize resources based on previously initialized resources.
	TextureLoader::SetTextureLibRoot(GraphicsCommon::KeywordWStr::TexLibRoot);
	GraphicsCommon::InitGraphicsCommon();

	// Initialize GUI.
	if (s_EnableGUI)
	{
		GuiManager::SGuiManagerInitDesc InitDesc;
		InitDesc.Hwnd = s_Hwnd;
		InitDesc.pDevice = s_D3dDevice.Get();
		InitDesc.BackBufferCount = s_BackBufferCount;
		InitDesc.BackBufferFormat = s_BackBufferFormat;
		GuiManager::InitGui(InitDesc);
	}
}

void CreateWindowSizeDependentResources()
{
	if (!s_Hwnd)
	{
		THROW_MSG_IF_FAILED(E_HANDLE, L"Call SetWindow with a valid Win32 window handle.\n");
	}

	// Wait until all previous GPU work is complete.
	_ASSERTE(s_pCommandManager);
	s_pCommandManager->IdleGPU();

	// Determine the render target size in pixels.
	UINT backBufferWidth = max((int)(s_WindowRect.right - s_WindowRect.left), 1);
	UINT backBufferHeight = max((int)(s_WindowRect.bottom - s_WindowRect.top), 1);
	DXGI_FORMAT backBufferFormat = NoSRGB(s_BackBufferFormat);

	// If the swap chain already exists, resize it, otherwise create one.
	if (s_SwapChain)
	{
		// If the swap chain already exists, resize it.
		HRESULT hr = s_SwapChain->ResizeBuffers(
			s_BackBufferCount,
			backBufferWidth,
			backBufferHeight,
			backBufferFormat,
			(s_Options & c_AllowTearing) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0
		);

		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
#ifdef _DEBUG
			char buff[64] = {};
			sprintf_s(buff, "Device Lost on ResizeBuffers: Reason code 0x%08X\n", (hr == DXGI_ERROR_DEVICE_REMOVED) ? s_D3dDevice->GetDeviceRemovedReason() : hr);
			OutputDebugStringA(buff);
#endif
			THROW_IF_FAILED(hr);

			// Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method 
			// and correctly set up the new device.
			return;
		}
	}
	else
	{
		// Create a descriptor for the swap chain.
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = backBufferWidth;
		swapChainDesc.Height = backBufferHeight;
		swapChainDesc.Format = backBufferFormat;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = s_BackBufferCount;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		swapChainDesc.Flags = (s_Options & c_AllowTearing) ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = { 0 };
		fsSwapChainDesc.Windowed = TRUE;

		// Create a swap chain for the window.
		ComPtr<IDXGISwapChain1> swapChain;

		_ASSERTE(s_pCommandManager);
		CCommandQueue& CommandQueue = s_pCommandManager->GetCommandQueue();
		THROW_IF_FAILED(s_DxgiFactory->CreateSwapChainForHwnd(CommandQueue.GetCommandQueue(), s_Hwnd, &swapChainDesc, &fsSwapChainDesc, nullptr, &swapChain));

		THROW_IF_FAILED(swapChain.As(&s_SwapChain));

		// With tearing support enabled we will handle ALT+Enter key presses in the
		// window message loop rather than let DXGI handle it by calling SetFullscreenState.
		if (IsTearingSupported())
		{
			s_DxgiFactory->MakeWindowAssociation(s_Hwnd, DXGI_MWA_NO_ALT_ENTER);
		}
	}

	// Obtain the back buffers for this window which will be the final render targets
	// and create render target views for each of them.
	for (UINT n = 0; n < s_BackBufferCount; n++)
	{
		ComPtr<ID3D12Resource> RenderTarget;
		THROW_IF_FAILED(s_SwapChain->GetBuffer(n, IID_PPV_ARGS(&RenderTarget)));
		s_RenderTargets[n].Create2DFromSwapChain(L"Primary SwapChain Buffer", RenderTarget.Detach());
		s_RenderTargets[n].SetClearColor(CColor(Colors::LightBlue));
	}

	// Reset the index to the current back buffer.
	s_CurrentBackBufferIndex = s_SwapChain->GetCurrentBackBufferIndex();

	if (s_DepthBufferFormat != DXGI_FORMAT_UNKNOWN)
	{
		s_DepthStencil.Create2D(L"Depth stencil", backBufferWidth, backBufferHeight, s_DepthBufferFormat);
		s_DepthStencil.SetClearDepth(1.0f);
		s_DepthStencil.SetClearStencil(0);
	}

	// Set the 3D rendering viewport and scissor rectangle to target the entire window.
	s_ScreenViewport.TopLeftX = s_ScreenViewport.TopLeftY = 0.f;
	s_ScreenViewport.Width = static_cast<float>(backBufferWidth);
	s_ScreenViewport.Height = static_cast<float>(backBufferHeight);
	s_ScreenViewport.MinDepth = D3D12_MIN_DEPTH;
	s_ScreenViewport.MaxDepth = D3D12_MAX_DEPTH;

	s_ScissorRect.left = s_ScissorRect.top = 0;
	s_ScissorRect.right = backBufferWidth;
	s_ScissorRect.bottom = backBufferHeight;
}

void MoveToNextFrame()
{
	_ASSERTE(s_pCommandManager);
	CCommandQueue& CommandQueue = s_pCommandManager->GetCommandQueue();

	// Set fence for current frame.
	s_RenderTargetFences[s_CurrentBackBufferIndex] = CommandQueue.IncrementFence();
	
	// Update the back buffer index.
	s_CurrentBackBufferIndex = s_SwapChain->GetCurrentBackBufferIndex();

	// Wait the next frame's fence.
	if (s_RenderTargetFences[s_CurrentBackBufferIndex] > 0) CommandQueue.WaitForFence(s_RenderTargetFences[s_CurrentBackBufferIndex]);

	// Only for test.
	//if (s_RenderTargetFences[s_CurrentBackBufferIndex] > 0 && !CommandQueue.IsFenceComplete(s_RenderTargetFences[s_CurrentBackBufferIndex]))
	//{
	//	CommandQueue.WaitForFence(s_RenderTargetFences[s_CurrentBackBufferIndex]);
	//}

	// Increase the frame id.
	s_FrameID++;
}

// Destructor for DeviceResources.
void ShutdownD3D12()
{
	if (!s_IsCoreInitialized) return;

	// Ensure that the GPU is no longer referencing resources that are about to be destroyed.
	_ASSERTE(s_pCommandManager);
	s_pCommandManager->IdleGPU();

	if (s_EnableGUI) GuiManager::DestroyGui();

	TextureLoader::ClearTextureCache();
	GraphicsCommon::DestroyGraphicsCommon();

	if (s_pCommandManager) { delete s_pCommandManager; s_pCommandManager = nullptr; }
	if (s_pDescriptorAllocatorExplorer) { delete s_pDescriptorAllocatorExplorer; s_pDescriptorAllocatorExplorer = nullptr; }

	CDescriptorAllocator::sShutdown();
	CDynamicDescriptorHeap::sShutdown();
	CLinearAllocator::sShutdown();
	CPipelineStateObject::sShutdown();
	CRootSignature::sShutdown();
	GpuTimeManager::Shutdown();

	if (s_pTimer) delete s_pTimer;

	s_IsCoreInitialized = false;

// Only For Debug Live Objects
//#ifdef _DEBUG
//	ID3D12DebugDevice1* DebugDevice;
//	s_D3dDevice->QueryInterface(__uuidof(ID3D12DebugDevice1), reinterpret_cast<void**>(&DebugDevice));
//	DebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL);
//	if (DebugDevice != nullptr)
//	{
//		DebugDevice->Release();
//	}
//#endif
}

// Destroy core.
void DestroyCore()
{
	s_pDXSampleClass->OnDestroy();
	delete s_pDXSampleClass;
	ModelManager::ReleaseResources();
	ShutdownD3D12();
}

// Initialize Timer.
void InitializeTimer()
{
	s_pTimer = new CStepTimer;
	_ASSERTE(s_pTimer);

	if (s_TargetElapsedMicroseconds > 0)
	{
		double TargetElapsedSeconds = (double)s_TargetElapsedMicroseconds / 1000000.0;
		s_pTimer->SetFixedTimeStep(true);
		s_pTimer->SetTargetElapsedSeconds(TargetElapsedSeconds);
	}
}

// Create window.
void CreateWin32Window()
{
	try
	{		
		_ASSERTE(s_pDXSampleClass);

		// Set window title.
		s_WindowTitle = s_pDXSampleClass->GetName() + L" :";

		// Parse the command line parameters
		int argc;
		LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
		s_pDXSampleClass->ParseCommandLineArgs(argv, argc);
		LocalFree(argv);

		// Get hInstance.
		HINSTANCE s_hInstance = GetModuleHandle(0);

		// Initialize the window class.
		WNDCLASSEX windowClass = { 0 };
		windowClass.cbSize = sizeof(WNDCLASSEX);
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc = WindowProc;
		windowClass.hInstance = s_hInstance;
		windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		windowClass.lpszClassName = L"GraphicCoreWindowClass";
		RegisterClassEx(&windowClass);

		// Create the window and store a handle to it.
		s_Hwnd = CreateWindow(
			windowClass.lpszClassName,
			s_WindowTitle.c_str(),
			s_WindowStyle,
			s_WindowRect.left,
			s_WindowRect.top,
			s_WindowRect.right - s_WindowRect.left,
			s_WindowRect.bottom - s_WindowRect.top,
			nullptr,        // We have no parent window.
			nullptr,        // We aren't using menus.
			s_hInstance,
			NULL);

		// Show window.
		ShowWindow(s_Hwnd, SW_SHOWDEFAULT);
	}
	catch (std::exception& e)
	{
		OutputDebugString(L"Application hit a problem: ");
		OutputDebugStringA(e.what());
		OutputDebugString(L"\nTerminating.\n");
		exit(-1);
	}
}

// Main loop.
int MainLoop()
{
	_ASSERTE(s_pTimer);
	_ASSERTE(s_pDXSampleClass);
	
	// Main sample loop.
	MSG msg = {};
	try
	{
		while (msg.message != WM_QUIT)
		{
			// Process any messages in the queue.
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				bool IsFrameNeedUpdate = false;
				s_pTimer->Tick(IsFrameNeedUpdate);
				if (IsFrameNeedUpdate)
				{
					s_pDXSampleClass->OnUpdate();
					s_pDXSampleClass->OnRender();
					s_pDXSampleClass->OnPostProcess();
					CalculateFrameStats();
				}
			}
		}

		// Return this part of the WM_QUIT message to Windows.
		return static_cast<char>(msg.wParam);
	}
	catch (std::exception& e)
	{
		OutputDebugString(L"Application hit a problem: ");
		OutputDebugStringA(e.what());
		OutputDebugString(L"\nTerminating.\n");

		s_pDXSampleClass->OnDestroy();

		return EXIT_FAILURE;
	}
}

// Window message processer
extern LRESULT CALLBACK WindowProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	// GUI process message.
	if(s_IsCoreInitialized && s_EnableGUI) GuiManager::WndProcHandler(hWnd, Message, wParam, lParam);
	
	// Core process message.
	switch (Message)
	{
	case WM_CREATE:
	{
		// Save the CDXSample* passed in to CreateWindow.
		LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
		return 0;
	}

	case WM_KEYDOWN:
	{
		if (s_pDXSampleClass)
		{
			s_pDXSampleClass->OnKeyDown(static_cast<UINT8>(wParam));
		}
		return 0;
	}

	case WM_KEYUP:
	{
		if (s_pDXSampleClass)
		{
			s_pDXSampleClass->OnKeyUp(static_cast<UINT8>(wParam));
		}
		return 0;
	}

	case WM_MOVE:
	{
		if (s_pDXSampleClass)
		{
			RECT windowRect = {};
			GetWindowRect(hWnd, &s_WindowRect);
			int xPos = (int)(short)LOWORD(lParam);
			int yPos = (int)(short)HIWORD(lParam);
			s_pDXSampleClass->OnWindowMoved(xPos, yPos);
		}
		return 0;
	}

	case WM_MOUSEMOVE:
	{
		if (s_pDXSampleClass && static_cast<UINT8>(wParam) == MK_RBUTTON)
		{
			UINT x = LOWORD(lParam);
			UINT y = HIWORD(lParam);
			s_pDXSampleClass->OnMouseMove(x, y);
		}
		return 0;
	}

	case WM_LBUTTONDOWN:
	{
		SetCapture(s_Hwnd);
		UINT x = LOWORD(lParam);
		UINT y = HIWORD(lParam);
		s_pDXSampleClass->OnLeftButtonDown(x, y);
		return 0;
	}

	case WM_LBUTTONUP:
	{
		UINT x = LOWORD(lParam);
		UINT y = HIWORD(lParam);
		s_pDXSampleClass->OnLeftButtonUp(x, y);
		ReleaseCapture();
		return 0;
	}

	case WM_RBUTTONDOWN:
	{
		SetCapture(s_Hwnd);
		UINT x = LOWORD(lParam);
		UINT y = HIWORD(lParam);
		s_pDXSampleClass->OnRightButtonDown(x, y);
		return 0;
	}

	case WM_RBUTTONUP:
	{
		UINT x = LOWORD(lParam);
		UINT y = HIWORD(lParam);
		s_pDXSampleClass->OnRightButtonUp(x, y);
		ReleaseCapture();
		return 0;
	}

	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}
	}

	// Handle any messages the switch statement didn't.
	return DefWindowProc(hWnd, Message, wParam, lParam);
}

// Present the contents of the swap chain to the screen.
void Present()
{
	HRESULT hr;
	if (s_Options & c_AllowTearing)
	{
		// Recommended to always use tearing if supported when using a sync interval of 0.
		// Note this will fail if in true 'fullscreen' mode.
		hr = s_SwapChain->Present(0, DXGI_PRESENT_ALLOW_TEARING);
	}
	else
	{
		// The first argument instructs DXGI to block until VSync, putting the application
		// to sleep until the next VSync. This ensures we don't waste any cycles rendering
		// frames that will never be displayed to the screen.
		hr = s_SwapChain->Present(1, 0);
	}

	// If the device was reset we must completely reinitialize the renderer.
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
#ifdef _DEBUG
		char buff[64] = {};
		sprintf_s(buff, "Device Lost on Present: Reason code 0x%08X\n", (hr == DXGI_ERROR_DEVICE_REMOVED) ? s_D3dDevice->GetDeviceRemovedReason() : hr);
		OutputDebugStringA(buff);
#endif
		THROW_IF_FAILED(hr);
	}
	else
	{
		THROW_IF_FAILED(hr);
		MoveToNextFrame();
	}
}

// Calculate frame stats.
void CalculateFrameStats()
{
	_ASSERTE(s_pTimer);
	
	static long long  frameCnt = 0;
	static double elapsedTime = 0.0f;
	double totalTime = s_pTimer->GetTotalSeconds();
	frameCnt++;

	// Compute averages over one second period.
	if ((totalTime - elapsedTime) >= 1.0f)
	{
		float diff = static_cast<float>(totalTime - elapsedTime);
		float fps = static_cast<float>(frameCnt) / diff; // Normalize to an exact second.
		s_FramePerSecond = fps;
		elapsedTime = totalTime;
		frameCnt = 0;
		
		if (s_ShowFps)
		{
			float mspf = 1000.0f / fps;
			wstringstream FpsText;
			FpsText << setprecision(2) << fixed
				<< L"   fps: " << fps << L"   mspf: " << mspf << L"   GPU[" << GetAdapterID()
				<< L"]: " << GetAdapterDescription();
			std::wstring WinText = s_WindowTitle + FpsText.str();
			SetWindowText(s_Hwnd, WinText.c_str());
		}
		else
		{
			wstringstream FpsText;
			FpsText << setprecision(2) << fixed
				<< L"   GPU[" << GetAdapterID()
				<< L"]: " << GetAdapterDescription();
			std::wstring WinText = s_WindowTitle + FpsText.str();
			SetWindowText(s_Hwnd, WinText.c_str());
		}
	}
}