#pragma once
#include "GraphicsCoreBase.h"

#define CAMERA_TYPE_UNKNOWN			0
#define CAMERA_TYPE_BASIC			1
#define CAMERA_TYPE_WALKTHROUGH		2

using namespace DirectX;

class CCameraWalkthrough;

class CCameraBasic
{
public:
	CCameraBasic(void);
	CCameraBasic(const std::wstring& name);
	virtual ~CCameraBasic(void);

	static CCameraBasic* CreateCamera(const std::wstring& cameraName, UINT cameraType);

	void SetCamera(const XMFLOAT3& camPos, const XMFLOAT3& camRight, const XMFLOAT3& camUp, const XMFLOAT3& camLook);

	void SetJitter(float jitterDeltaX, float jitterDeltaY);
	void DumpJitter(float& jitterX, float& jitterY) const { jitterX = m_JitterX; jitterY = m_JitterY; }
	void DumpPreJitter(float& jitterX, float& jitterY) const { jitterX = m_PreJitterX; jitterY = m_PreJitterY; }

	// Get world camera position.
	const XMFLOAT3& GetPositionXMF3() const { return m_Position; }
	XMVECTOR GetPositionXMV() const { return XMLoadFloat3(&m_Position); }
	const XMFLOAT3& GetPrePositionXMF3() const { return m_PrePosition; }
	const XMFLOAT3& GetVirtualTargetXMF3() const { return m_VirtualTarget; }
	
	// Get camera basis vectors.
	const XMFLOAT3& GetRightXMF3() const { return m_Right; }
	XMVECTOR GetRightXMV() const { return XMLoadFloat3(&m_Right); }
	const XMFLOAT3& GetUpXMF3() const { return m_Up; }
	XMVECTOR GetUpXMV() const { return XMLoadFloat3(&m_Up); }
	const XMFLOAT3& GetLookXMF3() const { return m_Look; }
	XMVECTOR GetLookXMV() const { return XMLoadFloat3(&m_Look); }

	// Get View/Proj matrices.
	const XMFLOAT4X4& GetViewXMF4X4() const { return m_View; }
	XMMATRIX GetViewXMM() const { return XMLoadFloat4x4(&m_View); }
	const XMFLOAT4X4& GetPreViewXMF4X4() const { return m_PreView; }
	XMMATRIX GetPreViewXMM() const { return XMLoadFloat4x4(&m_PreView); }
	XMFLOAT4X4 GetProjectXMF4X4(bool withJitter = true) const;
	XMMATRIX GetProjectXMM(bool withJitter = true) const;
	XMMATRIX GetPreProjectXMM(bool withJitter = true) const;

	// Get Bounding Frustum
	const DirectX::BoundingFrustum& GetBoundingFrustum() const { return m_BoundingFrustum; }

	// Get camera modifying state.
	bool IsCameraModified() const { return m_DirtyHint; }

	// After modifying camera position/orientation, call to rebuild the view matrix.
	void OnFrameUpdate();
	void OnFramePostProcess();
	UINT GetLastUpdateFrameID() const { return m_LastUpdateFrameID; }
	bool IsUpdatedInCurrentFrame() const;

	// Callback functions
	virtual void OnKeyDown(UINT8 key);
	virtual void OnMouseMove(UINT /*x*/, UINT /*y*/) {}
	virtual void OnMouseButtonDown(UINT /*x*/, UINT /*y*/) {}
	virtual void OnMouseButtonUp(UINT /*x*/, UINT /*y*/) {}

	UINT GetCameraType() const { return m_CameraType; }

	const std::wstring& GetCameraName() const { return m_CameraName; }

	CCameraWalkthrough* QueryWalkthroughCamera();

protected:
	void _SetDirtyHint(bool isDirty) { m_DirtyHint = isDirty; }
	void _ComputeBoundingFrustum() { DirectX::XMMATRIX Proj = GetProjectXMM(); DirectX::BoundingFrustum::CreateFromMatrix(m_BoundingFrustum, Proj); }

	UINT m_CameraType = CAMERA_TYPE_UNKNOWN;

    // Camera coordinate system with coordinates relative to world space.
	XMFLOAT3 m_Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 m_PrePosition = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 m_Right = XMFLOAT3(1.0f, 0.0f, 0.0f);
	XMFLOAT3 m_Up = XMFLOAT3(0.0f, 1.0f, 0.0f);
	XMFLOAT3 m_Look = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3 m_VirtualTarget = XMFLOAT3(0.0f, 0.0f, 0.0f);

	// Cache View/Proj matrices.
	XMFLOAT4X4 m_View = MathUtility::Identity4x4();
	XMFLOAT4X4 m_Proj = MathUtility::Identity4x4();
	XMFLOAT4X4 m_PreView = MathUtility::Identity4x4();

	DirectX::BoundingFrustum m_BoundingFrustum;

	float m_JitterX = 0.0f;
	float m_JitterY = 0.0f;
	float m_PreJitterX = 0.0f;
	float m_PreJitterY = 0.0f;
	XMMATRIX m_JitterMat = XMMatrixIdentity();
	XMMATRIX m_PreJitterMat = XMMatrixIdentity();

private:
	std::wstring m_CameraName = L"";
	bool m_DirtyHint = true;
	UINT m_LastUpdateFrameID = 0;
};