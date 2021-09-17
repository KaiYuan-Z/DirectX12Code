#pragma once
#include "MathUtility.h"
#include "BoundingObject.h"
#include "Model.h"


class CScene;

class CModelInstance
{
public:
	CModelInstance();
	CModelInstance(const std::wstring& instanceName, CModel* pHostModel, CScene* pHostScene);
	~CModelInstance();

	void SetInstance(const XMFLOAT3& translation, const XMFLOAT3& yawPitchRoll, const XMFLOAT3& scale);

    CModel* GetModel() const { return m_pModel; };

	CScene* GetScene() const { return m_pScene; }

    void SetVisible(bool visible) { m_Visible = visible; };
    bool IsVisible() const { return m_Visible; };

	void SetDynamic(bool dynamic) { m_Dynamic = dynamic; }
	bool IsDynamic() const { return m_Dynamic; }

	void SetIndex(UINT index) { m_Index = index; }
	UINT GetIndex() const { return m_Index; }

	void SetName(const std::wstring& name) { m_Name = name; }
	const std::wstring& GetName() const { return m_Name; }

	void SetTranslation(const XMFLOAT3& translation);
    const XMFLOAT3& GetTranslation() const { return m_CurTransform.Translation; };

	void SetScaling(const XMFLOAT3& scaling);
    const XMFLOAT3& GetScaling() const { return m_CurTransform.Scale; }

	void SetRotation(const XMFLOAT3& yawPitchRoll);
	const XMFLOAT3& GetRotation() const { return m_CurTransform.Rotation; }

	void SetTransformMatrix(const XMMATRIX& mat);

	const XMMATRIX& GetTransformMatrix() const { __UpdateInstanceCurProperties(); return m_FinalTransformMatrix; }
	const XMMATRIX& GetPrevTransformMatrix() const { __UpdateInstanceCurProperties(); return m_PrevFinalTransformMatrix; }
	const SBoundingObject& GetBoundingObject() const { __UpdateInstanceCurProperties(); return m_BoundingObject; }
	const SBoundingObject& GetPrevBoundingObject() const { __UpdateInstanceCurProperties(); return m_PrevBoundingObject; }

	bool GetAndResetCurDirtyUnProcessHint() const { bool hint = m_CurDirtyUnProcessHint; m_CurDirtyUnProcessHint = false;  return hint; }

	bool IsUpdatedInCurrentFrame() const;
 
private:

	struct STransform
	{
		XMFLOAT3 Translation = { 0.0f, 0.0f, 0.0f };
		XMFLOAT3 Rotation = { 0.0f, 0.0f, 1.0f };
		XMFLOAT3 Scale = { 1.0f, 1.0f, 1.0f };

		// Matrix containing the above transforms
		XMMATRIX Matrix = XMMatrixIdentity();

		bool MatrixDirty = true;
		UINT UpdateFrameID = 0xffffffff;
	};

	void __UpdateInstanceCurProperties() const;

	static XMMATRIX __CreateMatrixFromBasis(const XMFLOAT3& forward, const XMFLOAT3& up);
	static XMMATRIX __CalculateTransformMatrix(const XMFLOAT3& translation, const XMFLOAT3& yawPitchRoll, const XMFLOAT3& scale);

    std::wstring m_Name = L"";

	CModel* m_pModel = nullptr;
	CScene* m_pScene = nullptr;

	UINT m_Index = 0xffffffff;

	mutable UINT m_LastUpdateFrameID = 0;

	mutable bool m_Visible = true;
	mutable bool m_Dynamic = false;
	mutable bool m_CurDirtyUnProcessHint = false;

    mutable STransform m_CurTransform;
    
    mutable XMMATRIX m_FinalTransformMatrix = XMMatrixIdentity();
    mutable XMMATRIX m_PrevFinalTransformMatrix = XMMatrixIdentity();
    mutable SBoundingObject m_BoundingObject;
	mutable SBoundingObject m_PrevBoundingObject;
};
