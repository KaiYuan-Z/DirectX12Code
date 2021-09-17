#pragma once
#include "GraphicsCoreBase.h"
#include "CommandManager.h"
#include "ColorBuffer.h"
#include "DepthBuffer.h"
#include "GuiManager.h"
#include "StepTimer.h"

#define MAX_BACK_BUFFER_COUNT 3

class CDXSample;

// Controls all the DirectX device resources.
namespace GraphicsCore
{
	typedef void (*EnableExperimentalFeatureFunctionPointor)(IDXGIAdapter1* adapter);

	struct SWindowConfig
	{
		UINT PosX = 0;
		UINT PosY = 0;
		UINT Width = 0;
		UINT Height = 0;

		SWindowConfig()
		{
		}

		SWindowConfig(UINT posX, UINT posY, UINT width, UINT height)
			:PosX(posX), PosY(posY), Width(width), Height(height)
		{
		}
	};

	struct SD3DConfig
	{
		UINT BackBufferCount = 3;
		DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		DXGI_FORMAT DepthBufferFormat = DXGI_FORMAT_D32_FLOAT;
		D3D_FEATURE_LEVEL MinFeatureLevel = D3D_FEATURE_LEVEL_11_0;
		UINT AdapterIDoverride = UINT_MAX;
		EnableExperimentalFeatureFunctionPointor pFuncEnableExperimentalFeature = nullptr;
	};

	struct SGraphicsCoreInitDesc
	{
		SWindowConfig WinConfig;
		SD3DConfig D3DConfig;
		UINT Flags = 0;
		bool ShowFps = true;
		bool EnableGUI = false;
		UINT TargetElapsedMicroseconds = 0;
		CDXSample* pDXSampleClass = nullptr;
	};

	struct STextureDataDesc
	{
		size_t Width = 0;
		size_t Height = 0;
		size_t Depth = 1;
		size_t MipCount = 1;
		size_t ArraySize = 1;
		DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
		size_t BitSize = 0;
		const void* BitData = nullptr;
	};

	static const unsigned int		c_AllowTearing = 0x1;
    static const unsigned int		c_RequireTearingSupport = 0x2;

	void InitCore(const SGraphicsCoreInitDesc& InitDesc);
    int RunCore();

    // Device Accessors.
	RECT GetWindowRect();
	UINT GetWindowWidth();
	UINT GetWindowHeight();
	bool IsWindowVisible();
	bool IsTearingSupported();

    // Direct3D Accessors.
	IDXGIAdapter1*				GetAdapter();
	ID3D12Device*				GetD3DDevice();
	IDXGIFactory4*				GetDXGIFactory();
	IDXGISwapChain3*			GetSwapChain();
	D3D_FEATURE_LEVEL			GetDeviceFeatureLevel();
	CColorBuffer&				GetCurrentRenderTarget();
	CDepthBuffer&				GetDepthStencil();
	DXGI_FORMAT					GetBackBufferFormat();
	DXGI_FORMAT					GetDepthBufferFormat();
	D3D12_VIEWPORT&				GetScreenViewport();
	D3D12_RECT&					GetScissorRect();
	UINT						GetCurrentBackBufferIndex();
	UINT						GetPreviousBackBufferIndex();
	UINT						GetBackBufferCount();
	unsigned int				GetDeviceOptions();
	LPCWSTR						GetAdapterDescription();
	UINT						GetAdapterID();
	long long					GetFrameID();

	// Descriptor Allocator.
	CD3DX12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT Count = 1);

	// Timer and FPS.
	CStepTimer& GetTimer();
	double GetFps();

	// GUI.
	bool IsGuiEnabled();
	void RegisterGuiCallbakFunction(GUI_CALLBACK_FUNCTION Function);

	// Manage CommandQueue and CommandList.
	// Note: The D3D12 function [ID3D12GraphicsCommandList::Reset] called in [BeginCommandList] and
	// the D3D12 function [ID3D12CommandQueue::ExecuteCommandLists] called in [FinishCommandList] usually
	// takes a long time, especially [ID3D12CommandQueue::ExecuteCommandLists]. So, we should be careful when use them.     
	CCommandList*				BeginCommandList(const std::wstring& ID = L"");
	CGraphicsCommandList*		BeginGraphicsCommandList(const std::wstring& ID = L"");
	CComputeCommandList*		BeginComputeCommandList(const std::wstring& ID = L"", bool Async = false);
	uint64_t					FinishCommandList(CCommandList*& pCommandList, bool WaitForCompletion = false);
	uint64_t					FinishCommandList(CGraphicsCommandList*& pCommandList, bool WaitForCompletion = false);
	uint64_t					FinishCommandList(CComputeCommandList*& pCommandList, bool WaitForCompletion = false);
	void						FinishCommandListAndPresent(CCommandList*& pCommandList, bool WaitForCompletion = false);
	void						FinishCommandListAndPresent(CGraphicsCommandList*& pCommandList, bool WaitForCompletion = false);
	CCommandQueue&				GetCommandQueue(ECommandQueueType Type = kDirectCommandQueue);
	CCommandQueue&				GetCommandQueue(D3D12_COMMAND_LIST_TYPE Type);

	// Test to see if a fence has already been reached
	bool IsFenceComplete(uint64_t FenceValue);
	// The CPU will wait for a fence to reach a specified value
	void WaitForFence(uint64_t FenceValue);
	// The CPU will wait for all command queues to empty (so that the GPU is idle)
	void IdleGPU(void);

	// Help functions for resource initialization.
	void InitializeTexture(CGpuResource& Dest, UINT NumSubresources, D3D12_SUBRESOURCE_DATA SubData[]);
	void InitializeTexture(CGpuResource& Dest, const STextureDataDesc& TexData);
	void InitializeBuffer(CGpuResource& Dest, const void* pBufferData, size_t NumBytes, size_t Offset = 0);
	void InitializeTextureArraySlice(CGpuResource& Dest, UINT SliceIndex, CGpuResource& Src);
	void ReadbackTexture2D(CGpuResource& ReadbackBuffer, CPixelBuffer& SrcBuffer);
	void BufferCopy(CGpuResource& Dest, CGpuResource& Src, size_t NumBytes, size_t Offset = 0);
};
