#include "BlankWindow.h"
#include "../../GraphicFramework/Core/SamplerDesc.h"
#include "../../GraphicFramework/Core/RootSignature.h"
#include "../../GraphicFramework/Core/GraphicsCore.h"
#include "../../GraphicFramework/Core/GraphicsCommon.h"

#include "CompiledShaders/ColorPS.h"
#include "CompiledShaders/ColorVS.h"

using Microsoft::WRL::ComPtr;
using namespace std;
using namespace DirectX;

BlankWindow::BlankWindow(std::wstring name) : CDXSample(name)
{
	m_Camera.SetLens(XMConvertToRadians(45), float(GraphicsCore::GetWindowWidth()) / float(GraphicsCore::GetWindowHeight()), 0.1f, 1000);
	m_Camera.SetLookAt(XMFLOAT3(10,0,0), XMFLOAT3(0,0,0), XMFLOAT3(0,1,0));
	m_Camera.SetMoveStep(0.5f);
	m_Camera.SetRotateStep(0.0025f);
}

void BlankWindow::OnInit()
{
	__BuildRootSignature();
	__BuildBoxGeometry();
	__BuildPSO();

	GraphicsCore::IdleGPU();
}

void BlankWindow::OnKeyDown(UINT8 key)
{
    /*switch (key)
    {
	default:_ASSERT(false); break;
    }*/

	m_Camera.OnKeyDown(key);
}

void BlankWindow::OnRightButtonDown(UINT x, UINT y)
{
	m_Camera.OnMouseButtonDown(x, y);
}

void BlankWindow::OnMouseMove(UINT x, UINT y)
{
	m_Camera.OnMouseMove(x, y);
}

// Update frame-based values.
void BlankWindow::OnUpdate()
{
	m_Camera.OnFrameUpdate();
	m_ObjectConstants.WorldViewProj = m_Camera.GetViewXMM() *m_Camera.GetProjectXMM();
}

// Render the scene.
void BlankWindow::OnRender()
{
    if (!GraphicsCore::IsWindowVisible())
    {
        return;
    }

	CGraphicsCommandList* pGraphicsCommandList = GraphicsCore::BeginGraphicsCommandList();
	_ASSERTE(pGraphicsCommandList);

	pGraphicsCommandList->SetViewportAndScissor(GraphicsCore::GetScreenViewport(), GraphicsCore::GetScissorRect());

	pGraphicsCommandList->TransitionResource(GraphicsCore::GetCurrentRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, false);
	pGraphicsCommandList->TransitionResource(GraphicsCore::GetDepthStencil(), D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
	pGraphicsCommandList->ClearColor(GraphicsCore::GetCurrentRenderTarget());
	pGraphicsCommandList->ClearDepth(GraphicsCore::GetDepthStencil());

	pGraphicsCommandList->SetRootSignature(m_RootSignature);
	pGraphicsCommandList->SetPipelineState(m_PSO);
	pGraphicsCommandList->SetRenderTarget(GraphicsCore::GetCurrentRenderTarget().GetRTV(), GraphicsCore::GetDepthStencil().GetDSV());
	pGraphicsCommandList->SetDynamicConstantBufferView(0, sizeof(SObjectConstants), &m_ObjectConstants);
	pGraphicsCommandList->SetVertexBuffer(0, m_VertexBufferView);
	pGraphicsCommandList->SetIndexBuffer(m_IndexBufferView);
	pGraphicsCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pGraphicsCommandList->DrawIndexed(36, 0, 0);

	GraphicsCore::FinishCommandListAndPresent(pGraphicsCommandList);
}

void BlankWindow::__BuildRootSignature()
{
	CSamplerDesc DefaultSamplerDesc;
	DefaultSamplerDesc.MaxAnisotropy = 8;

	m_RootSignature.Reset(1, 1);
	m_RootSignature.InitStaticSampler(0, 0, DefaultSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature[0].InitAsConstantBuffer(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	m_RootSignature.Finalize(L"BlankWindow", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
}

void BlankWindow::__BuildBoxGeometry()
{
	std::array<Vertex, 8> vertices =
	{
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
	};

	std::array<std::uint16_t, 36> indices =
	{
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};
	
	m_VertexBuffer.Create(L"Box Vertex Buffer", (UINT)vertices.size(), (UINT)sizeof(Vertex), &vertices[0]);
	m_VertexBufferView = m_VertexBuffer.CreateVertexBufferView();
	m_IndexBuffer.Create(L"Box Index Buffer", (UINT)indices.size(), (UINT)sizeof(uint16_t), indices.data());
	m_IndexBufferView = m_IndexBuffer.CreateIndexBufferView();
}

void BlankWindow::__BuildPSO()
{
	m_PSO.SetInputLayout((UINT)GraphicsCommon::InputLayout::PosColor.size(), GraphicsCommon::InputLayout::PosColor.data());
	m_PSO.SetRootSignature(m_RootSignature);
	m_PSO.SetVertexShader(g_pColorVS, sizeof(g_pColorVS));
	m_PSO.SetPixelShader(g_pColorPS, sizeof(g_pColorPS));
	m_PSO.SetRasterizerState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
	m_PSO.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
	m_PSO.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
	m_PSO.SetDepthStencilState(CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT));
	m_PSO.SetSampleMask(UINT_MAX);
	m_PSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	DXGI_FORMAT RenderTargetFormat = GraphicsCore::GetBackBufferFormat();
	m_PSO.SetRenderTargetFormats(1, &RenderTargetFormat, GraphicsCore::GetDepthBufferFormat());
	m_PSO.Finalize();
}
