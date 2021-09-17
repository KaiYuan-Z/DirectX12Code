#pragma once
#include "../../GraphicFramework/Core/DXSample.h"
#include "../../GraphicFramework/Core/MathUtility.h"
#include "../../GraphicFramework/Core/CameraWalkthrough.h"
#include "../../GraphicFramework/Core/StructuredBuffer.h"
#include "../../GraphicFramework/Core/ByteAddressBuffer.h"

using namespace DirectX;

struct SObjectConstants
{
	XMMATRIX WorldViewProj;
};

struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};


class BlankWindow : public CDXSample
{
public:
	BlankWindow(std::wstring name);

    virtual void OnInit() override;
    virtual void OnKeyDown(UINT8 key) override;
	virtual void OnMouseMove(UINT x, UINT y) override;
	virtual void OnRightButtonDown(UINT x, UINT y) override;
    virtual void OnUpdate() override;
    virtual void OnRender() override;
	virtual void OnDestroy() override {}

private:
	CCameraWalkthrough m_Camera;

	CStructuredBuffer m_VertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
	CByteAddressBuffer m_IndexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

	XMMATRIX m_World;
	SObjectConstants m_ObjectConstants;

	CGraphicsPSO m_PSO;
	CRootSignature m_RootSignature;

	void __BuildRootSignature();
	void __BuildBoxGeometry();
	void __BuildPSO();
};