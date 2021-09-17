#include "SSAOTest.h"

int main(int argc, wchar_t** argv)
{
	GraphicsCore::SGraphicsCoreInitDesc InitDesc;
	InitDesc.WinConfig = GraphicsCore::SWindowConfig(0, 0, 1600, 900);
	InitDesc.pDXSampleClass = new CSSAOTest(L"D3D12 HBAO Plus");
	InitDesc.EnableGUI = false;
	InitDesc.ShowFps = true;
	GraphicsCore::InitCore(InitDesc);
	return GraphicsCore::RunCore();
}
