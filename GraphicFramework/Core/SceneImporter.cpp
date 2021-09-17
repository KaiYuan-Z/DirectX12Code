#include "SceneImporter.h"
#include "SceneCommon.h"
#include "CameraWalkthrough.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "ModelManager.h"
#include "GraphicsCore.h"

CSceneImporter::CSceneImporter(CScene& scene) : m_Scene(scene)
{
}

CSceneImporter::~CSceneImporter()
{
}

void CSceneImporter::__ExtraPrepare()
{
	RegisterTokenParseFunc(Keywords::SceneImporter::s_Models, CSceneImporter::__ParseModels);
	RegisterTokenParseFunc(Keywords::SceneImporter::s_Lights, CSceneImporter::__ParseLights);
	RegisterTokenParseFunc(Keywords::SceneImporter::s_Cameras, CSceneImporter::__ParseCameras);
	RegisterTokenParseFunc(Keywords::SceneImporter::s_Grasses, CSceneImporter::__ParseGrasses);
}

bool CSceneImporter::__ParseModels(const rapidjson::Value& jsonVal)
{
	_ASSERTE(jsonVal.IsArray());
	for (UINT i = 0; i < jsonVal.Size(); i++)
	{
		if (!__CreateModel(jsonVal[i])) return false;
	}
	return true;
}

bool CSceneImporter::__ParseLights(const rapidjson::Value& jsonVal)
{
	_ASSERTE(jsonVal.IsArray());
	for (UINT i = 0; i < jsonVal.Size(); i++)
	{
		if (!__CreateLight(jsonVal[i])) return false;
	}
	return true;
}

bool CSceneImporter::__ParseCameras(const rapidjson::Value&  jsonVal)
{
	_ASSERTE(jsonVal.IsArray());
	for (UINT i = 0; i < jsonVal.Size(); i++)
	{
		if (!__CreateCamera(jsonVal[i])) return false;
	}
	return true;;
}

bool CSceneImporter::__ParseGrasses(const rapidjson::Value& jsonVal)
{
	TVariableWrap<int> patchDim;
	TVariableWrap<XMFLOAT3> windDirection;
	TVariableWrap<float> grassHeight, grassThickness, windStrength, positionJitterStrength, bendStrengthAlongTangent;

	for (auto it = jsonVal.MemberBegin(); it != jsonVal.MemberEnd(); it++)
	{
		std::string key(it->name.GetString());
		const auto& value = it->value;

		if (key == Keywords::SceneImporter::s_PatchDim)
		{
			_ASSERTE(value.IsInt());
			patchDim.Set(value.GetInt());
		}
		else if (key == Keywords::SceneImporter::s_WindDir)
		{
			float vec[3];
			_GetFloatVec<3>(value, "wind dir", vec);
			windDirection.Set(XMFLOAT3(vec));
		}
		else if (key == Keywords::SceneImporter::s_Height)
		{
			_ASSERTE(value.IsNumber());
			grassHeight.Set((float)value.GetDouble());
		}
		else if (key == Keywords::SceneImporter::s_Thickness)
		{
			_ASSERTE(value.IsNumber());
			grassThickness.Set((float)value.GetDouble());
		}
		else if (key == Keywords::SceneImporter::s_WindStrength)
		{
			_ASSERTE(value.IsNumber());
			windStrength.Set((float)value.GetDouble());
		}
		else if (key == Keywords::SceneImporter::s_PosJitterStrength)
		{
			_ASSERTE(value.IsNumber());
			positionJitterStrength.Set((float)value.GetDouble());
		}
		else if (key == Keywords::SceneImporter::s_BendStrengthAlongTangent)
		{
			_ASSERTE(value.IsNumber());
			bendStrengthAlongTangent.Set((float)value.GetDouble());
		}
		else if (key == Keywords::SceneImporter::s_Patches)
		{
			_ASSERTE(value.IsArray());
			for (UINT grassPatchIndex = 0; grassPatchIndex < value.Size(); grassPatchIndex++)
			{
				if (!__CreateGrassPatch(value[grassPatchIndex])) return false;
			}
		}
		else
		{
			std::string errStr = "Invalid key found in grass. Key == " + key + ".\n";
			OutputDebugStringA(errStr.c_str());
			return false;
		}
	}

	auto& grassParam = m_Scene.FetchGrassManager().FetchParamter();
	if (patchDim.IsSetted())
	{
		UINT dim = patchDim.Get();
		if (dim > MAX_GRASS_STRAWS_1D) dim = MAX_GRASS_STRAWS_1D;
		grassParam.activePatchDim = XMUINT2(dim, dim);
	}
	if (windDirection.IsSetted()) grassParam.windDirection = windDirection.Get();
	if (grassHeight.IsSetted()) grassParam.grassHeight = grassHeight.Get();
	if (grassThickness.IsSetted()) grassParam.grassThickness = grassThickness.Get();
	if (windStrength.IsSetted()) grassParam.windStrength = windStrength.Get();
	if (positionJitterStrength.IsSetted()) grassParam.positionJitterStrength = positionJitterStrength.Get();
	if (bendStrengthAlongTangent.IsSetted()) grassParam.bendStrengthAlongTangent = bendStrengthAlongTangent.Get();
	
	return true;
}

bool CSceneImporter::__CreateModel(const rapidjson::Value& jsonVal)
{
	TVariableWrap<UINT> type;
	TVariableWrap<std::string> name, file;

	for (auto it = jsonVal.MemberBegin(); it != jsonVal.MemberEnd(); it++)
	{
		std::string key(it->name.GetString());
		const auto& value = it->value;

		if (key == Keywords::SceneImporter::s_Name)
		{
			_ASSERTE(value.IsString());
			name.Set( value.GetString());
		}
		else if (key == Keywords::SceneImporter::s_Type)
		{
			_ASSERTE(value.IsString());
			std::string typeStr = value.GetString();

			if (typeStr == Keywords::SceneImporter::s_Basic)
			{
				type.Set(MODEL_TYPE_BASIC);
			}
			else if (typeStr == Keywords::SceneImporter::s_Sdf)
			{
				type.Set(MODEL_TYPE_SDF);
			}
			else
			{
				std::string errStr = "Invalid model type.\n";
				OutputDebugStringA(errStr.c_str());
				return false;
			}
		}
		else if (key == Keywords::SceneImporter::s_File)
		{
			_ASSERTE(value.IsString());
			file.Set(value.GetString());
		}
		else if (key == Keywords::SceneImporter::s_Instances)
		{
			continue;
		}
		else
		{
			std::string errStr = "Invalid key found in model. Key == " + key + ".\n";
			OutputDebugStringA(errStr.c_str());
			return false;
		}
	}

	UINT modelIndex = ModelManager::AddModel(MakeWStr(name.Get()), MakeWStr(file.Get()), type.Get());
	if (modelIndex == INVALID_OBJECT_INDEX) return false;
	m_Scene.AddModelToScene(MakeWStr(name.Get()));

	for (auto it = jsonVal.MemberBegin(); it != jsonVal.MemberEnd(); it++)
	{
		std::string key(it->name.GetString());
		const auto& value = it->value;

		if (key == Keywords::SceneImporter::s_Name)
		{
			continue;
		}
		else if (key == Keywords::SceneImporter::s_Type)
		{
			continue;
		}
		else if (key == Keywords::SceneImporter::s_File)
		{
			continue;
		}
		else if (key == Keywords::SceneImporter::s_Instances)
		{
			_ASSERTE(value.IsArray());
			for (UINT instanceIndex = 0; instanceIndex < value.Size(); instanceIndex++)
			{
				if (!__CreateInstance(name.Get(), value[instanceIndex])) return false;
			}
		}
		else
		{
			std::string errStr = "Invalid key found in model. Key == " + key + ".\n";
			OutputDebugStringA(errStr.c_str());
			return false;
		}
	}

	return true;
}

bool CSceneImporter::__CreateInstance(const std::string& hostModelName, const rapidjson::Value& jsonVal)
{
	TVariableWrap<XMFLOAT3> pos, yawPitchRoll;
	TVariableWrap<float> scale;
	TVariableWrap<std::string> name, type;

	for (auto it = jsonVal.MemberBegin(); it != jsonVal.MemberEnd(); it++)
	{
		std::string key(it->name.GetString());
		const auto& value = it->value;

		if (key == Keywords::SceneImporter::s_Name)
		{
			_ASSERTE(value.IsString());
			name.Set(value.GetString());
		}
		else if (key == Keywords::SceneImporter::s_Scale)
		{
			_ASSERTE(value.IsNumber());
			scale.Set((float)value.GetDouble());
		}
		else if (key == Keywords::SceneImporter::s_Pos)
		{
			float vec[3];
			_GetFloatVec<3>(value, "instance pos", vec);
			pos.Set(XMFLOAT3(vec));
		}
		else if (key == Keywords::SceneImporter::s_YawPitchRoll)
		{
			float vec[3];
			_GetFloatVec<3>(value, "instance yawPitchRoll", vec);
			yawPitchRoll.Set(XMFLOAT3(XMConvertToRadians(vec[0]), XMConvertToRadians(vec[1]), XMConvertToRadians(vec[2])));
		}
		else if (key == Keywords::SceneImporter::s_Type)
		{
			_ASSERTE(value.IsString());
			type.Set(value.GetString());
		}
		else
		{
			std::string errStr = "Invalid key found in instance. Key == " + key + ".\n";
			OutputDebugStringA(errStr.c_str());
			return false;
		}
	}

	UINT instanceIndex = m_Scene.AddInstance(MakeWStr(name.Get()), MakeWStr(hostModelName));
	if (instanceIndex == INVALID_OBJECT_INDEX) return false;
	CModelInstance* pInstance = m_Scene.GetInstance(instanceIndex);
	_ASSERTE(pInstance);

	pInstance->SetInstance(pos.Get(), yawPitchRoll.Get(), XMFLOAT3(scale.Get(), scale.Get(), scale.Get()));

	if (type.IsSetted())
	{
		if (type.Get() == Keywords::SceneImporter::s_Dynamic)
			pInstance->SetDynamic(true);
		else
			pInstance->SetDynamic(false);
	}

	return true;
}

bool CSceneImporter::__CreateLight(const rapidjson::Value& jsonVal)
{
	TVariableWrap<XMFLOAT3> pos, dir, target, intensity, center;
	TVariableWrap<float> openingAngle, penumbraAngle, radius;
	TVariableWrap<std::string> name;
	TVariableWrap<UINT> type;

	for (auto it = jsonVal.MemberBegin(); it != jsonVal.MemberEnd(); it++)
	{
		std::string key(it->name.GetString());
		const auto& value = it->value;

		if (key == Keywords::SceneImporter::s_Name)
		{
			_ASSERTE(value.IsString());
			name.Set(value.GetString());
		}
		else if (key == Keywords::SceneImporter::s_Type)
		{
			_ASSERTE(value.IsString());
			std::string typeStr = value.GetString();

			if (typeStr == Keywords::SceneImporter::s_Point)
			{
				type.Set(LIGHT_TYPE_POINT);
			}
			else if (typeStr == Keywords::SceneImporter::s_Directional)
			{
				type.Set(LIGHT_TYPE_DIRECTIONAL);
			}
			else
			{
				std::string errStr = "Invalid light type.\n";
				OutputDebugStringA(errStr.c_str());
				return false;
			}
		}
		else if (key == Keywords::SceneImporter::s_OpeningAngle)
		{
			_ASSERTE(value.IsNumber());
			openingAngle.Set((float)value.GetDouble());
		}
		else if (key == Keywords::SceneImporter::s_PenumbraAngle)
		{
			_ASSERTE(value.IsNumber());
			penumbraAngle.Set((float)value.GetDouble());
		}
		else if (key == Keywords::SceneImporter::s_Radius)
		{
			_ASSERTE(value.IsNumber());
			radius.Set((float)value.GetDouble());
		}
		else if (key == Keywords::SceneImporter::s_Pos)
		{
			float vec[3];
			_GetFloatVec<3>(value, "light pos", vec);
			pos.Set(XMFLOAT3(vec));
		}
		else if (key == Keywords::SceneImporter::s_Dir)
		{
			float vec[3];
			_GetFloatVec<3>(value, "light dir", vec);
			dir.Set(XMFLOAT3(vec));
		}
		else if (key == Keywords::SceneImporter::s_Target)
		{
			float vec[3];
			_GetFloatVec<3>(value, "light target", vec);
			target.Set(XMFLOAT3(vec));
		}
		else if (key == Keywords::SceneImporter::s_Intensity)
		{
			float vec[3];
			_GetFloatVec<3>(value, "light intensity", vec);
			intensity.Set(XMFLOAT3(vec));
		}
		else if (key == Keywords::SceneImporter::s_Center)
		{
			float vec[3];
			_GetFloatVec<3>(value, "light center", vec);
			center.Set(XMFLOAT3(vec));
		}
		else
		{
			std::string errStr = "Invalid key found in light. Key == " + key + ".\n";
			OutputDebugStringA(errStr.c_str());
			return false;
		}
	}

	UINT lightIndex = m_Scene.AddLight(MakeWStr(name.Get()), type.Get());
	if (lightIndex == INVALID_OBJECT_INDEX) return false;
	CLight* pLight = m_Scene.GetLight(lightIndex);
	_ASSERTE(pLight);
	switch (type.Get())
	{
	case LIGHT_TYPE_POINT:
	{
		CPointLight* pPointLight = pLight->QueryPiontLight();
		_ASSERTE(pPointLight);
		pPointLight->SetLightPosW(pos.Get());
		pPointLight->SetLightIntensity(intensity.Get());
		if(dir.IsSetted()) pPointLight->SetLightDirW(dir.Get());
		if(openingAngle.IsSetted()) pPointLight->SetLightOpeningAngle(openingAngle.Get());
		if(penumbraAngle.IsSetted()) pPointLight->SetLightPenumbraAngle(penumbraAngle.Get());
		break;
	}
	case LIGHT_TYPE_DIRECTIONAL:
	{
		CDirectionalLight* pDirectionalLight = pLight->QueryDirectionalLight();
		_ASSERTE(pDirectionalLight);
		pDirectionalLight->SetLightDirW(dir.Get());
		pDirectionalLight->SetLightIntensity(intensity.Get());
		if(center.IsSetted() && radius.IsSetted()) pDirectionalLight->SetWorldParams(center.Get(), radius.Get());
		break;
	}
	default:_ASSERT(false);
	}
	
	return true;
}

bool CSceneImporter::__CreateCamera(const rapidjson::Value& jsonVal)
{
	TVariableWrap<XMFLOAT3> pos, target, up;
	TVariableWrap<float> aspect, fovy, zn, zf, moveSpeed, rotateSpeed;
	TVariableWrap<std::string> name;
	TVariableWrap<UINT> type;

	for (auto it = jsonVal.MemberBegin(); it != jsonVal.MemberEnd(); it++)
	{
		std::string key(it->name.GetString());
		const auto& value = it->value;

		if (key == Keywords::SceneImporter::s_Name)
		{
			_ASSERTE(value.IsString());
			name.Set(value.GetString());
		}
		else if (key == Keywords::SceneImporter::s_Type)
		{
			_ASSERTE(value.IsString());
			std::string typeStr = value.GetString();

			if (typeStr == Keywords::SceneImporter::s_Walkthrough)
			{
				type.Set(CAMERA_TYPE_WALKTHROUGH);
			}
			else
			{
				std::string errStr = "Invalid camera type.\n";
				OutputDebugStringA(errStr.c_str());
				return false;
			}
		}
		else if (key == Keywords::SceneImporter::s_Aspect)
		{
			_ASSERTE(value.IsNumber());
			aspect.Set((float)value.GetDouble());
		}
		else if (key == Keywords::SceneImporter::s_FovY)
		{
			_ASSERTE(value.IsNumber());
			fovy.Set(XMConvertToRadians((float)value.GetDouble()));
		}
		else if (key == Keywords::SceneImporter::s_Zn)
		{
			_ASSERTE(value.IsNumber());
			zn.Set((float)value.GetDouble());
		}
		else if (key == Keywords::SceneImporter::s_Zf)
		{
			_ASSERTE(value.IsNumber());
			zf.Set((float)value.GetDouble());
		}
		else if (key == Keywords::SceneImporter::s_MoveSpeed)
		{
			_ASSERTE(value.IsNumber());
			moveSpeed.Set((float)value.GetDouble());
		}
		else if (key == Keywords::SceneImporter::s_RotateSpeed)
		{
			_ASSERTE(value.IsNumber());
			rotateSpeed.Set((float)value.GetDouble());
		}
		else if (key == Keywords::SceneImporter::s_Pos)
		{
			float vec[3];
			_GetFloatVec<3>(value, "camera pos", vec);
			pos.Set(XMFLOAT3(vec));
		}
		else if (key == Keywords::SceneImporter::s_Target)
		{
			float vec[3];
			_GetFloatVec<3>(value, "camera target", vec);
			target.Set(XMFLOAT3(vec));
		}
		else if (key == Keywords::SceneImporter::s_Up)
		{
			float vec[3];
			_GetFloatVec<3>(value, "camera up", vec);
			up.Set(XMFLOAT3(vec));
		}
		else
		{
			std::string errStr = "Invalid key found in camera. Key == " + key + ".\n";
			OutputDebugStringA(errStr.c_str());
			return false;
		}
	}

	UINT cameraIndex = m_Scene.AddCamera(MakeWStr(name.Get()), type.Get());
	if (cameraIndex == INVALID_OBJECT_INDEX) return false;
	CCameraBasic* pCamera = m_Scene.GetCamera(cameraIndex);
	_ASSERTE(pCamera);
	switch(type.Get())
	{
	case CAMERA_TYPE_WALKTHROUGH: 
	{
		CCameraWalkthrough* pCameraWalkthrough = pCamera->QueryWalkthroughCamera();
		_ASSERTE(pCameraWalkthrough);
		if (!aspect.IsSetted()) aspect.Set(double(GraphicsCore::GetWindowWidth()) / double(GraphicsCore::GetWindowHeight()));
		pCameraWalkthrough->SetLens(fovy.Get(), aspect.Get(), zn.Get(), zf.Get());
		pCameraWalkthrough->SetLookAt(pos.Get(), target.Get(), up.Get());
		if(moveSpeed.IsSetted()) pCameraWalkthrough->SetMoveStep(moveSpeed.Get());
		if(rotateSpeed.IsSetted()) pCameraWalkthrough->SetRotateStep(rotateSpeed.Get());
		break;
	}
	default:_ASSERT(false);
	}

	return true;
}

bool CSceneImporter::__CreateGrassPatch(const rapidjson::Value& jsonVal)
{
	TVariableWrap<XMFLOAT3> pos;
	TVariableWrap<float> scale;

	for (auto it = jsonVal.MemberBegin(); it != jsonVal.MemberEnd(); it++)
	{
		std::string key(it->name.GetString());
		const auto& value = it->value;

		if (key == Keywords::SceneImporter::s_Scale)
		{
			_ASSERTE(value.IsNumber());
			scale.Set((float)value.GetDouble());
		}
		else if (key == Keywords::SceneImporter::s_Pos)
		{
			float vec[3];
			_GetFloatVec<3>(value, "grass patch pos", vec);
			pos.Set(XMFLOAT3(vec));
		}
		else
		{
			std::string errStr = "Invalid key found in grass patch. Key == " + key + ".\n";
			OutputDebugStringA(errStr.c_str());
			return false;
		}
	}

	if (scale.IsSetted())
	{
		m_Scene.FetchGrassManager().AddGrassPatch(pos.Get(), scale.Get());
	}
	else
	{
		m_Scene.FetchGrassManager().AddGrassPatch(pos.Get());
	}

	return true;
}
