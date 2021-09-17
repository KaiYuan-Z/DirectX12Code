#pragma once
#include "JsonConfigImporter.h"
#include "Scene.h"

class CSceneImporter : public CJsonConfigImporter
{
public:
	CSceneImporter(CScene& scene);
	~CSceneImporter();

private:
	virtual void __ExtraPrepare();

	bool __ParseModels(const rapidjson::Value& jsonVal);
	bool __ParseLights(const rapidjson::Value& jsonVal);
	bool __ParseCameras(const rapidjson::Value& jsonVal);
	bool __ParseGrasses(const rapidjson::Value& jsonVal);

	bool __CreateModel(const rapidjson::Value& jsonVal);
	bool __CreateInstance(const std::string& hostModelName, const rapidjson::Value& jsonVal);
	bool __CreateLight(const rapidjson::Value& jsonVal);
	bool __CreateCamera(const rapidjson::Value& jsonVal);
	bool __CreateGrassPatch(const rapidjson::Value& jsonVal);

	CScene& m_Scene;
};

