#pragma once
#include <string>
#include <vector>
#include <functional>
#include "rapidjson/document.h"
#include "Utility.h"

/* Define TokenParseFunc type */
#define TOKEN_PARSE_FUNC std::function<bool(const rapidjson::Value& jsonVal)>

/* ConfigImporters should register TokenParseFunc in __ExtraPrepare(...) */
#define RegisterTokenParseFunc(token, parseFunc) _RegisterTokenParseFunc(token, std::bind(&parseFunc, this, std::placeholders::_1))

class CJsonConfigImporter
{
	struct STokenParser
	{
		const std::string Token;
		TOKEN_PARSE_FUNC ParseFunc;
	};

public:
	CJsonConfigImporter();
	~CJsonConfigImporter();

	bool LoadConfig(const std::string& configFileName);

protected:
	/* ConfigImporters should register TokenParseFunc in __ExtraPrepare(...) */
	void _RegisterTokenParseFunc(const std::string& token, TOKEN_PARSE_FUNC parseFunc);

	/* Should be used in TokenParseFunc */
	template<uint32_t VecSize>
	bool _GetFloatVec(const rapidjson::Value& jsonVal, const std::string& desc, float vec[VecSize]);

	/* Should be used in TokenParseFunc */
	bool _GetFloatVecAnySize(const rapidjson::Value& jsonVal, const std::string& desc, std::vector<float>& vec);

private:
	/* Will be called before __ParseConfig(...)*/
	virtual void __ExtraPrepare() {}

	bool __ValidateConfig(const std::vector<STokenParser>& parseFuncSet);

	bool __ParseConfig(const std::vector<STokenParser>& parseFuncSet);

private:
	rapidjson::Document m_JDoc;
	std::vector<STokenParser> m_ParseFuncSet;
};

template<uint32_t VecSize>
bool CJsonConfigImporter::_GetFloatVec(const rapidjson::Value& jsonVal, const std::string& desc, float vec[VecSize])
{
	if (!jsonVal.IsArray())
	{
		std::string errStr = "Trying to load a vector for " + desc + ", but JValue is not an array.\n";
		OutputDebugStringA(errStr.c_str());
		return false;
	}

	if (jsonVal.Size() != VecSize)
	{
		std::string errStr = "Trying to load a vector for " + desc + ", but vector size mismatches. Required size is " + std::to_string(VecSize) + ", array size is " + std::to_string(jsonVal.Size()) + ".\n";
		OutputDebugStringA(errStr.c_str());
		return false;
	}

	for (uint32_t i = 0; i < jsonVal.Size(); i++)
	{
		if (!jsonVal[i].IsNumber())
		{
			std::string errStr = "Trying to load a vector for " + desc + ", but one the elements is not a number.\n";
			OutputDebugStringA(errStr.c_str());
			return false;
		}

		vec[i] = (float)(jsonVal[i].GetDouble());
	}

	return true;
}

template <class VariableType>
class TVariableWrap
{
public:
	TVariableWrap() {}
	~TVariableWrap() {}

	void Set(const VariableType& v) { m_Value = v; m_IsSetted = true; }
	const VariableType& Get() const { _ASSERTE(m_IsSetted); return m_Value; }
	bool IsSetted() const { return m_IsSetted; }

private:
	VariableType m_Value;
	bool m_IsSetted = false;
};
