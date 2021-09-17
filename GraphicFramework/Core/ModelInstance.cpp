#include "ModelInstance.h"
#include "Utility.h"
#include "Scene.h"
#include "GraphicsCore.h"


CModelInstance::CModelInstance() : CModelInstance(L"", nullptr, nullptr) 
{
}

CModelInstance::CModelInstance(const std::wstring& instanceName, CModel* pHostModel, CScene* pHostScene)
	: m_Name(instanceName), m_pModel(pHostModel), m_pScene(pHostScene)
{
	if(m_pModel) m_BoundingObject = m_pModel->GetBoundingObject();
}

CModelInstance::~CModelInstance()
{
}

void CModelInstance::SetInstance(const XMFLOAT3& translation, const XMFLOAT3& yawPitchRoll, const XMFLOAT3& scale)
{
	m_CurTransform.Translation = translation;
	SetRotation(yawPitchRoll);
	m_CurTransform.Scale = scale;
	m_CurTransform.MatrixDirty = true;
}

void CModelInstance::SetTranslation(const XMFLOAT3& translation)
{
	m_CurTransform.Translation = translation;
	m_CurTransform.MatrixDirty = true;
	m_CurTransform.UpdateFrameID = (UINT)GraphicsCore::GetFrameID();
	m_CurDirtyUnProcessHint = true;
	_ASSERTE(m_pScene);
	m_pScene->OnInstanceUpdate(m_CurTransform.UpdateFrameID, m_Index);
}

void CModelInstance::SetScaling(const XMFLOAT3& scaling)
{
	m_CurTransform.Scale = scaling; 
	m_CurTransform.MatrixDirty = true; 
	m_CurTransform.UpdateFrameID = (UINT)GraphicsCore::GetFrameID();
	m_CurDirtyUnProcessHint = true;
	_ASSERTE(m_pScene);
	m_pScene->OnInstanceUpdate(m_CurTransform.UpdateFrameID, m_Index);
}

void CModelInstance::SetRotation(const XMFLOAT3& yawPitchRoll)
{
	m_CurTransform.Rotation = yawPitchRoll;
	m_CurTransform.MatrixDirty = true;
	m_CurTransform.UpdateFrameID = (UINT)GraphicsCore::GetFrameID();
	m_CurDirtyUnProcessHint = true;
	_ASSERTE(m_pScene);
	m_pScene->OnInstanceUpdate(m_CurTransform.UpdateFrameID, m_Index);
}

void CModelInstance::SetTransformMatrix(const XMMATRIX& mat)
{
	m_CurTransform.UpdateFrameID = GraphicsCore::GetFrameID();
	if (m_LastUpdateFrameID != m_CurTransform.UpdateFrameID)
	{
		m_PrevFinalTransformMatrix = m_CurTransform.Matrix;
		m_PrevBoundingObject = m_BoundingObject;
	}
	m_CurTransform.Matrix = mat;
	m_CurTransform.MatrixDirty = false;
	m_FinalTransformMatrix = m_CurTransform.Matrix;
	_ASSERTE(m_pModel);
	m_pModel->GetBoundingObject().Transform(m_BoundingObject, m_FinalTransformMatrix);
	m_LastUpdateFrameID = m_CurTransform.UpdateFrameID;

	m_CurDirtyUnProcessHint = true;
	_ASSERTE(m_pScene);
	m_pScene->OnInstanceUpdate(m_CurTransform.UpdateFrameID, m_Index);
}

bool CModelInstance::IsUpdatedInCurrentFrame() const
{
	return (m_LastUpdateFrameID == GraphicsCore::GetFrameID());
}

void CModelInstance::__UpdateInstanceCurProperties() const
{
	if (m_CurTransform.MatrixDirty)
	{
		if (m_LastUpdateFrameID != m_CurTransform.UpdateFrameID)
		{
			m_PrevFinalTransformMatrix = m_CurTransform.Matrix;
			m_PrevBoundingObject = m_BoundingObject;
		}
		m_CurTransform.Matrix = __CalculateTransformMatrix(m_CurTransform.Translation, m_CurTransform.Rotation, m_CurTransform.Scale);
		m_CurTransform.MatrixDirty = false;
		m_FinalTransformMatrix = m_CurTransform.Matrix;
		_ASSERTE(m_pModel);
		m_pModel->GetBoundingObject().Transform(m_BoundingObject, m_FinalTransformMatrix);
		m_LastUpdateFrameID = m_CurTransform.UpdateFrameID;
	}
}

XMMATRIX CModelInstance::__CreateMatrixFromBasis(const XMFLOAT3& forward, const XMFLOAT3& up)
{	
	XMVECTOR f = XMVector3Normalize(XMLoadFloat3(&forward));
	XMVECTOR u = XMVector3Normalize(XMLoadFloat3(&up));
	_ASSERTE(!MathUtility::IsEqual(u, f) && !MathUtility::IsEqual(u, -f));// These two vectors shouldn't be collinear.	
	XMVECTOR s = XMVector3Normalize(XMVector3Cross(u, f));
	u = XMVector3Cross(f, s);
	return XMMATRIX(s, u, f, XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f));
}

XMMATRIX CModelInstance::__CalculateTransformMatrix(const XMFLOAT3& translation, const XMFLOAT3& yawPitchRoll, const XMFLOAT3& scale)
{
	XMMATRIX translationMtx = XMMatrixTranslation(translation.x, translation.y, translation.z);
	XMMATRIX rotationMtx = XMMatrixRotationRollPitchYaw(yawPitchRoll.x, yawPitchRoll.y, yawPitchRoll.z);
	XMMATRIX scalingMtx = XMMatrixScaling(scale.x, scale.y, scale.z);
	return scalingMtx * rotationMtx * translationMtx;
}