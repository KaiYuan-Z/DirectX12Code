#include "JsonConfigImporter.h"
#include <sstream>
#include <fstream>
#include <algorithm>
#include "rapidjson/error/en.h"

CJsonConfigImporter::CJsonConfigImporter()
{
}


CJsonConfigImporter::~CJsonConfigImporter()
{
}

bool CJsonConfigImporter::LoadConfig(const std::string& configFileName)
{
	// Load the file
	std::ifstream fileStream(configFileName);
	if (!fileStream)
	{
		std::string errStr = "Can not open the config file [" + configFileName + "].\n";
		OutputDebugStringA(errStr.c_str());
		return false;
	}
	std::stringstream strStream;
	strStream << fileStream.rdbuf();
	std::string jsonData = strStream.str();
	rapidjson::StringStream jStream(jsonData.c_str());

	// create the DOM
	m_JDoc.ParseStream(jStream);

	// Check error
	if (m_JDoc.HasParseError())
	{
		size_t line;
		line = std::count(jsonData.begin(), jsonData.begin() + m_JDoc.GetErrorOffset(), '\n');
		std::string errStr = std::string("JSON Parse error in line ") + std::to_string(line) + ". " + rapidjson::GetParseError_En(m_JDoc.GetParseError()) + ".\n";
		OutputDebugStringA(errStr.c_str());
		return false;
	}

	__ExtraPrepare();

	if (!__ValidateConfig(m_ParseFuncSet)) return false;
	if (!__ParseConfig(m_ParseFuncSet)) return false;

	return true;
}

void CJsonConfigImporter::_RegisterTokenParseFunc(const std::string& token, TOKEN_PARSE_FUNC parseFunc)
{
	STokenParser tokenParser = { token, parseFunc };
	m_ParseFuncSet.push_back(tokenParser);
}

bool CJsonConfigImporter::_GetFloatVecAnySize(const rapidjson::Value& jsonVal, const std::string& desc, std::vector<float>& vec)
{
	if (!jsonVal.IsArray())
	{
		std::string errStr = "Trying to load a vector for " + desc + ", but JValue is not an array.\n";
		OutputDebugStringA(errStr.c_str());
		return false;
	}

	vec.resize(jsonVal.Size());
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

bool CJsonConfigImporter::__ValidateConfig(const std::vector<STokenParser>& parseFuncSet)
{
	for (auto it = m_JDoc.MemberBegin(); it != m_JDoc.MemberEnd(); it++)
	{
		bool found = false;
		const std::string name(it->name.GetString());

		for (size_t i = 0; i < parseFuncSet.size(); i++)
		{
			// Check that we support this value
			if (parseFuncSet[i].Token == name)
			{
				found = true;
				break;
			}
		}

		if (found == false)
		{
			std::string errStr = "Invalid key found in top-level object. Key == " + std::string(it->name.GetString()) + ".\n";
			OutputDebugStringA(errStr.c_str());
			return false;
		}
	}

	return true;
}

bool CJsonConfigImporter::__ParseConfig(const std::vector<STokenParser>& parseFuncSet)
{
	for (size_t i = 0; i < parseFuncSet.size(); i++)
	{
		const auto& jsonMember = m_JDoc.FindMember(parseFuncSet[i].Token.c_str());
		if (jsonMember != m_JDoc.MemberEnd())
		{
			auto ParseFunc = parseFuncSet[i].ParseFunc;
			if (!ParseFunc(jsonMember->value)) return false;
		}
	}

	return true;
}





