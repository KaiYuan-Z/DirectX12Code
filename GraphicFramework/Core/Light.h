#pragma once
#include <string>
#include "MathUtility.h"

#define LIGHT_TYPE_UNKNOWN		0 
#define LIGHT_TYPE_POINT		1
#define LIGHT_TYPE_DIRECTIONAL	2

struct SLightData
{
	XMFLOAT3 PosW				= { 0.0f, 0.0f, 0.0f };			///< World-space position of the center of a light source
	uint32_t Type               = LIGHT_TYPE_POINT;             ///< Type of the light source (see above)
	XMFLOAT3 DirW				= { 0.0f, -1.0f, 0.0f };		///< World-space orientation of the light source
	float    OpeningAngle       = 3.14159265f;					///< For point (spot) light: Opening angle of a spot light cut-off, pi by default - full-sphere point light
	XMFLOAT3 Intensity          = { 1.0f, 1.0f, 1.0f };			///< Emitted radiance of th light source
	float    CosOpeningAngle    = -1.0f;						///< For point (spot) light: cos(openingAngle), -1 by default because openingAngle is pi by default
	XMFLOAT3 Pad;
	float    PenumbraAngle      = 0.0f;							///< For point (spot) light: Opening angle of penumbra region in radians, usually does not exceed openingAngle. 0.f by default, meaning a spot light with hard cut-off
};

class CPointLight;
class CDirectionalLight;

class CLight
{
public:
	CLight();
	CLight(const std::wstring& lightName, uint32_t lightType);
	virtual ~CLight();

	static CLight* CreateLight(const std::wstring& lightName, uint32_t lightType);

	const SLightData& GetLightData() const { return m_LightData; }

	UINT GetLightType() const { return m_LightData.Type; }

	void SetLightName(const std::wstring& lightName) { m_LightName = lightName; }
	const std::wstring& GetLightName() const { return m_LightName; }

	CPointLight* QueryPiontLight();
	CDirectionalLight* QueryDirectionalLight();

protected:
	std::wstring m_LightName;
	SLightData m_LightData;
};

