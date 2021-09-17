#include "JsonConfigExporter.h"
#include <sstream>
#include <fstream>
#include <algorithm>
#include "rapidjson/error/en.h"
#include "rapidjson/prettywriter.h"

CJsonConfigExporter::CJsonConfigExporter()
{
}


CJsonConfigExporter::~CJsonConfigExporter()
{
}

bool CJsonConfigExporter::SaveConfig(const std::string& configFileName, uint32_t exportOptions)
{
	m_JDoc.SetObject();
	
	__PrepareConfig(m_JDoc, m_JDoc.GetAllocator(), exportOptions);

	// Get the output string
	rapidjson::StringBuffer buffer;
	rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
	writer.SetIndent(' ', 4);
	m_JDoc.Accept(writer);
	std::string str(buffer.GetString(), buffer.GetSize());

	// Output the file
	std::ofstream outputStream(configFileName.c_str());
	if (outputStream.fail())
	{
		std::string errStr("Can't open output scene file " + configFileName + ".\nExporting failed.");
		OutputDebugStringA(errStr.c_str());
		return false;
	}
	outputStream << str;
	outputStream.close();

	return true;
}

void CJsonConfigExporter::_AddJsonValue(rapidjson::Value& jval, rapidjson::Document::AllocatorType& jallocator, const std::string& key, rapidjson::Value& value)
{
	rapidjson::Value jkey;
	jkey.SetString(key.c_str(), (uint32_t)key.size(), jallocator);
	jval.AddMember(jkey, value, jallocator);
}

void CJsonConfigExporter::_AddString(rapidjson::Value& jval, rapidjson::Document::AllocatorType& jallocator, const std::string& key, const std::string& value)
{
	rapidjson::Value jstring, jkey;
	jstring.SetString(value.c_str(), (uint32_t)value.size(), jallocator);
	jkey.SetString(key.c_str(), (uint32_t)key.size(), jallocator);

	jval.AddMember(jkey, jstring, jallocator);
}

void CJsonConfigExporter::_AddBool(rapidjson::Value& jval, rapidjson::Document::AllocatorType& jallocator, const std::string& key, bool isValue)
{
	rapidjson::Value jbool, jkey;
	jbool.SetBool(isValue);
	jkey.SetString(key.c_str(), (uint32_t)key.size(), jallocator);

	jval.AddMember(jkey, jbool, jallocator);
}
