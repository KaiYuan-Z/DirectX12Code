#include "DXSample.h"


// Helper function for parsing any supplied command line args.
CDXSample::CDXSample(const std::wstring& Name) : m_SampleName(Name)
{
}

CDXSample::~CDXSample()
{
}

void CDXSample::ParseCommandLineArgs(WCHAR* Argv[], int Argc)
{
	for (int i = 1; i < Argc; ++i)
	{
		/*Add parsing code.*/
	}
}