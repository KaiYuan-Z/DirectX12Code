#include "RayTracingAO.h"

int main(int argc, wchar_t** argv)
{
	GraphicsCore::SGraphicsCoreInitDesc InitDesc;
	InitDesc.EnableGUI = false;
	InitDesc.WinConfig = GraphicsCore::SWindowConfig(0, 0, 1600, 900);
	InitDesc.pDXSampleClass = new CRayTracingAO(L"D3D12 Crossroad Demo");
	InitDesc.ShowFps = true;
	//InitDesc.TargetElapsedMicroseconds = 0;
	GraphicsCore::InitCore(InitDesc);
	return GraphicsCore::RunCore();
}