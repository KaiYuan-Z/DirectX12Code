#include "ModelManager.h"
#include "ObjectLibrary.h"
#include "Utility.h"

using namespace ModelManager;

TObjectLibrary<CModel*> s_ModelLib;
std::map<std::wstring, UINT> s_Name2IndexMap;

UINT ModelManager::AddModel(const std::wstring& modelName, const std::wstring& fileAddr, UINT modelType)
{
	if (s_ModelLib.IsObjectExist(modelName))
	{
		THROW_MSG_IF_FALSE(false, L"The model name already exists.\n");
		return INVALID_OBJECT_INDEX;
	}

	CModel* pModel = CModel::CreateModel(modelName, modelType);
	_ASSERTE(pModel);
	THROW_MSG_IF_FALSE(pModel->Load(MakeStr(fileAddr).c_str()), L"Fail to load model file.\n");

	return s_ModelLib.AddObject(modelName, pModel);
}

UINT ModelManager::QueryModelIndex(const std::wstring& modelName)
{
	return s_ModelLib.QueryObjectEntry(modelName);
}

bool ModelManager::IsModelExist(const std::wstring& modelName)
{
	return s_ModelLib.IsObjectExist(modelName);
}

CModel* ModelManager::GetBasicModel(UINT modelEntry)
{
	return s_ModelLib[modelEntry];
}

CModelSDF* ModelManager::GetSdfModel(UINT modelEntry)
{
	CModel* pModel = s_ModelLib[modelEntry];
	_ASSERTE(pModel && pModel->GetModelType() == MODEL_TYPE_SDF);
	return static_cast<CModelSDF*>(pModel);
}

UINT ModelManager::GetModelCount()
{
	return s_ModelLib.GetObjectCount();
}

void ModelManager::ReleaseResources()
{
	UINT modelCount = s_ModelLib.GetObjectCount();
	for (UINT i = 0; i < modelCount; i++)
	{
		if (s_ModelLib[i]) delete s_ModelLib[i];
	}
}
