#pragma once
#include "StepTimer.h"
#include "GraphicsCore.h"

class CDXSample
{
public:

	CDXSample(const std::wstring& Name);
	virtual ~CDXSample();

	// Samples override the event handlers to handle specific messages.
	virtual void OnInit() {}
	virtual void OnUpdate() {}
	virtual void OnRender() {}
	virtual void OnPostProcess() {}
	virtual void OnDestroy() {}
	virtual void OnKeyDown(UINT8 /*Key*/) {}
	virtual void OnKeyUp(UINT8 /*Key*/) {}
	virtual void OnWindowMoved(int /*X*/, int /*Y*/) {}
	virtual void OnMouseMove(UINT /*X*/, UINT /*Y*/) {}
	virtual void OnLeftButtonDown(UINT /*X*/, UINT /*Y*/) {}
	virtual void OnLeftButtonUp(UINT /*X*/, UINT /*Y*/) {}
	virtual void OnRightButtonDown(UINT /*X*/, UINT /*Y*/) {}
	virtual void OnRightButtonUp(UINT /*X*/, UINT /*Y*/) {}

	// Overridable members.
	virtual void ParseCommandLineArgs(_In_reads_(argc) WCHAR* Argv[], int Argc);

	// Accessors. 
	const std::wstring& GetName() { return m_SampleName; }

protected:

	std::wstring m_SampleName = L"";
};
