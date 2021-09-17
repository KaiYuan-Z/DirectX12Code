#pragma once
#include "DirectXMath.h"
#include "DirectXCollision.h"

struct SBoundingObject
{
	DirectX::BoundingSphere			boundingSphere;
	DirectX::BoundingBox			boundingBox;
	DirectX::BoundingOrientedBox	boundingOrientedBox;

	SBoundingObject()
	{
		Clear();
	}

	void ComputeFromPoints(_In_ size_t Count, _In_reads_bytes_(sizeof(XMFLOAT3) + Stride * (Count - 1)) const DirectX::XMFLOAT3* pPoints, _In_ size_t Stride)
	{
		DirectX::BoundingSphere::CreateFromPoints(boundingSphere, Count, pPoints, Stride);
		DirectX::BoundingBox::CreateFromPoints(boundingBox, Count, pPoints, Stride);
		//DirectX::BoundingOrientedBox::CreateFromPoints(boundingOrientedBox, Count, pPoints, Stride);
		DirectX::BoundingOrientedBox::CreateFromBoundingBox(boundingOrientedBox, boundingBox);
	}

	void Transform(SBoundingObject& OutBoundingObject, const DirectX::XMMATRIX& TransformMatrix) const
	{
		boundingSphere.Transform(OutBoundingObject.boundingSphere, TransformMatrix);
		boundingBox.Transform(OutBoundingObject.boundingBox, TransformMatrix);
		boundingOrientedBox.Transform(OutBoundingObject.boundingOrientedBox, TransformMatrix);
	}

	void Clear()
	{
		boundingSphere.Center = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
		boundingSphere.Radius = 0;
		boundingBox.Center = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
		boundingBox.Extents = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
		boundingOrientedBox.Center = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
		boundingOrientedBox.Extents = DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f);
		boundingOrientedBox.Orientation = DirectX::XMFLOAT4(0, 0, 0, 1.0f);
	}
};