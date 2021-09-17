#pragma once
#include "Light.h"

class CPointLight : public CLight
{
public:
	CPointLight();
	CPointLight(const std::wstring& lightName);
	virtual ~CPointLight();

	void SetLightPosW(const XMFLOAT3& posW) { m_LightData.PosW = posW; }
	const XMFLOAT3& GetLightPosW() const { return m_LightData.PosW; }

	void SetLightDirW(const XMFLOAT3& dirW);
	const XMFLOAT3& GetLightDirW() const { return m_LightData.DirW; }

	void SetLightIntensity(const XMFLOAT3& intensity) { m_LightData.Intensity = intensity; }
	const XMFLOAT3& GetLightIntensity() const { return m_LightData.Intensity; }

	void SetLightOpeningAngle(float openingAngle);
	float GetLightOpeningAngle() const { return m_LightData.OpeningAngle; }

	void SetLightPenumbraAngle(float penumbraAngle);
	float GetLightPenumbraAngle() const { return m_LightData.PenumbraAngle; }

	void Move(const XMFLOAT3& position, const XMFLOAT3& target, const XMFLOAT3& up);
};

