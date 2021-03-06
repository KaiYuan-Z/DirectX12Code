#include "RayTracingAO.h"

int main(int argc, wchar_t** argv)
{
	GraphicsCore::SGraphicsCoreInitDesc InitDesc;
	InitDesc.EnableGUI = true;
	InitDesc.ShowFps = true;
	InitDesc.WinConfig = GraphicsCore::SWindowConfig(0, 0, 1600, 900);
	InitDesc.pDXSampleClass = new CRayTracingAO(L"D3D12 Temporal Raytracing AO");
	GraphicsCore::InitCore(InitDesc);
	return GraphicsCore::RunCore();
}