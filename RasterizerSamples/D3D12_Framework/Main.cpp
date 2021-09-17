#include "BlankWindow.h"



int main(int argc, wchar_t** argv)
{
	GraphicsCore::SGraphicsCoreInitDesc InitDesc;
	InitDesc.WinConfig = GraphicsCore::SWindowConfig(0, 0, 1600, 900);
	InitDesc.pDXSampleClass = new BlankWindow(L"D3D12 Framework Demo");
	GraphicsCore::InitCore(InitDesc);
	return GraphicsCore::RunCore();
}
