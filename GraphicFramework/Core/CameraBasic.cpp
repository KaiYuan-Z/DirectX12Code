#include "CameraBasic.h"
#include "CameraWalkthrough.h"
#include "GraphicsCore.h"
#include <fstream>

using namespace DirectX;

CCameraBasic::CCameraBasic(void)
{
}

CCameraBasic::CCameraBasic(const std::wstring& name) : m_CameraName(name)
{
}

CCameraBasic::~CCameraBasic(void)
{
}

CCameraBasic* CCameraBasic::CreateCamera(const std::wstring& cameraName, UINT cameraType)
{
	CCameraBasic* pCamera = nullptr;
	switch (cameraType)
	{
	case CAMERA_TYPE_BASIC: pCamera = new CCameraBasic(cameraName); break;
	case CAMERA_TYPE_WALKTHROUGH: pCamera = new CCameraWalkthrough(cameraName); break;
	default:_ASSERT(false);
	}
	_ASSERTE(pCamera);
	return pCamera;
}

void CCameraBasic::SetCamera(const XMFLOAT3& camPos, const XMFLOAT3& camRight, const XMFLOAT3& camUp, const XMFLOAT3& camLook)
{
	m_Position = camPos;
	m_Right = camRight;
	m_Up = camUp;
	m_Look = camLook;

	m_DirtyHint = true;

	OnFrameUpdate();
}

void CCameraBasic::SetJitter(float jitterX, float jitterY)
{
	_ASSERTE(jitterX >= -0.5f && jitterX <= 0.5f && jitterY >= -0.5f && jitterY <= 0.5f);
	m_JitterX = jitterX;
	m_JitterY = jitterY;
	m_JitterMat = { 
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		2.0f * jitterX, 2.0f * jitterY, 0.0f, 1.0f };
}

XMFLOAT4X4 CCameraBasic::GetProjectXMF4X4(bool withJitter) const
{
	XMFLOAT4X4 P_4X4; 
	XMStoreFloat4x4(&P_4X4, withJitter ? XMLoadFloat4x4(&m_Proj)*m_JitterMat : XMLoadFloat4x4(&m_Proj));
	return P_4X4;
}

XMMATRIX CCameraBasic::GetProjectXMM(bool withJitter) const
{
	return withJitter ? XMLoadFloat4x4(&m_Proj)*m_JitterMat : XMLoadFloat4x4(&m_Proj);
}

XMMATRIX CCameraBasic::GetPreProjectXMM(bool withJitter) const
{
	return withJitter ? XMLoadFloat4x4(&m_Proj) * m_PreJitterMat : XMLoadFloat4x4(&m_Proj);
}
void CCameraBasic::OnFrameUpdate()
{
	if (m_DirtyHint)
	{
		m_LastUpdateFrameID = (UINT)GraphicsCore::GetFrameID();

		XMVECTOR R = XMLoadFloat3(&m_Right);
		XMVECTOR U = XMLoadFloat3(&m_Up);
		XMVECTOR L = XMLoadFloat3(&m_Look);
		XMVECTOR P = XMLoadFloat3(&m_Position);

		// Keep camera's axes orthogonal to each other and of unit length.
		L = XMVector3Normalize(L);
		U = XMVector3Normalize(XMVector3Cross(L, R));

		// U, L already ortho-normal, so no need to normalize cross product.
		R = XMVector3Cross(U, L);

		// Fill in the view matrix entries.
		float x = -XMVectorGetX(XMVector3Dot(P, R));
		float y = -XMVectorGetX(XMVector3Dot(P, U));
		float z = -XMVectorGetX(XMVector3Dot(P, L));

		XMStoreFloat3(&m_Right, R);
		XMStoreFloat3(&m_Up, U);
		XMStoreFloat3(&m_Look, L);

		m_View(0, 0) = m_Right.x;
		m_View(1, 0) = m_Right.y;
		m_View(2, 0) = m_Right.z;
		m_View(3, 0) = x;

		m_View(0, 1) = m_Up.x;
		m_View(1, 1) = m_Up.y;
		m_View(2, 1) = m_Up.z;
		m_View(3, 1) = y;

		m_View(0, 2) = m_Look.x;
		m_View(1, 2) = m_Look.y;
		m_View(2, 2) = m_Look.z;
		m_View(3, 2) = z;

		m_View(0, 3) = 0.0f;
		m_View(1, 3) = 0.0f;
		m_View(2, 3) = 0.0f;
		m_View(3, 3) = 1.0f;

		m_VirtualTarget = XMFLOAT3(m_Position.x + 10.0f * m_Look.x, m_Position.y + 10.0f * m_Look.y, m_Position.z + 10.0f * m_Look.z);

		_SetDirtyHint(false);
	}
}

void CCameraBasic::OnFramePostProcess()
{
	m_PreView = m_View;
	m_PrePosition = m_Position;
	m_PreJitterX = m_JitterX;
	m_PreJitterY = m_JitterY;
	m_PreJitterMat = m_JitterMat;
}

bool CCameraBasic::IsUpdatedInCurrentFrame() const
{
	return (m_LastUpdateFrameID == GraphicsCore::GetFrameID());
}

void CCameraBasic::OnKeyDown(UINT8 key)
{
	if (key == 'p' || key == 'P')
	{
		std::fstream f("Camera_Paramters.txt", std::ios::out);
		f << "Pos: [" << m_Position.x << ", " << m_Position.y << ", " << m_Position.z << "]\r\n";
		f << "Up: [" << m_Up.x << ", " << m_Up.y << ", " << m_Up.z << "]\r\n";
		f << "Virtual Target: [" << m_VirtualTarget.x << ", " << m_VirtualTarget.y << ", " << m_VirtualTarget.z << "]\r\n";
		f.close();
	}
}

CCameraWalkthrough* CCameraBasic::QueryWalkthroughCamera()
{
	_ASSERTE(m_CameraType == CAMERA_TYPE_WALKTHROUGH);
	return static_cast<CCameraWalkthrough*>(this);
}
