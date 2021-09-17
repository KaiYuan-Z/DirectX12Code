#pragma once
#include "DirectXMath.h"
#include "DirectXCollision.h"

using namespace DirectX;

class CBoundingFrustum
{
public:
	CBoundingFrustum();
	~CBoundingFrustum();

	void SetFrustum(const BoundingFrustum& frustum);
	void SetFrustum(const BoundingFrustum& frustum, const XMMATRIX& transform);

	void Transform(const XMMATRIX& transform);

	void Contains(const XMFLOAT3& centerA, float radiusA, const XMFLOAT3& centerB, float radiusB, bool& resA, bool& resB) const;

	const BoundingFrustum& GetFrustum() const { return m_Frustum; }

	const XMVECTOR& GetNear() const { return m_NearPlane; }
	const XMVECTOR& GetFar() const { return m_FarPlane; }
	const XMVECTOR& GetRight() const { return m_RightPlane; }
	const XMVECTOR& GetLeft() const { return m_LeftPlane; }
	const XMVECTOR& GetTop() const { return m_TopPlane; }
	const XMVECTOR& GetBottom() const { return m_BottomPlane; }

private:
	BoundingFrustum m_Frustum;

	XMMATRIX m_PlaneCache0;
	XMMATRIX m_PlaneCache1;

	XMVECTOR m_NearPlane;
	XMVECTOR m_FarPlane;
	XMVECTOR m_RightPlane;
	XMVECTOR m_LeftPlane;
	XMVECTOR m_TopPlane;
	XMVECTOR m_BottomPlane;

	void  __UpdatePlanes();
};

