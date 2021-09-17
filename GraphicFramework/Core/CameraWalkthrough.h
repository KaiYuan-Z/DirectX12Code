#pragma once
#include "CameraBasic.h"

class CCameraWalkthrough:public CCameraBasic
{
public:
	CCameraWalkthrough();
	CCameraWalkthrough(const std::wstring& name);
	virtual ~CCameraWalkthrough();

	// Set frustum.
	void SetLens(float fovY, float aspect, float zn, float zf);

	// Define camera space via setLookAt parameters.
	void SetLookAt(const XMFLOAT3& camPos, const XMFLOAT3& camTarget, const XMFLOAT3& camUp);
	void SetLookAt(const XMVECTOR& camPos, const XMVECTOR& camTarget, const XMVECTOR& camUp);

	// Set move/rotate step.
	void SetMoveStep(float moveStep) { _ASSERTE(moveStep > 0.0f); m_MoveStep = moveStep; }
	void SetRotateStep(float rotateStep) { _ASSERTE(rotateStep > 0.0f); m_RotateStep = rotateStep; }

	// Get frustum properties.
	float GetNearZ() const { return m_NearZ; }
	float GetFarZ() const { return m_FarZ; }
	float GetAspect() const { return m_Aspect; }
	float GetFovY() const { return m_FovY; }
	float GetFovX() const { float halfWidth = 0.5f*GetNearWindowWidth(); return 2.0f*atan(halfWidth / m_NearZ); }

	// Get near and far plane dimensions in view space coordinates.
	float GetNearWindowWidth() const { return m_Aspect * m_NearWindowHeight; }
	float GetNearWindowHeight() const { return m_NearWindowHeight; }
	float GetFarWindowWidth() const { return m_Aspect * m_FarWindowHeight; }
	float GetFarWindowHeight() const { return m_FarWindowHeight; }

	// Get move/rotate step.
	float GetMoveStep() { return m_MoveStep; }
	float GetRotateStep() { return m_RotateStep; }

	// Callback functions
	virtual void OnKeyDown(UINT8 key) override;
	virtual void OnMouseButtonDown(UINT x, UINT y) override;
	virtual void OnMouseMove(UINT x, UINT y) override;

private:

	// strafe/walk the camera a distance d.
	void __Strafe(float distance);
	void __Walk(float distance);
	void __Up(float distance);

	// Rotate the camera.
	void __Pitch(float angle);
	void __RotateY(float angle);

	// Cache frustum properties.
	float m_NearZ;
	float m_FarZ;
	float m_Aspect;
	float m_FovY;
	float m_NearWindowHeight;
	float m_FarWindowHeight;

	float m_MoveStep;
	float m_RotateStep;
	DirectX::XMFLOAT2 m_LastMousePos;		
};