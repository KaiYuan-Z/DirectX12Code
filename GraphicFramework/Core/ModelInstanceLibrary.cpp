#include "ModelInstanceLibrary.h"
#include "ModelManager.h"

CModelInstanceRecordLibrary::CModelInstanceRecordLibrary(CModel* pHostModel)
{
	_ASSERTE(pHostModel);
	m_pHostModel = pHostModel;
}

CModelInstanceRecordLibrary::~CModelInstanceRecordLibrary()
{
}

UINT CModelInstanceRecordLibrary::AddInstanceRecord(const std::wstring& instanceName, UINT instanceIndex)
{
	if (!m_InstanceRecordLib.IsObjectExist(instanceName))
	{
		return m_InstanceRecordLib.AddObject(instanceName, instanceIndex);
	}
	else
	{
		return INVALID_OBJECT_INDEX;
	}		
}

bool CModelInstanceRecordLibrary::DeleteInstanceRecord(const std::wstring& instanceName)
{
	return m_InstanceRecordLib.DeleteObject(instanceName);
}

bool CModelInstanceRecordLibrary::SetInstanceRecord(const std::wstring& instanceName, UINT instanceIndex)
{
	UINT Entry = m_InstanceRecordLib.QueryObjectEntry(instanceName);

	if (Entry == INVALID_OBJECT_INDEX) return false;
	
	m_InstanceRecordLib[Entry] = instanceIndex;

	return true;
}

UINT CModelInstanceRecordLibrary::QueryInstanceRecordEntryInLib(const std::wstring& instanceName) const
{
	return m_InstanceRecordLib.QueryObjectEntry(instanceName);
}

bool CModelInstanceRecordLibrary::IsInstanceRecordExist(const std::wstring& instanceName) const
{
	return m_InstanceRecordLib.IsObjectExist(instanceName);
}

UINT CModelInstanceRecordLibrary::GetInstanceRecord(UINT InstanceIndexInLib) const
{
	return m_InstanceRecordLib[InstanceIndexInLib];
}