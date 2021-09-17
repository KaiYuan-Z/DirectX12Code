#pragma once
#include "Model.h"
#include "ModelSDF.h"
#include "Win32Base.h"

namespace ModelManager
{
	UINT AddModel(const std::wstring& modelName, const std::wstring& fileAddr, UINT modelType = MODEL_TYPE_BASIC);
	UINT QueryModelIndex(const std::wstring& modelName);
	bool IsModelExist(const std::wstring& modelName);
	CModel* GetBasicModel(UINT modelEntry);
	CModelSDF* GetSdfModel(UINT modelEntry);
	UINT GetModelCount();
	void ReleaseResources();
}; 