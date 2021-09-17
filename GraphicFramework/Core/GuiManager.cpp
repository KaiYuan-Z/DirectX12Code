#include "GuiManager.h"
#include "GraphicsCore.h"

namespace GuiManager
{
	bool s_IsInitialized = false;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> s_GuiDescriptorHeap; //Only need one descriptor for SRV.
	std::vector<GUI_CALLBACK_FUNCTION> s_GuiCallbackFunctionSet;

	void InitGui(const SGuiManagerInitDesc& InitDesc)
	{
		if (!s_IsInitialized)
		{
			_ASSERTE(InitDesc.Hwnd != 0);
			_ASSERTE(InitDesc.pDevice);
			_ASSERTE(InitDesc.BackBufferCount > 0);
			_ASSERTE(InitDesc.BackBufferFormat != DXGI_FORMAT_UNKNOWN);

			D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
			HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			HeapDesc.NumDescriptors = 1;
			HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			ASSERT_SUCCEEDED(InitDesc.pDevice->CreateDescriptorHeap(&HeapDesc, MY_IID_PPV_ARGS(s_GuiDescriptorHeap.ReleaseAndGetAddressOf())));

			IMGUI_CHECKVERSION();
			ImGui::CreateContext();

			ImGui_ImplWin32_Init(InitDesc.Hwnd);
			ImGui_ImplDX12_Init(InitDesc.pDevice, InitDesc.BackBufferCount, DXGI_FORMAT_R8G8B8A8_UNORM, s_GuiDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), s_GuiDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

			ImGui::StyleColorsDark();

			s_IsInitialized = true;
		}
	}

	void DestroyGui()
	{
		if (s_IsInitialized)
		{
			ImGui_ImplDX12_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();
		}
	}

	LRESULT WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		_ASSERTE(s_IsInitialized);
		return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
	}

	void RegisterGuiCallbackFunction(GUI_CALLBACK_FUNCTION CallbackFunction)
	{
		_ASSERTE(s_IsInitialized);
		s_GuiCallbackFunctionSet.push_back(CallbackFunction);
	}

	void RenderGui(CGraphicsCommandList* pCommandList)
	{
		_ASSERTE(s_IsInitialized && pCommandList);

		if (s_GuiCallbackFunctionSet.size() > 0)
		{
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			for (size_t i = 0; i < s_GuiCallbackFunctionSet.size(); i++) s_GuiCallbackFunctionSet[i]();

			pCommandList->SetViewportAndScissor(GraphicsCore::GetScreenViewport(), GraphicsCore::GetScissorRect());
			pCommandList->TransitionResource(GraphicsCore::GetCurrentRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, false);
			pCommandList->TransitionResource(GraphicsCore::GetDepthStencil(), D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
			pCommandList->SetRenderTarget(GraphicsCore::GetCurrentRenderTarget().GetRTV(), GraphicsCore::GetDepthStencil().GetDSV());
			pCommandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, s_GuiDescriptorHeap.Get());

			ImGui::Render();
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pCommandList->GetD3D12CommandList());
		}
	}
}
