#pragma once
#include <string>
#include <vector>
#include <functional>
#include "rapidjson/document.h"
#include "Utility.h"

class CJsonConfigExporter
{
public:
	CJsonConfigExporter();
	~CJsonConfigExporter();

	bool SaveConfig(const std::string& configFileName, uint32_t exportOptions);

protected:
	/* Should be used in __PrepareConfig */
	template<typename T>
	void _AddScalar(rapidjson::Value& jval, rapidjson::Document::AllocatorType& jallocator, const std::string& key, const T& value);

	/* Should be used in __PrepareConfig */
	template<typename T>
	void _AddVector(rapidjson::Value& jval, rapidjson::Document::AllocatorType& jallocator, const std::string& key, const T values[], uint32_t length);

	/* Should be used in __PrepareConfig */
	void _AddString(rapidjson::Value& jval, rapidjson::Document::AllocatorType& jallocator, const std::string& key, const std::string& value);

	/* Should be used in __PrepareConfig */
	void _AddBool(rapidjson::Value& jval, rapidjson::Document::AllocatorType& jallocator, const std::string& key, bool isValue);

	/* Should be used in __PrepareConfig */
	void _AddJsonValue(rapidjson::Value& jval, rapidjson::Document::AllocatorType& jallocator, const std::string& key, rapidjson::Value& value);

	/* Used for setting Configs */
	virtual void __PrepareConfig(rapidjson::Value& jdoc, rapidjson::Document::AllocatorType& jallocator, uint32_t exportOptions) {}

private:
	rapidjson::Document m_JDoc;
};

template<typename T>
inline void CJsonConfigExporter::_AddScalar(rapidjson::Value& jval, rapidjson::Document::AllocatorType& jallocator, const std::string& key, const T& value)
{
	rapidjson::Value jkey;
	jkey.SetString(key.c_str(), (uint32_t)key.size(), jallocator);
	jval.AddMember(jkey, value, jallocator);
}

template<typename T>
inline void CJsonConfigExporter::_AddVector(rapidjson::Value& jval, rapidjson::Document::AllocatorType& jallocator, const std::string& key, const T values[], uint32_t length)
{
	rapidjson::Value jkey;
	jkey.SetString(key.c_str(), (uint32_t)key.size(), jallocator);
	rapidjson::Value jvec(rapidjson::kArrayType);
	for (uint32_t i = 0; i < length; i++)
	{
		jvec.PushBack(values[i], jallocator);
	}

	jval.AddMember(jkey, jvec, jallocator);
}
