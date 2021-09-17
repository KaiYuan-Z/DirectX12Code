#include "PbrModelViewer.h"

int main(int argc, wchar_t** argv)
{	
	GraphicsCore::SGraphicsCoreInitDesc InitDesc;
	InitDesc.WinConfig = GraphicsCore::SWindowConfig(0, 0, 1600, 900);
	InitDesc.pDXSampleClass = new ModelViewer(L"D3D12 PbrModelViewer");
	GraphicsCore::InitCore(InitDesc);
	return GraphicsCore::RunCore();
}
