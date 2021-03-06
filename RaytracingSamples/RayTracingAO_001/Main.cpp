#include "RayTracingAO.h"

int main(int argc, wchar_t** argv)
{
	GraphicsCore::SGraphicsCoreInitDesc InitDesc;
	InitDesc.WinConfig = GraphicsCore::SWindowConfig(0, 0, 1600, 900);
	InitDesc.pDXSampleClass = new CRayTracingAO(L"D3D12 RayTracingAO");
	InitDesc.EnableGUI = true;
	InitDesc.ShowFps = true;
	GraphicsCore::InitCore(InitDesc);
	return GraphicsCore::RunCore();
}