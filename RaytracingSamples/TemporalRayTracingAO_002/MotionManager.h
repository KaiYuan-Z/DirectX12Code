#pragma once
#include "../../GraphicFramework/Core/Utility.h"
#include "../../GraphicFramework/Core/RootSignature.h"
#include "../../GraphicFramework/Core/GraphicsCore.h"
#include "../../GraphicFramework/Core/GraphicsCommon.h"
#include "../../GraphicFramework/Core/ModelInstance.h"


//enum EMotionMode
//{
//	kClockwiseX = 0,
//	kClockwiseZ,
//	kAnticlockwiseX,
//	kAnticlockwiseZ
//};
//
//struct SMotionDesc
//{
//	EMotionMode MotionMode = kClockwiseX;
//	float EndPos = 0.0f;
//
//	SMotionDesc()
//	{
//	}
//
//	SMotionDesc(EMotionMode motionMode, float endPos)
//	{
//		MotionMode = motionMode;
//		EndPos = endPos;
//	}
//};
//
//class CMotionManager
//{
//public:
//	void InitMotionManager(CModelInstance* pHostInst, float baseSpeed, float speedFactor0, float speedFactor1, float rotationLength, SMotionDesc MotionDescSet[4]);
//
//public:
//	static void sDoMotions(float delta);
//
//private:
//	void __DoMotion(float delta);
//
//private:
//	static bool __ClockwiseX(CModelInstance* pIntance, float endX, float rotationLength, float baseSpeed, float speedFactor0, float speedFactor1, float baseAngle, float delta);
//	static bool __ClockwiseZ(CModelInstance* pIntance, float endZ, float rotationLength, float baseSpeed, float speedFactor0, float speedFactor1, float baseAngle, float delta);
//	static bool __AnticlockwiseX(CModelInstance* pIntance, float endX, float rotationLength, float baseSpeed, float speedFactor0, float speedFactor1, float baseAngle, float delta);
//	static bool __AnticlockwiseZ(CModelInstance* pIntance, float endZ, float rotationLength, float baseSpeed, float speedFactor0, float speedFactor1, float baseAngle, float delta);
//
//	
//private:
//	CModelInstance* m_pHostInst = nullptr;
//	SMotionDesc m_MotionDescSet[4];
//	UINT m_CurrentMotionIndex = 0;
//	float m_BaseAngle = 0.0f;
//	float m_BaseSpeed = 0.0f;
//	float m_SpeedFactor0 = 0.0f;
//	float m_SpeedFactor1 = 0.0f;
//	float m_RotationLength = 0.0f;
//
//private:
//	static std::vector<CMotionManager*> s_MotionManagerSet;
//};

enum EMotionMode
{
	kForwardX = 0,
	kForwardZ
};


class CMotionManager
{
public:
	void InitMotionManager(CModelInstance* pHostInst, EMotionMode motionMode, float startPos, float endPos, float speed);

public:
	static void sDoMotions(float delta);

private:
	void __DoMotion(float delta);

private:
	static bool __ClockwiseX(CModelInstance* pIntance, float endX, float rotationLength, float baseSpeed, float speedFactor0, float speedFactor1, float baseAngle, float delta);
	static bool __ClockwiseZ(CModelInstance* pIntance, float endZ, float rotationLength, float baseSpeed, float speedFactor0, float speedFactor1, float baseAngle, float delta);
	static bool __AnticlockwiseX(CModelInstance* pIntance, float endX, float rotationLength, float baseSpeed, float speedFactor0, float speedFactor1, float baseAngle, float delta);
	static bool __AnticlockwiseZ(CModelInstance* pIntance, float endZ, float rotationLength, float baseSpeed, float speedFactor0, float speedFactor1, float baseAngle, float delta);


private:
	CModelInstance* m_pHostInst = nullptr;
	EMotionMode m_MotionMode = kForwardX;
	float m_StartPos = 0.0f;
	float m_EndPos = 0.0f;
	float m_Speed = 0.0f;
	bool m_Reset = false;

private:
	static std::vector<CMotionManager*> s_MotionManagerSet;
};