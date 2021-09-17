#include "ModelViewer.h"

//FUNCTION: detect the memory leak in DEBUG mode
void InstallMemoryLeakDetector()
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	//_CRTDBG_LEAK_CHECK_DF: Perform automatic leak checking at program exit through a call to _CrtDumpMemoryLeaks and generate an error 
	//report if the application failed to free all the memory it allocated. OFF: Do not automatically perform leak checking at program exit.
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	//the following statement is used to trigger a breakpoint when memory leak happens
	//comment it out if there is no memory leak report;
	//_crtBreakAlloc = 1823;
#endif
}

int main(int argc, wchar_t** argv)
{
	InstallMemoryLeakDetector();
	
	GraphicsCore::SGraphicsCoreInitDesc InitDesc;
	InitDesc.WinConfig = GraphicsCore::SWindowConfig(0, 0, 1920, 1080);
	InitDesc.pDXSampleClass = new ModelViewer(L"D3D12 ModelViewer");
	InitDesc.ShowFps = true;
	InitDesc.EnableGUI = true;
	GraphicsCore::InitCore(InitDesc);
	return GraphicsCore::RunCore();
}
