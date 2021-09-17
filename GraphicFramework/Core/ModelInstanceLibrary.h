#pragma once
#include "ObjectLibrary.h"
#include "ModelInstance.h"
#include "Model.h"

class CScene;

class CModelInstanceRecordLibrary
{
public:
	CModelInstanceRecordLibrary(CModel* pHostModel);
	~CModelInstanceRecordLibrary();

	UINT AddInstanceRecord(const std::wstring& instanceName, UINT instanceIndex);
	bool DeleteInstanceRecord(const std::wstring& instanceName);
	bool SetInstanceRecord(const std::wstring& instanceName, UINT instanceIndex);

	UINT QueryInstanceRecordEntryInLib(const std::wstring& instanceName) const;
	bool IsInstanceRecordExist(const std::wstring& instanceName) const;
	UINT GetInstanceRecord(UINT recordEntry)  const; // Return index of instance.
	UINT GetInstanceRecordCount()  const { return m_InstanceRecordLib.GetObjectCount(); }

	CModel* GetHostModel() const { return m_pHostModel; }

private:
	CModel* m_pHostModel = nullptr;
	TObjectLibrary<UINT> m_InstanceRecordLib;
};

