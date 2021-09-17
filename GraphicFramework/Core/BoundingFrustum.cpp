#include "BoundingFrustum.h"



CBoundingFrustum::CBoundingFrustum()
{
}


CBoundingFrustum::~CBoundingFrustum()
{
}

void CBoundingFrustum::SetFrustum(const BoundingFrustum& frustum)
{
	m_Frustum = frustum;
	__UpdatePlanes();
}

void CBoundingFrustum::SetFrustum(const BoundingFrustum& frustum, const XMMATRIX& transform)
{
	frustum.Transform(m_Frustum, transform);
	__UpdatePlanes();
}

void CBoundingFrustum::Transform(const XMMATRIX& transform)
{
	m_Frustum.Transform(m_Frustum, transform);
	__UpdatePlanes();
}

void CBoundingFrustum::Contains(const XMFLOAT3& centerA, float radiusA, const XMFLOAT3& centerB, float radiusB, bool& resA, bool& resB) const
{
	XMVECTOR shA_xxxx = XMVectorSet(centerA.x, centerA.x, centerA.x, centerA.x);
	XMVECTOR shA_yyyy = XMVectorSet(centerA.y, centerA.y, centerA.y, centerA.y);
	XMVECTOR shA_zzzz = XMVectorSet(centerA.z, centerA.z, centerA.z, centerA.z);
	XMVECTOR shA_rrrr = XMVectorSet(radiusA, radiusA, radiusA, radiusA);

	XMVECTOR dotA_0123 = XMVectorMultiplyAdd(shA_zzzz, m_PlaneCache0.r[2], m_PlaneCache0.r[3]);
	dotA_0123 = XMVectorMultiplyAdd(shA_yyyy, m_PlaneCache0.r[1], dotA_0123);
	dotA_0123 = XMVectorMultiplyAdd(shA_xxxx, m_PlaneCache0.r[0], dotA_0123);

	XMVECTOR shB_xxxx = XMVectorSet(centerB.x, centerB.x, centerB.x, centerB.x);
	XMVECTOR shB_yyyy = XMVectorSet(centerB.y, centerB.y, centerB.y, centerB.y);
	XMVECTOR shB_zzzz = XMVectorSet(centerB.z, centerB.z, centerB.z, centerB.z);
	XMVECTOR shB_rrrr = XMVectorSet(radiusB, radiusB, radiusB, radiusB);

	XMVECTOR dotB_0123 = XMVectorMultiplyAdd(shB_zzzz, m_PlaneCache0.r[2], m_PlaneCache0.r[3]);
	dotB_0123 = XMVectorMultiplyAdd(shB_yyyy, m_PlaneCache0.r[1], dotB_0123);
	dotB_0123 = XMVectorMultiplyAdd(shB_xxxx, m_PlaneCache0.r[0], dotB_0123);

	XMVECTOR shAB_xxxx = XMVectorInsert<0, 0, 0, 1, 1>(shA_xxxx, shB_xxxx);
	XMVECTOR shAB_yyyy = XMVectorInsert<0, 0, 0, 1, 1>(shA_yyyy, shB_yyyy);
	XMVECTOR shAB_zzzz = XMVectorInsert<0, 0, 0, 1, 1>(shA_zzzz, shB_zzzz);
	XMVECTOR shAB_rrrr = XMVectorInsert<0, 0, 0, 1, 1>(shA_rrrr, shB_rrrr);

	XMVECTOR dotA45B45 = XMVectorMultiplyAdd(shAB_zzzz, m_PlaneCache1.r[2], m_PlaneCache1.r[3]);
	dotA45B45 = XMVectorMultiplyAdd(shAB_yyyy, m_PlaneCache1.r[1], dotA45B45);
	dotA45B45 = XMVectorMultiplyAdd(shAB_xxxx, m_PlaneCache1.r[0], dotA45B45);

	dotA_0123 = XMVectorGreater(dotA_0123, shA_rrrr);
	dotB_0123 = XMVectorGreater(dotB_0123, shB_rrrr);
	dotA45B45 = XMVectorGreater(dotA45B45, shAB_rrrr);

	XMVECTOR dotA45 = XMVectorInsert<0, 0, 0, 1, 1>(dotA45B45, XMVectorZero());
	XMVECTOR dotB45 = XMVectorInsert<0, 0, 0, 1, 1>(XMVectorZero(), dotA45B45);

	bool resA_0123 = XMVector4Equal(dotA_0123, XMVectorZero());
	bool resB_0123 = XMVector4Equal(dotB_0123, XMVectorZero());
	bool resA_45 = XMVector4Equal(dotA45, XMVectorZero());
	bool resB_45 = XMVector4Equal(dotB45, XMVectorZero());
	
	resA = (resA_0123 & resA_45);
	resB = (resB_0123 & resB_45);
}

void CBoundingFrustum::__UpdatePlanes()
{
	m_Frustum.GetPlanes(&m_NearPlane, &m_FarPlane, &m_RightPlane, &m_LeftPlane, &m_TopPlane, &m_BottomPlane);
	m_PlaneCache0 = XMMatrixTranspose(XMMATRIX(m_NearPlane, m_FarPlane, m_RightPlane, m_LeftPlane));
	m_PlaneCache1 = XMMatrixTranspose(XMMATRIX(m_TopPlane, m_BottomPlane, m_TopPlane, m_BottomPlane));
}
