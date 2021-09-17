#include "DirectionalLight.h"



CDirectionalLight::CDirectionalLight() : CDirectionalLight(L"")
{
}

CDirectionalLight::CDirectionalLight(const std::wstring& lightName) : CLight(lightName, LIGHT_TYPE_DIRECTIONAL)
{
}

CDirectionalLight::~CDirectionalLight()
{
}

void CDirectionalLight::SetWorldParams(const XMFLOAT3& center, float radius)
{
	m_Distance = radius;
	m_Center = center;
	XMStoreFloat3(&m_LightData.PosW, XMLoadFloat3(&m_Center) - XMLoadFloat3(&m_LightData.DirW) * m_Distance);
}

void CDirectionalLight::SetLightDirW(const XMFLOAT3& dirW)
{
	XMVECTOR DirXMV = XMVector3Normalize(XMLoadFloat3(&dirW));
	XMStoreFloat3(&m_LightData.DirW, DirXMV);
	XMStoreFloat3(&m_LightData.PosW, XMLoadFloat3(&m_Center) - DirXMV * m_Distance); // Move light's position sufficiently far away
}
