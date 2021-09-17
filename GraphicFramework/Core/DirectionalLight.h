#pragma once
#include "Light.h"


class CDirectionalLight : public CLight
{
public:
	CDirectionalLight();
	CDirectionalLight(const std::wstring& lightName);
	virtual ~CDirectionalLight();

	void SetWorldParams(const XMFLOAT3& center, float radius);

	void SetLightDirW(const XMFLOAT3& dirW);
	const XMFLOAT3& GetLightDirW() const { return m_LightData.DirW; }

	void SetLightIntensity(const XMFLOAT3& intensity) { m_LightData.Intensity = intensity; }
	const XMFLOAT3& GetLightIntensity() const { return m_LightData.Intensity; }	

private:
	float m_Distance = 1e3f; ///< Scene bounding radius is required to move the light position sufficiently far away
	XMFLOAT3 m_Center = { 0.0f ,0.0f, 0.0f };
};

