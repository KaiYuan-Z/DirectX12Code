#include "CameraWalkthrough.h"

using namespace DirectX;

CCameraWalkthrough::CCameraWalkthrough() : CCameraWalkthrough(L"")
{
}

CCameraWalkthrough::CCameraWalkthrough(const std::wstring& name)
	: CCameraBasic(name), m_MoveStep(1.0f), m_RotateStep(0.05f), m_LastMousePos(0.0f, 0.0f)
{
	SetLens(0.25f*XM_PI, 1.0f, 1.0f, 1000.0f);
	m_CameraType = CAMERA_TYPE_WALKTHROUGH;
}

CCameraWalkthrough::~CCameraWalkthrough()
{
}

void CCameraWalkthrough::SetLens(float fovY, float aspect, float zn, float zf)
{
	// cache properties
	m_FovY = fovY;
	m_Aspect = aspect;
	m_NearZ = zn;
	m_FarZ = zf;

	m_NearWindowHeight = 2.0f * m_NearZ * tanf(0.5f*m_FovY);
	m_FarWindowHeight = 2.0f * m_FarZ * tanf(0.5f*m_FovY);

	XMMATRIX P = XMMatrixPerspectiveFovLH(m_FovY, m_Aspect, m_NearZ, m_FarZ);
	XMStoreFloat4x4(&m_Proj, P);

	_ComputeBoundingFrustum();

	_SetDirtyHint(true);
}

void CCameraWalkthrough::SetLookAt(const XMFLOAT3& camPos, const XMFLOAT3& camTarget, const XMFLOAT3& camUp)
{
	XMVECTOR P = XMLoadFloat3(&camPos);
	XMVECTOR T = XMLoadFloat3(&camTarget);
	XMVECTOR U = XMLoadFloat3(&camUp);

	SetLookAt(P, T, U);
}

void CCameraWalkthrough::SetLookAt(const XMVECTOR& camPos, const XMVECTOR& camTarget, const XMVECTOR& camUp)
{
	XMVECTOR L = XMVector3Normalize(XMVectorSubtract(camTarget, camPos));
	_ASSERTE(!MathUtility::IsEqual(camUp, L) && !MathUtility::IsEqual(camUp, -L));// These two vectors shouldn't be collinear.
	XMVECTOR R = XMVector3Normalize(XMVector3Cross(camUp, L));
	_ASSERTE(!MathUtility::IsEqual(L, R) && !MathUtility::IsEqual(L, -R));// These two vectors shouldn't be collinear.
	XMVECTOR U = XMVector3Cross(L, R);

	XMStoreFloat3(&m_Position, camPos);
	XMStoreFloat3(&m_Look, L);
	XMStoreFloat3(&m_Right, R);
	XMStoreFloat3(&m_Up, U);

	_SetDirtyHint(true);
}

void CCameraWalkthrough::OnKeyDown(UINT8 key)
{
	CCameraBasic::OnKeyDown(key);
	
	switch (key)
	{
	case 's':
	case 'S':__Walk(-m_MoveStep); break;

	case 'w':
	case 'W':__Walk(m_MoveStep); break;

	case 'c':
	case 'C':__Up(-m_MoveStep); break;

	case VK_SPACE:__Up(m_MoveStep); break;

	case 'a':
	case 'A':__Strafe(-m_MoveStep); break;

	case 'd':
	case 'D':__Strafe(m_MoveStep); break;

	default:break;
	}
}

void CCameraWalkthrough::OnMouseButtonDown(UINT x, UINT y)
{
	m_LastMousePos.x = (float)x;
	m_LastMousePos.y = (float)y;
}

void CCameraWalkthrough::OnMouseMove(UINT x, UINT y)
{
	float dx = XMConvertToRadians(m_RotateStep*(float(x) - m_LastMousePos.x));
	float dy = XMConvertToRadians(m_RotateStep*(float(y) - m_LastMousePos.y));

	__Pitch(dy);
	__RotateY(dx);
}

void CCameraWalkthrough::__Strafe(float distance)
{
	// mPosition += d*mRight
	XMVECTOR s = XMVectorReplicate(distance);
	XMVECTOR r = XMLoadFloat3(&m_Right);
	XMVECTOR p = XMLoadFloat3(&m_Position);
	XMStoreFloat3(&m_Position, XMVectorMultiplyAdd(s, r, p));

	_SetDirtyHint(true);
}

void CCameraWalkthrough::__Walk(float distance)
{
	// mPosition += d*mLook
	XMVECTOR s = XMVectorReplicate(distance);
	XMVECTOR l = XMLoadFloat3(&m_Look);
	XMVECTOR p = XMLoadFloat3(&m_Position);
	XMStoreFloat3(&m_Position, XMVectorMultiplyAdd(s, l, p));

	_SetDirtyHint(true);
}

void CCameraWalkthrough::__Up(float distance)
{
	XMFLOAT3 GlobalUp = { 0.0f, 1.0f, 0.0f };
	XMVECTOR s = XMVectorReplicate(distance);
	XMVECTOR d = XMLoadFloat3(&GlobalUp);
	XMVECTOR p = XMLoadFloat3(&m_Position);
	XMStoreFloat3(&m_Position, XMVectorMultiplyAdd(s, d, p));

	_SetDirtyHint(true);
}

void CCameraWalkthrough::__Pitch(float angle)
{
	// Rotate up and look vector about the right vector.
	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&m_Right), angle);
	XMStoreFloat3(&m_Up, XMVector3TransformNormal(XMLoadFloat3(&m_Up), R));
	XMStoreFloat3(&m_Look, XMVector3TransformNormal(XMLoadFloat3(&m_Look), R));

	_SetDirtyHint(true);
}

void CCameraWalkthrough::__RotateY(float angle)
{
	// Rotate the basis vectors about the world y-axis.
	XMMATRIX R = XMMatrixRotationY(angle);
	XMStoreFloat3(&m_Right, XMVector3TransformNormal(XMLoadFloat3(&m_Right), R));
	XMStoreFloat3(&m_Up, XMVector3TransformNormal(XMLoadFloat3(&m_Up), R));
	XMStoreFloat3(&m_Look, XMVector3TransformNormal(XMLoadFloat3(&m_Look), R));

	_SetDirtyHint(true);
}