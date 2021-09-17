#include "PointLight.h"


CPointLight::CPointLight() : CPointLight(L"")
{
}

CPointLight::CPointLight(const std::wstring& lightName) : CLight(lightName, LIGHT_TYPE_POINT)
{
}

CPointLight::~CPointLight()
{
}

void CPointLight::SetLightDirW(const XMFLOAT3& dirW)
{
	XMVECTOR DirXMV = XMVector3Normalize(XMLoadFloat3(&dirW));
	XMStoreFloat3(&m_LightData.DirW, DirXMV);
}

void CPointLight::SetLightOpeningAngle(float openingAngle)
{	
	openingAngle = MathUtility::Clamp(openingAngle, 0.f, XM_PI);
	m_LightData.OpeningAngle = openingAngle;
	// Prepare an auxiliary cosine of the opening angle to quickly check whether we're within the cone of a spot light
	m_LightData.CosOpeningAngle = cos(openingAngle);
}

void CPointLight::SetLightPenumbraAngle(float penumbraAngle)
{
	m_LightData.PenumbraAngle = MathUtility::Clamp(penumbraAngle, 0.0f, m_LightData.OpeningAngle);
}

void CPointLight::Move(const XMFLOAT3& position, const XMFLOAT3& target, const XMFLOAT3& up)
{
	m_LightData.PosW = position;
	m_LightData.DirW = { target.x - position.x, target.y - position.y, target.z - position.z };
}
