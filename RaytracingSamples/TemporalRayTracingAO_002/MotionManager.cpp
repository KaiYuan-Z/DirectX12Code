#include "MotionManager.h"


std::vector<CMotionManager*> CMotionManager::s_MotionManagerSet;

void CMotionManager::InitMotionManager(CModelInstance* pHostInst, EMotionMode motionMode, float startPos, float endPos, float speed)
{
	_ASSERTE(pHostInst);
	m_pHostInst = pHostInst;
	m_MotionMode = motionMode;
	m_StartPos = startPos;
	m_EndPos = endPos;
	m_Speed = speed;
	s_MotionManagerSet.push_back(this);
}

void CMotionManager::sDoMotions(float delta)
{
	for (UINT i = 0; i < s_MotionManagerSet.size(); i++)
	{
		_ASSERTE(s_MotionManagerSet[i]);
		s_MotionManagerSet[i]->__DoMotion(delta);
	}
}

void CMotionManager::__DoMotion(float delta)
{
	auto GoOnX = [&](CModelInstance* pIntance, float startX, float endX, float speed, float delta, bool reset = false) {
		XMFLOAT3 translation = pIntance->GetTranslation();
		if (reset)
		{
			translation.x = startX;
		}
		if (endX > startX)
		{
			if (translation.x > endX)
			{
				return true;
			}
			translation.x += delta * speed;
		}
		else
		{
			if (translation.x < endX)
			{
				return true;
			}
			translation.x -= delta * speed;
		}
		pIntance->SetTranslation(translation);
		return false;
	};

	auto GoOnZ = [&](CModelInstance* pIntance, float startZ, float endZ, float speed, float delta, bool reset = false) {
		XMFLOAT3 translation = pIntance->GetTranslation();
		if (reset)
		{
			translation.z = startZ;
		}
		if (endZ > startZ)
		{
			if (translation.z > endZ)
			{
				return true;
			}
			translation.z += delta * speed;
		}
		else
		{
			if (translation.z < endZ)
			{
				return true;
			}
			translation.z -= delta * speed;
		}
		pIntance->SetTranslation(translation);
		return false;
	};
	
	switch (m_MotionMode)
	{
	case kForwardX: m_Reset = GoOnX(m_pHostInst, m_StartPos, m_EndPos, m_Speed, delta, m_Reset); break;
	case kForwardZ: m_Reset = GoOnZ(m_pHostInst, m_StartPos, m_EndPos, m_Speed, delta, m_Reset); break;
	default: _ASSERT(false);
	}
}

//void CMotionManager::InitMotionManager(CModelInstance* pHostInst, float baseSpeed, float speedFactor0, float speedFactor1, float rotationLength, SMotionDesc MotionDescSet[4])
//{
//	_ASSERTE(pHostInst);
//	m_pHostInst = pHostInst;
//	m_BaseAngle = round(XMConvertToDegrees(m_pHostInst->GetRotation().y));
//	m_BaseSpeed = baseSpeed;
//	m_SpeedFactor0 = speedFactor0;
//	m_SpeedFactor1 = speedFactor1;
//	m_RotationLength = rotationLength;
//	for (UINT i = 0; i < 4; i++)
//	{
//		m_MotionDescSet[i] = MotionDescSet[i];
//	}
//	s_MotionManagerSet.push_back(this);
//}
//
//void CMotionManager::sDoMotions(float delta)
//{
//	for (UINT i = 0; i < s_MotionManagerSet.size(); i++)
//	{
//		_ASSERTE(s_MotionManagerSet[i]);
//		s_MotionManagerSet[i]->__DoMotion(delta);
//	}
//}
//
//void CMotionManager::__DoMotion(float delta)
//{
//	bool isFinish = false;
//	auto& motionDesc = m_MotionDescSet[m_CurrentMotionIndex];
//
//	switch (motionDesc.MotionMode)
//	{
//	case kClockwiseX: isFinish = __ClockwiseX(m_pHostInst, motionDesc.EndPos, m_RotationLength, m_BaseSpeed, m_SpeedFactor0, m_SpeedFactor1, m_BaseAngle + 90 * m_CurrentMotionIndex, delta); break;
//	case kClockwiseZ: isFinish = __ClockwiseZ(m_pHostInst, motionDesc.EndPos, m_RotationLength, m_BaseSpeed, m_SpeedFactor0, m_SpeedFactor1, m_BaseAngle + 90 * m_CurrentMotionIndex, delta); break;
//	case kAnticlockwiseX: isFinish = __AnticlockwiseX(m_pHostInst, motionDesc.EndPos, m_RotationLength, m_BaseSpeed, m_SpeedFactor0, m_SpeedFactor1, m_BaseAngle - 90 * m_CurrentMotionIndex, delta); break;
//	case kAnticlockwiseZ: isFinish = __AnticlockwiseZ(m_pHostInst, motionDesc.EndPos, m_RotationLength, m_BaseSpeed, m_SpeedFactor0, m_SpeedFactor1, m_BaseAngle - 90 * m_CurrentMotionIndex, delta); break;
//	default: _ASSERT(false);
//	}
//
//	if (isFinish)
//	{
//		m_CurrentMotionIndex = (m_CurrentMotionIndex + 1) % 4;
//	}
//}
//
//bool CMotionManager::__ClockwiseX(CModelInstance* pIntance, float endX, float rotationLength, float baseSpeed, float speedFactor0, float speedFactor1, float baseAngle, float delta)
//{
//	bool isFinish = false;
//	XMFLOAT3 pos = pIntance->GetTranslation();
//	bool isLittleEnd = endX < pos.x;
//	float speed0 = abs(pos.x - endX) < rotationLength ? baseSpeed * speedFactor0 : baseSpeed;
//	if (!isLittleEnd)
//	{
//		pos.x += delta * speed0;
//		if (pos.x >= endX)
//		{
//			pos.x = endX;
//			isFinish = true;
//		}
//	}
//	else
//	{
//		pos.x -= delta * speed0;
//		if (pos.x <= endX)
//		{
//			pos.x = endX;
//			isFinish = true;
//		}
//	}
//
//	if (abs(pos.x - endX) < rotationLength)
//	{
//		float factor = (rotationLength - abs(pos.x - endX)) / rotationLength;
//		float angle = baseAngle + 90.0f * factor;
//		while (angle >= 360) angle -= 360;
//		pIntance->SetRotation({ 0.0f, XMConvertToRadians(angle), 0.0f });
//		float speed1 = speed0;
//		if (angle > 30)
//		{
//			float s = (angle - 30.f) / 30.f;
//			s = s > 1.0f ? 1.0f : s;
//			speed1 += speed0 * s * speedFactor1;
//		}
//		pos.z += isLittleEnd ? delta * speed1 : -delta * speed1;
//	}
//
//	pIntance->SetTranslation(pos);
//
//	return isFinish;
//}
//
//bool CMotionManager::__ClockwiseZ(CModelInstance* pIntance, float endZ, float rotationLength, float baseSpeed, float speedFactor0, float speedFactor1, float baseAngle, float delta)
//{
//	bool isFinish = false;
//	XMFLOAT3 pos = pIntance->GetTranslation();
//	bool isLittleEnd = endZ < pos.z;
//	float speed0 = abs(pos.z - endZ) < rotationLength ? baseSpeed * speedFactor0 : baseSpeed;
//	if (!isLittleEnd)
//	{
//		pos.z += delta * speed0;
//		if (pos.z >= endZ)
//		{
//			pos.z = endZ;
//			isFinish = true;
//		}
//	}
//	else
//	{
//		pos.z -= delta * speed0;
//		if (pos.z <= endZ)
//		{
//			pos.z = endZ;
//			isFinish = true;
//		}
//	}
//
//	if (abs(pos.z - endZ) < rotationLength)
//	{
//		float factor = (rotationLength - abs(pos.z - endZ)) / rotationLength;
//		float angle = baseAngle + 90.0f * factor;
//		while (angle >= 360) angle -= 360;
//		pIntance->SetRotation({ 0.0f, XMConvertToRadians(angle), 0.0f });
//		float speed1 = speed0;
//		if (angle > 30)
//		{
//			float s = (angle - 30.f) / 30.f;
//			s = s > 1.0f ? 1.0f : s;
//			speed1 += speed0 * s * speedFactor1;
//		}
//		pos.x += isLittleEnd ? -delta * speed1 : delta * speed1;
//	}
//
//	pIntance->SetTranslation(pos);
//
//	return isFinish;
//}
//
//bool CMotionManager::__AnticlockwiseX(CModelInstance* pIntance, float endX, float rotationLength, float baseSpeed, float speedFactor0, float speedFactor1, float baseAngle, float delta)
//{
//	bool isFinish = false;
//	XMFLOAT3 pos = pIntance->GetTranslation();
//	bool isLittleEnd = endX < pos.x;
//	float speed0 = abs(pos.x - endX) < rotationLength ? baseSpeed * speedFactor0 : baseSpeed;
//	if (!isLittleEnd)
//	{
//		pos.x += delta * speed0;
//		if (pos.x >= endX)
//		{
//			pos.x = endX;
//			isFinish = true;
//		}
//	}
//	else
//	{
//		pos.x -= delta * speed0;
//		if (pos.x <= endX)
//		{
//			pos.x = endX;
//			isFinish = true;
//		}
//	}
//
//	if (abs(pos.x - endX) < rotationLength)
//	{
//		float factor = (rotationLength - abs(pos.x - endX)) / rotationLength;
//		float angle = baseAngle - 90.0f * factor;
//		while (angle <= -360) angle += 360;
//		pIntance->SetRotation({ 0.0f, XMConvertToRadians(angle), 0.0f });
//		float speed1 = speed0;
//		if (angle > 30.0f)
//		{
//			float s = (angle - 30.0f) / 30.f;
//			s = s > 1.0f ? 1.0f : s;
//			speed1 += speed0 * s * speedFactor1;
//		}
//		pos.z += isLittleEnd ? -delta * speed1 : delta * speed1;
//	}
//
//	pIntance->SetTranslation(pos);
//
//	return isFinish;
//}
//
//bool CMotionManager::__AnticlockwiseZ(CModelInstance* pIntance, float endZ, float rotationLength, float baseSpeed, float speedFactor0, float speedFactor1, float baseAngle, float delta)
//{
//	bool isFinish = false;
//	XMFLOAT3 pos = pIntance->GetTranslation();
//	bool isLittleEnd = endZ < pos.z;
//	float speed0 = abs(pos.z - endZ) < rotationLength ? baseSpeed * speedFactor0 : baseSpeed;
//	if (!isLittleEnd)
//	{
//		pos.z += delta * speed0;
//		if (pos.z >= endZ)
//		{
//			pos.z = endZ;
//			isFinish = true;
//		}
//	}
//	else
//	{
//		pos.z -= delta * speed0;
//		if (pos.z <= endZ)
//		{
//			pos.z = endZ;
//			isFinish = true;
//		}
//	}
//
//	if (abs(pos.z - endZ) < rotationLength)
//	{
//		float factor = (rotationLength - abs(pos.z - endZ)) / rotationLength;
//		float angle = baseAngle - 90.0f * factor;
//		while (angle <= -360) angle += 360;
//		pIntance->SetRotation({ 0.0f, XMConvertToRadians(angle), 0.0f });
//		float speed1 = speed0;
//		if (angle > 30.0f)
//		{
//			float s = (angle - 30.0f) / 30.0f;
//			s = s > 1.0f ? 1.0f : s;
//			speed1 += speed0 * s * speedFactor1;
//		}
//		pos.x += isLittleEnd ? delta * speed1 : -delta * speed1;
//	}
//
//	pIntance->SetTranslation(pos);
//
//	return isFinish;
//}
