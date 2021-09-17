#include "GpuResource.h"

void CGpuResource::ReSetResourceName(const std::wstring& Name)
{
	m_ResourceName = Name;

#ifndef RELEASE
	m_pResource->SetName(Name.c_str());
#else
	(Name);
#endif
}
