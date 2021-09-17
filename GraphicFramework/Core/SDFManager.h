#pragma once
#include <vector>
#include "MathUtility.h"
#include "ColorBuffer.h"

struct SBounds
{
	SBounds() : sphere(), forward(), up(), right()
	{}
	DirectX::XMFLOAT4 sphere;
	DirectX::XMFLOAT4 forward;
	DirectX::XMFLOAT4 up;
	DirectX::XMFLOAT4 right;
};

struct SWorldFieldUpdateParams
{
	SWorldFieldUpdateParams() : offset(), size()
	{}
	DirectX::XMINT3 offset;
	DirectX::XMINT3 size;

	bool operator== (SWorldFieldUpdateParams& other)
	{
		return other.offset.x == this->offset.x && other.offset.y == this->offset.y && other.offset.z == this->offset.z &&
			   other.size.x == this->size.x && other.size.y == this->size.y && other.size.z == this->size.z;
	}
};

class CSDFManager
{
public:
	CSDFManager() : m_Dimensions(), m_GridResolutions(), m_Offsets(), m_Positions(), m_Rotations(), m_Data(),
		m_Scales(), m_CachedInverseTransforms(), m_CachedTransforms(), m_CachedRotations(), m_FieldBounds()
	{
	}

	void addFields(size_t amount, const std::vector<SDimensions>& dimension, const std::vector<float>& resolution, std::vector<CColorBuffer*>& data);

	~CSDFManager()
	{
		clearFieldData();
	}

	void clearFieldData()
	{
	}

	const size_t length() {	return m_Dimensions.size(); }
	const DirectX::XMMATRIX getInverseTransform(size_t index) { return m_CachedInverseTransforms[index]; }
	const DirectX::XMMATRIX getTransform(size_t index) { return m_CachedTransforms[index]; }
	const DirectX::XMMATRIX getRotationTransform(size_t index) { return m_CachedRotations[index]; }
	const DirectX::XMFLOAT3 getPosition(size_t index) { return m_Positions[index]; }
	const DirectX::XMFLOAT3 getRotation(size_t index) { return m_Rotations[index]; }
	const float getScale(size_t index) { return m_Scales[index]; }
	const float getResolution(size_t index) { return m_GridResolutions[index]; }
	const CColorBuffer* getData(size_t index) { return m_Data[index]; }
	const SDimensions getDimensions(size_t index) { return m_Dimensions[index]; }
	const size_t getGridLength(size_t index) { return getDimensions(index).length(); }
	const SBounds getFieldBounds(size_t index) { return m_FieldBounds[index]; }

	void setPosition(size_t index, DirectX::XMFLOAT3 pos)
	{
		m_Positions[index] = pos;
		MarkAsDirty(index);
	}


	void setRotation(size_t index, DirectX::XMFLOAT3 rot)
	{
		m_Rotations[index] = rot;
		MarkAsDirty(index);
	}

	void setScale(size_t index, float sca)
	{
		m_Scales[index] = sca;
		MarkAsDirty(index);
	}

	void MarkAsDirty(size_t index)
	{
		if (m_NeedsUpdate[index] == -1)
			m_NeedsUpdate[index] = UPDATE_FREQUENCY;
	}

	void RecalculateTransforms(DirectX::XMFLOAT4 time);

	bool ObjectNeedsUpdate(size_t index, SWorldFieldUpdateParams& params, DirectX::XMINT3& dispatchSize, DirectX::XMFLOAT3& fieldOrigin, DirectX::XMFLOAT3& fieldExtents);

	void CalculateWorldFieldUpdateParams(size_t index, DirectX::XMFLOAT3& fieldOrigin, DirectX::XMFLOAT3& fieldExtents, DirectX::XMINT3& dispatchSize, SWorldFieldUpdateParams &params);

private:

	const int UPDATE_FREQUENCY = 2;

	float __GetBoundingSphereRadius(float width, float height, float depth);

	std::vector<CColorBuffer*> m_Data;
	std::vector<DirectX::XMMATRIX> m_CachedInverseTransforms;
	std::vector<DirectX::XMMATRIX> m_CachedTransforms;
	std::vector<DirectX::XMMATRIX> m_CachedRotations;
	std::vector<SDimensions> m_Dimensions;
	std::vector<DirectX::XMFLOAT3> m_Positions;
	std::vector<DirectX::XMFLOAT3> m_Rotations;
	std::vector<float> m_Scales;
	std::vector<float> m_GridResolutions;
	std::vector<size_t> m_Offsets;
	std::vector<SBounds> m_FieldBounds;

	std::vector<SWorldFieldUpdateParams> m_OldParams;
	std::vector<int> m_NeedsUpdate;
};