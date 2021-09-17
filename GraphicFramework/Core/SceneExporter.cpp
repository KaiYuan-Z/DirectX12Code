#include "SceneExporter.h"
#include "SceneCommon.h"
#include "CameraWalkthrough.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "ModelManager.h"
#include "GraphicsCore.h"

CSceneExporter::CSceneExporter(CScene& scene) : m_Scene(scene)
{
}

CSceneExporter::~CSceneExporter()
{
}

void CSceneExporter::__PrepareConfig(rapidjson::Value& jdoc, rapidjson::Document::AllocatorType& jallocator, uint32_t exportOptions)
{
	if (exportOptions & ExportOptions::ExportModels)            __ExportModels(jdoc, jallocator);
	if (exportOptions & ExportOptions::ExportLights)            __ExportLights(jdoc, jallocator);
	if (exportOptions & ExportOptions::ExportCameras)           __ExportCameras(jdoc, jallocator);
	if (exportOptions & ExportOptions::ExportGrasses)           __ExportGrasses(jdoc, jallocator);
}

void CSceneExporter::__ExportModels(rapidjson::Value& jdoc, rapidjson::Document::AllocatorType& jallocator)
{
	rapidjson::Value jsonModelArray(rapidjson::kArrayType);

	for (UINT m = 0; m < m_Scene.GetModelCountInScene(); m++)
	{
		CModel* pModel = m_Scene.GetModelInScene(m);
		_ASSERTE(pModel);

		rapidjson::Value jsonModel;
		jsonModel.SetObject();

		std::string name = MakeStr(pModel->GetModelName());
		std::string file = MakeStr(pModel->GetModelFileName());
		std::string type = "";
		switch (pModel->GetModelType())
		{
		case MODEL_TYPE_BASIC: type = Keywords::SceneImporter::s_Basic; break;
		case MODEL_TYPE_SDF: type = Keywords::SceneImporter::s_Sdf; break;
		default: break;
		}

		_AddString(jsonModel, jallocator, Keywords::SceneImporter::s_Name, name);
		_AddString(jsonModel, jallocator, Keywords::SceneImporter::s_File, file);
		_AddString(jsonModel, jallocator, Keywords::SceneImporter::s_Type, type);

		rapidjson::Value jsonInstanceArray(rapidjson::kArrayType);
		for (UINT i = 0; i < m_Scene.GetModelInstanceCount(m); i++)
		{
			CModelInstance* pInstance = m_Scene.GetModelInstance(m, i);
			_ASSERTE(pInstance);

			rapidjson::Value jsonInstance;
			jsonInstance.SetObject();

			std::string name = MakeStr(pInstance->GetName());
			std::string type = pInstance->IsDynamic() ? Keywords::SceneImporter::s_Dynamic : Keywords::SceneImporter::s_Static;
			float scale = pInstance->GetScaling().x;
			XMFLOAT3 pos = pInstance->GetTranslation();
			//XMFLOAT3 target = pInstance->GetTarget();
			//XMFLOAT3 up = pInstance->GetUpVector();
			XMFLOAT3 yawPitchRoll = XMFLOAT3(XMConvertToDegrees(pInstance->GetRotation().x), XMConvertToDegrees(pInstance->GetRotation().y), XMConvertToDegrees(pInstance->GetRotation().z));

			_AddString(jsonInstance, jallocator, Keywords::SceneImporter::s_Name, name);
			_AddString(jsonInstance, jallocator, Keywords::SceneImporter::s_Type, type);
			_AddScalar(jsonInstance, jallocator, Keywords::SceneImporter::s_Scale, scale);
			_AddVector(jsonInstance, jallocator, Keywords::SceneImporter::s_Pos, (float*)(&pos), 3);
			//_AddVector(jsonInstance, jallocator, Keywords::SceneImporter::s_Target, (float*)(&target), 3);
			//_AddVector(jsonInstance, jallocator, Keywords::SceneImporter::s_Up, (float*)(&up), 3);
			_AddVector(jsonInstance, jallocator, Keywords::SceneImporter::s_YawPitchRoll, (float*)(&yawPitchRoll), 3);

			jsonInstanceArray.PushBack(jsonInstance, jallocator);
		}
		_AddJsonValue(jsonModel, jallocator, Keywords::SceneImporter::s_Instances, jsonInstanceArray);
		
		jsonModelArray.PushBack(jsonModel, jallocator);
	}

	_AddJsonValue(jdoc, jallocator, Keywords::SceneImporter::s_Models, jsonModelArray);
}

void CSceneExporter::__ExportLights(rapidjson::Value& jdoc, rapidjson::Document::AllocatorType& jallocator)
{
	auto CreatePointLightValue = [&](CPointLight* pLight, rapidjson::Document::AllocatorType& allocator, rapidjson::Value& jsonLight) {
		_ASSERTE(pLight);

		jsonLight.SetObject();

		std::string name = MakeStr(pLight->GetLightName());
		float openingAngle = pLight->GetLightOpeningAngle();
		float penumbraAngle = pLight->GetLightPenumbraAngle();
		XMFLOAT3 pos = pLight->GetLightPosW();
		XMFLOAT3 dir = pLight->GetLightDirW();
		XMFLOAT3 intensity = pLight->GetLightIntensity();

		_AddString(jsonLight, allocator, Keywords::SceneImporter::s_Name, name);
		_AddString(jsonLight, allocator, Keywords::SceneImporter::s_Type, Keywords::SceneImporter::s_Point);
		_AddScalar(jsonLight, allocator, Keywords::SceneImporter::s_OpeningAngle, openingAngle);
		_AddScalar(jsonLight, allocator, Keywords::SceneImporter::s_PenumbraAngle, penumbraAngle);
		_AddVector(jsonLight, allocator, Keywords::SceneImporter::s_Pos, (float*)(&pos), 3);
		_AddVector(jsonLight, allocator, Keywords::SceneImporter::s_Dir, (float*)(&dir), 3);
		_AddVector(jsonLight, allocator, Keywords::SceneImporter::s_Intensity, (float*)(&intensity), 3);
	};

	auto CreateDirectionalLightValue = [&](CDirectionalLight* pLight, rapidjson::Document::AllocatorType& allocator, rapidjson::Value& jsonLight) {
		_ASSERTE(pLight);

		jsonLight.SetObject();

		std::string name = MakeStr(pLight->GetLightName());
		XMFLOAT3 dir = pLight->GetLightDirW();
		XMFLOAT3 intensity = pLight->GetLightIntensity();

		_AddString(jsonLight, allocator, Keywords::SceneImporter::s_Name, name);
		_AddString(jsonLight, allocator, Keywords::SceneImporter::s_Type, Keywords::SceneImporter::s_Directional);
		_AddVector(jsonLight, allocator, Keywords::SceneImporter::s_Dir, (float*)(&dir), 3);
		_AddVector(jsonLight, allocator, Keywords::SceneImporter::s_Intensity, (float*)(&intensity), 3);
	};

	rapidjson::Value jsonLightArray(rapidjson::kArrayType);

	for (UINT i = 0; i < m_Scene.GetLightCount(); i++)
	{
		CLight* pLight = m_Scene.GetLight(i);
		_ASSERTE(pLight);

		rapidjson::Value jsonLight;

		switch (pLight->GetLightType())
		{
		case LIGHT_TYPE_POINT: CreatePointLightValue(pLight->QueryPiontLight(), jallocator, jsonLight); break;
		case LIGHT_TYPE_DIRECTIONAL: CreateDirectionalLightValue(pLight->QueryDirectionalLight(), jallocator, jsonLight); break;
		default: break;
		}

		jsonLightArray.PushBack(jsonLight, jallocator);
	}

	_AddJsonValue(jdoc, jallocator, Keywords::SceneImporter::s_Lights, jsonLightArray);
}

void CSceneExporter::__ExportCameras(rapidjson::Value& jdoc, rapidjson::Document::AllocatorType& jallocator)
{
	auto CreateWalkthroughCameraValue = [&](CCameraWalkthrough* pCamera, rapidjson::Document::AllocatorType& allocator, rapidjson::Value& jsonCamera) {
		_ASSERTE(pCamera);

		jsonCamera.SetObject();

		std::string name = MakeStr(pCamera->GetCameraName());
		float aspect = pCamera->GetAspect();
		float fovy = XMConvertToDegrees(pCamera->GetFovY());
		float zn = pCamera->GetNearZ();
		float zf = pCamera->GetFarZ();
		float moveSpeed = pCamera->GetMoveStep();
		float rotateSpeed = pCamera->GetRotateStep();
		XMFLOAT3 pos = pCamera->GetPositionXMF3(); 
		XMFLOAT3 up = pCamera->GetUpXMF3();
		XMFLOAT3 target = pCamera->GetVirtualTargetXMF3();

		_AddString(jsonCamera, allocator, Keywords::SceneImporter::s_Name, name);
		_AddString(jsonCamera, allocator, Keywords::SceneImporter::s_Type, Keywords::SceneImporter::s_Walkthrough);
		_AddScalar(jsonCamera, allocator, Keywords::SceneImporter::s_Aspect, aspect);
		_AddScalar(jsonCamera, allocator, Keywords::SceneImporter::s_FovY, fovy);
		_AddScalar(jsonCamera, allocator, Keywords::SceneImporter::s_Zn, zn);
		_AddScalar(jsonCamera, allocator, Keywords::SceneImporter::s_Zf, zf);
		_AddScalar(jsonCamera, allocator, Keywords::SceneImporter::s_MoveSpeed, moveSpeed);
		_AddScalar(jsonCamera, allocator, Keywords::SceneImporter::s_RotateSpeed, rotateSpeed);
		_AddVector(jsonCamera, allocator, Keywords::SceneImporter::s_Pos, (float*)(&pos), 3);
		_AddVector(jsonCamera, allocator, Keywords::SceneImporter::s_Up, (float*)(&up), 3);
		_AddVector(jsonCamera, allocator, Keywords::SceneImporter::s_Target, (float*)(&target), 3);
	};

	rapidjson::Value jsonCameraArray(rapidjson::kArrayType);

	for (UINT i = 0; i < m_Scene.GetCameraCount(); i++)
	{
		CCameraBasic* pCamera = m_Scene.GetCamera(i);
		_ASSERTE(pCamera);

		rapidjson::Value jsonCamera;

		switch (pCamera->GetCameraType())
		{
		case CAMERA_TYPE_WALKTHROUGH:CreateWalkthroughCameraValue(pCamera->QueryWalkthroughCamera(), jallocator, jsonCamera); break;
		default:break;
		}

		jsonCameraArray.PushBack(jsonCamera, jallocator);
	}

	_AddJsonValue(jdoc, jallocator, Keywords::SceneImporter::s_Cameras, jsonCameraArray);
}

void CSceneExporter::__ExportGrasses(rapidjson::Value& jdoc, rapidjson::Document::AllocatorType& jallocator)
{
	auto& grassManager = m_Scene.FetchGrassManager();
	auto& grassParam = m_Scene.FetchGrassManager().FetchParamter();
	
	rapidjson::Value jsonGrasses;
	jsonGrasses.SetObject();

	_AddScalar(jsonGrasses, jallocator, Keywords::SceneImporter::s_PatchDim, (int)grassParam.activePatchDim.x);
	_AddVector(jsonGrasses, jallocator, Keywords::SceneImporter::s_WindDir, (float*)(&grassParam.windDirection), 3);
	_AddScalar(jsonGrasses, jallocator, Keywords::SceneImporter::s_Height, grassParam.grassHeight);
	_AddScalar(jsonGrasses, jallocator, Keywords::SceneImporter::s_Thickness, grassParam.grassThickness);
	_AddScalar(jsonGrasses, jallocator, Keywords::SceneImporter::s_WindStrength, grassParam.windStrength);
	_AddScalar(jsonGrasses, jallocator, Keywords::SceneImporter::s_PosJitterStrength, grassParam.positionJitterStrength);
	_AddScalar(jsonGrasses, jallocator, Keywords::SceneImporter::s_BendStrengthAlongTangent, grassParam.bendStrengthAlongTangent);

	rapidjson::Value jsonPatchArray(rapidjson::kArrayType);
	for (UINT i = 0; i < grassManager.GetGrarssPatchCount(); i++)
	{
		rapidjson::Value jsonPatch;
		jsonPatch.SetObject();

		float scale = grassManager.GetGrarssPatchScale(i);
		XMFLOAT3 pos = grassManager.GetGrarssPatchPosition(i);
		
		_AddScalar(jsonPatch, jallocator, Keywords::SceneImporter::s_Scale, scale);
		_AddVector(jsonPatch, jallocator, Keywords::SceneImporter::s_Pos, (float*)(&pos), 3);

		jsonPatchArray.PushBack(jsonPatch, jallocator);
	}

	_AddJsonValue(jsonGrasses, jallocator, Keywords::SceneImporter::s_Patches, jsonPatchArray);

	_AddJsonValue(jdoc, jallocator, Keywords::SceneImporter::s_Grasses, jsonGrasses);
}
