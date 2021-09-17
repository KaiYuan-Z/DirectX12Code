#include "Light.h"
#include "PointLight.h"
#include "DirectionalLight.h"

CLight::CLight() : CLight(L"", LIGHT_TYPE_UNKNOWN)
{
}

CLight::CLight(const std::wstring& lightName, uint32_t lightType)
{
	m_LightName = lightName;
	m_LightData.Type = lightType;
}

CLight::~CLight()
{
}

CLight* CLight::CreateLight(const std::wstring& lightName, uint32_t lightType)
{
	CLight* pLight = nullptr;
	switch (lightType)
	{
	case LIGHT_TYPE_POINT: pLight = new CPointLight(lightName); break;
	case LIGHT_TYPE_DIRECTIONAL: pLight = new CDirectionalLight(lightName); break;
	default:_ASSERT(pLight);
	}
	_ASSERTE(pLight);
	return pLight;
}

CPointLight* CLight::QueryPiontLight()
{
	_ASSERTE(m_LightData.Type == LIGHT_TYPE_POINT); 
	return static_cast<CPointLight*>(this);
}

CDirectionalLight * CLight::QueryDirectionalLight()
{
	_ASSERTE(m_LightData.Type == LIGHT_TYPE_DIRECTIONAL);
	return static_cast<CDirectionalLight*>(this);
}
