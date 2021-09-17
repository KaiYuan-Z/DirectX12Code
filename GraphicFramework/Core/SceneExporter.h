#pragma once
#include "JsonConfigExporter.h"
#include "Scene.h"

class CSceneExporter : public CJsonConfigExporter
{
public:
	CSceneExporter(CScene& scene);
	~CSceneExporter();

private:
	virtual void __PrepareConfig(rapidjson::Value& jdoc, rapidjson::Document::AllocatorType& jallocator, uint32_t exportOptions);

	void __ExportModels(rapidjson::Value& jdoc, rapidjson::Document::AllocatorType& jallocator);
	void __ExportLights(rapidjson::Value& jdoc, rapidjson::Document::AllocatorType& jallocator);
	void __ExportCameras(rapidjson::Value& jdoc, rapidjson::Document::AllocatorType& jallocator);
	void __ExportGrasses(rapidjson::Value& jdoc, rapidjson::Document::AllocatorType& jallocator);

	CScene& m_Scene;
};

