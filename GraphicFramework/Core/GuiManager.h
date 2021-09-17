#pragma once
#include <functional>
#include "D3D12Base.h"
#include "ImGui\imgui.h"
#include "ImGui\imgui_impl_win32.h"
#include "ImGui\imgui_impl_dx12.h"

#define GUI_CALLBACK_FUNCTION std::function<void(void)>

class CGraphicsCommandList;

namespace GuiManager
{
	struct SGuiManagerInitDesc
	{
		HWND Hwnd = 0;
		ID3D12Device* pDevice = nullptr;
		UINT BackBufferCount = 0;
		DXGI_FORMAT BackBufferFormat = DXGI_FORMAT_UNKNOWN;
	};
	
	void InitGui(const SGuiManagerInitDesc& InitDesc);
	void DestroyGui();
	LRESULT WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void RegisterGuiCallbackFunction(GUI_CALLBACK_FUNCTION CallbackFunction);
	void RenderGui(CGraphicsCommandList* pCommandList);
};

