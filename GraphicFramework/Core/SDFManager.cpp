#include "pch.h"
#include "SDFManager.h"

using namespace DirectX;

void CSDFManager::addFields(size_t amount, const std::vector<SDimensions>& dimension, const std::vector<float>& resolution, std::vector<CColorBuffer*>& data)
{
	size_t totalSize = 0;
	XMFLOAT3 zero;
	XMMATRIX zeroMat;
	for (size_t i = 0; i < amount; ++i)
	{
		SDimensions currentDimension = dimension[i];
		m_Offsets.push_back(totalSize);
		totalSize += currentDimension.length();
		m_Dimensions.push_back(currentDimension);
		m_GridResolutions.push_back(resolution[i]);
		m_Positions.push_back(zero);
		m_Rotations.push_back(zero);
		m_Scales.push_back(1);
		m_CachedRotations.push_back(zeroMat);
		m_CachedTransforms.push_back(zeroMat);
		m_CachedInverseTransforms.push_back(zeroMat);
		m_FieldBounds.push_back(SBounds());
		m_OldParams.push_back(SWorldFieldUpdateParams());
		m_NeedsUpdate.push_back(-1);
		m_Data.push_back(data[i]);
	}
}

void CSDFManager::RecalculateTransforms(XMFLOAT4 time)
{
	XMMATRIX rotate, scale, center;
	XMVECTOR forward, up, right;
	for (size_t i = 0; i < length(); ++i)
	{
		SDimensions dim = getDimensions(i);

		XMFLOAT3 pos = m_Positions[i];

		float scaledRes = getResolution(i)*getScale(i);

		XMVECTOR dimension = XMVectorSet((float)dim.width, (float)dim.height, (float)dim.depth, 0);
		scale = XMMatrixScalingFromVector(dimension * scaledRes);
		rotate = XMMatrixRotationRollPitchYawFromVector(XMLoadFloat3(&m_Rotations[i]));
		m_CachedRotations[i] = XMMatrixInverse(&XMMatrixDeterminant(rotate), rotate);
		center = XMMatrixTranslationFromVector(XMLoadFloat3(&pos));
		m_CachedTransforms[i] = center * rotate * scale;
		m_CachedInverseTransforms[i] = XMMatrixInverse(&XMMatrixDeterminant(center*rotate), center*rotate);

		//cachedTransforms[i][3] = time;
		//if (i == 4)
		//	cachedInverseTransforms[i][0][3] = time.y;

		float radius = __GetBoundingSphereRadius((float)dim.width, (float)dim.height, (float)dim.depth);
		m_FieldBounds[i].sphere = XMFLOAT4(pos.x, pos.y, pos.z, radius*scaledRes);
		right = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
		up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		forward = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

		XMStoreFloat4(&m_FieldBounds[i].forward, XMVector4Transform(forward, rotate));
		m_FieldBounds[i].forward.w = 0.5f*scaledRes*(float)dim.depth;

		XMStoreFloat4(&m_FieldBounds[i].up, XMVector4Transform(up, rotate));
		m_FieldBounds[i].up.w = 0.5f*scaledRes*(float)dim.height;

		XMStoreFloat4(&m_FieldBounds[i].right, XMVector4Transform(right, rotate));
		m_FieldBounds[i].right.w = 0.5f*scaledRes*(float)dim.width;
	}
}

bool CSDFManager::ObjectNeedsUpdate(size_t index, SWorldFieldUpdateParams& params, XMINT3& dispatchSize, XMFLOAT3& fieldOrigin, XMFLOAT3& fieldExtents)
{
	int counter = m_NeedsUpdate[index];
	if (counter == -1)
		return false;

	if (counter == UPDATE_FREQUENCY)
	{
		SWorldFieldUpdateParams oldParam;
		CalculateWorldFieldUpdateParams(index, fieldOrigin, fieldExtents, dispatchSize, oldParam);
		m_OldParams[index] = oldParam;
	}

	m_NeedsUpdate[index]--;

	if (counter == 1/* && m_OldParams[index].size != XMINT3(0,0,0)*/)
	{
		params = m_OldParams[index];
		return true;
	}

	if (counter != 0)
		return false;


	CalculateWorldFieldUpdateParams(index, fieldOrigin, fieldExtents, dispatchSize, params);

	if (m_OldParams[index] == params)
		return false;

	m_OldParams[index] = SWorldFieldUpdateParams();

	return true;
}

void CSDFManager::CalculateWorldFieldUpdateParams(size_t index, XMFLOAT3& fieldOrigin, XMFLOAT3& fieldExtents, XMINT3& dispatchSize, SWorldFieldUpdateParams& params)
{
	SBounds bound = getFieldBounds(index);

	XMVECTOR fwd = XMVectorSet(bound.forward.x, bound.forward.y, bound.forward.z, 0)*bound.forward.w;
	XMVECTOR rght = XMVectorSet(bound.right.x, bound.right.y, bound.right.z, 0)*bound.right.w;
	XMVECTOR up = XMVectorSet(bound.up.x, bound.up.y, bound.up.z, 0)*bound.up.w;
	XMVECTOR pos = XMLoadFloat3(&getPosition(index));
	XMFLOAT3 c1,c2,c3,c4,c5,c6,c7,c8;
	XMStoreFloat3(&c1, pos + fwd + up + rght);
	XMStoreFloat3(&c2, pos + fwd + up - rght);
	XMStoreFloat3(&c3, pos + fwd - up + rght);
	XMStoreFloat3(&c4, pos + fwd - up - rght);
	XMStoreFloat3(&c5, pos - fwd + up + rght);
	XMStoreFloat3(&c6, pos - fwd + up - rght);
	XMStoreFloat3(&c7, pos - fwd - up + rght);
	XMStoreFloat3(&c8, pos - fwd - up - rght);

	XMVECTOR aabbMin =
		XMVectorSet(
			XMMin(XMMin(XMMin(c1.x, c2.x), XMMin(c3.x, c4.x)), XMMin(XMMin(c5.x, c6.x), XMMin(c7.x, c8.x))),
			XMMin(XMMin(XMMin(c1.y, c2.y), XMMin(c3.y, c4.y)), XMMin(XMMin(c5.y, c6.y), XMMin(c7.y, c8.y))),
			XMMin(XMMin(XMMin(c1.z, c2.z), XMMin(c3.z, c4.z)), XMMin(XMMin(c5.z, c6.z), XMMin(c7.z, c8.z))), 0);
	XMVECTOR aabbMax =
		XMVectorSet(
			XMMax(XMMax(XMMax(c1.x, c2.x), XMMax(c3.x, c4.x)), XMMax(XMMax(c5.x, c6.x), XMMax(c7.x, c8.x))),
			XMMax(XMMax(XMMax(c1.y, c2.y), XMMax(c3.y, c4.y)), XMMax(XMMax(c5.y, c6.y), XMMax(c7.y, c8.y))),
			XMMax(XMMax(XMMax(c1.z, c2.z), XMMax(c3.z, c4.z)), XMMax(XMMax(c5.z, c6.z), XMMax(c7.z, c8.z))), 0);

	XMVECTOR dim = XMVectorAbs(aabbMax - aabbMin);


	XMVECTOR worldBoxOrigin = XMLoadFloat3(&fieldOrigin) - XMLoadFloat3(&fieldExtents) * 0.5f;
	XMVECTOR boxSize = XMLoadFloat3(&fieldExtents) / (XMLoadSInt3(&dispatchSize));
	XMVECTOR adjustedPos = aabbMin - worldBoxOrigin;
	XMStoreSInt3(&params.offset, XMVectorMax(XMVectorZero(), XMVECTOR(adjustedPos / boxSize) - XMVectorSet(1, 1, 1, 0)));

	XMVECTOR odispatchSize = XMVECTOR(dim / boxSize + XMVectorSet(2, 2, 2, 0));
	XMStoreSInt3(&params.size, XMVECTOR(dim / boxSize + XMVectorSet(2, 2, 2, 0)));
}

float CSDFManager::__GetBoundingSphereRadius(float width, float height, float depth)
{
	float diag = std::sqrt(width*width + height * height);
	float fulldiag = std::sqrt(diag*diag + depth * depth);
	return fulldiag * 0.5f;
}
