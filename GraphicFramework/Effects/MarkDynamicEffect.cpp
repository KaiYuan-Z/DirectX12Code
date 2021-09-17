#include "MarkDynamicEffect.h"
#include "../Core/GraphicsCommon.h"
#include "../Core/CameraWalkthrough.h"
#include "../Core/GeometryGenerator.h"
#include "../Core/GraphicsCore.h"

#include "CompiledShaders/MarkDynamicEffectPS.h"
#include "CompiledShaders/MarkDynamicEffectVS.h"

CMarkDynamicEffect::CMarkDynamicEffect()
{
}


CMarkDynamicEffect::~CMarkDynamicEffect()
{
}

void CMarkDynamicEffect::Init(const SMarkDynamicConfig& config)
{
	_ASSERTE(config.WinWidth > 0 && config.WinHeight > 0 && config.pScene);
	m_MarkDynamicConfig = config;
	m_ViewPort.TopLeftX = m_ViewPort.TopLeftY = 0;
	m_ViewPort.Width = (float)config.WinWidth;
	m_ViewPort.Height = (float)config.WinHeight;
	m_ViewPort.MinDepth = D3D12_MIN_DEPTH;
	m_ViewPort.MaxDepth = D3D12_MAX_DEPTH;
	m_ScissorRect.left = (LONG)m_ViewPort.TopLeftX;
	m_ScissorRect.top = (LONG)m_ViewPort.TopLeftY;
	m_ScissorRect.right = (LONG)m_ViewPort.TopLeftX + (LONG)m_ViewPort.Width;
	m_ScissorRect.bottom = (LONG)m_ViewPort.TopLeftY + (LONG)m_ViewPort.Height;

	m_DynamicMarkMap.Create2D(L"DynamicMarkMap", config.WinWidth, config.WinHeight, 1, DXGI_FORMAT_R8_SINT);

	m_OBBFinnals.resize(config.pScene->GetInstanceCount() * 2 + config.pScene->FetchGrassManager().GetGrarssPatchCount());
	m_OBBTags.resize(config.pScene->GetInstanceCount() * 2 + config.pScene->FetchGrassManager().GetGrarssPatchCount());

	__InitRootRootSignature();
	__InitPSO();
	__InitGeometry();

	if (m_MarkDynamicConfig.EnableGUI)
	{
		GraphicsCore::RegisterGuiCallbakFunction(std::bind(&CMarkDynamicEffect::__UpdateGUI, this));
	}
}

void CMarkDynamicEffect::Render(float extent, CDepthBuffer* pCurrentDepth, CGraphicsCommandList* pCommandList)
{
	_ASSERTE(pCurrentDepth && pCommandList);

	extent += m_ExtentAppend;

	CCameraBasic* pCamera = m_MarkDynamicConfig.pScene->GetCamera(m_MarkDynamicConfig.ActiveCameraIndex);
	_ASSERTE(pCamera);
	XMMATRIX ViewProj = pCamera->GetViewXMM()*pCamera->GetProjectXMM();

	UINT RenderOBBCount = 0;

	auto& grassManager = m_MarkDynamicConfig.pScene->FetchGrassManager();
	for (UINT grassPatchIndex = 0; grassPatchIndex < grassManager.GetGrarssPatchCount(); grassPatchIndex++)
	{
		if (grassManager.FetchParamter().windStrength > 0.0f)
		{
			m_OBBFinnals[RenderOBBCount] = __CreateOBBFinal(grassManager.GetGrarssPatchBoundingObject(grassPatchIndex), extent);
			m_OBBTags[RenderOBBCount] = DEFORMABLE_INSTANCE_TAG | GRASS_INST_TAG | DYNAMIC_INSTANCE_TAG;
			RenderOBBCount++;
		}
	}

	for (UINT instanceIndex = 0; instanceIndex < m_MarkDynamicConfig.pScene->GetInstanceCount(); instanceIndex++)
	{
		CModelInstance* pInstance = m_MarkDynamicConfig.pScene->GetInstance(instanceIndex);
		_ASSERTE(pInstance);

		if (pInstance->IsDynamic() && pInstance->IsUpdatedInCurrentFrame())
		{
			if (m_MarkPreFrameDynamicInstance && GraphicsCore::GetFrameID() > 0)
			{		
				m_OBBFinnals[RenderOBBCount] = __CreateOBBFinal(pInstance->GetPrevBoundingObject(), extent);
				m_OBBTags[RenderOBBCount] = pInstance->IsDynamic() ? DYNAMIC_INSTANCE_TAG : 0;
				RenderOBBCount++;
			}

			m_OBBFinnals[RenderOBBCount] = __CreateOBBFinal(pInstance->GetBoundingObject(), extent);
			m_OBBTags[RenderOBBCount] = pInstance->IsDynamic() ? DYNAMIC_INSTANCE_TAG : 0;
			RenderOBBCount++;
		}
	}
	
	pCommandList->SetViewportAndScissor(m_ViewPort, m_ScissorRect);
	pCommandList->SetRenderTarget(m_DynamicMarkMap.GetRTV(), pCurrentDepth->GetDSV());
	pCommandList->SetVertexBuffer(0, m_VertexBufferView);
	pCommandList->SetIndexBuffer(m_IndexBufferView);
	pCommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	pCommandList->SetRootSignature(m_RootSignature);
	pCommandList->TransitionResource(m_DynamicMarkMap, D3D12_RESOURCE_STATE_RENDER_TARGET, false);
	pCommandList->TransitionResource(*pCurrentDepth, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
	pCommandList->ClearColor(m_DynamicMarkMap);
	pCommandList->SetPipelineState(m_PSO1);
	pCommandList->SetStencilRef(1);
	pCommandList->SetDynamicConstantBufferView(0, sizeof(ViewProj), &ViewProj);
	pCommandList->SetDynamicSRV(1, sizeof(XMMATRIX)* RenderOBBCount, m_OBBFinnals.data());
	pCommandList->SetDynamicSRV(2, sizeof(int) * RenderOBBCount, m_OBBTags.data());
	pCommandList->DrawIndexedInstanced(36, RenderOBBCount, 0, 0, 0);

	pCommandList->SetRootSignature(m_RootSignature);
	pCommandList->TransitionResource(m_DynamicMarkMap, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	pCommandList->SetPipelineState(m_PSO2);
	pCommandList->SetStencilRef(1);
	pCommandList->SetDynamicConstantBufferView(0, sizeof(ViewProj), &ViewProj);
	pCommandList->SetDynamicSRV(1, sizeof(XMMATRIX) * RenderOBBCount, m_OBBFinnals.data());
	pCommandList->SetDynamicSRV(2, sizeof(int) * RenderOBBCount, m_OBBTags.data());
	pCommandList->DrawIndexedInstanced(36, RenderOBBCount, 0, 0, 0);

	pCommandList->SetStencilRef(0);
}

void CMarkDynamicEffect::__InitRootRootSignature()
{
	m_RootSignature.Reset(3, 0);
	m_RootSignature[0].InitAsConstantBuffer(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	m_RootSignature[1].InitAsBufferSRV(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	m_RootSignature[2].InitAsBufferSRV(1, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	m_RootSignature.Finalize(L"MarkDynamicEffect-m_RootSignature", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
}

void CMarkDynamicEffect::__InitPSO()
{
	m_PSO1.SetInputLayout((UINT)GraphicsCommon::InputLayout::PosColor.size(), GraphicsCommon::InputLayout::PosColor.data());
	m_PSO1.SetRootSignature(m_RootSignature);
	m_PSO1.SetVertexShader(g_pMarkDynamicEffectVS,sizeof(g_pMarkDynamicEffectVS));
	m_PSO1.SetPixelShader(g_pMarkDynamicEffectPS, sizeof(g_pMarkDynamicEffectPS));
	auto RasterizerState1 = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	m_PSO1.SetRasterizerState(RasterizerState1);
	auto DepthStencilState1 = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	DepthStencilState1.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	DepthStencilState1.DepthEnable = TRUE;
	DepthStencilState1.StencilEnable = TRUE;
	DepthStencilState1.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	m_PSO1.SetDepthStencilState(DepthStencilState1);
	auto BlendState1 = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	BlendState1.RenderTarget[0].RenderTargetWriteMask = 0;
	m_PSO1.SetBlendState(BlendState1);
	m_PSO1.SetSampleMask(UINT_MAX);
	m_PSO1.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	m_PSO1.SetRenderTargetFormat(DXGI_FORMAT_R8_SINT, m_MarkDynamicConfig.DepthStencilBufferFormat);
	m_PSO1.Finalize();

	m_PSO2 = m_PSO1;
	m_PSO2.SetRootSignature(m_RootSignature);
	m_PSO2.SetVertexShader(g_pMarkDynamicEffectVS, sizeof(g_pMarkDynamicEffectVS));
	m_PSO2.SetPixelShader(g_pMarkDynamicEffectPS, sizeof(g_pMarkDynamicEffectPS));
	auto RasterizerState2 = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	RasterizerState2.CullMode = D3D12_CULL_MODE_FRONT;
	m_PSO2.SetRasterizerState(RasterizerState2);
	auto DepthStencilState2 = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	DepthStencilState2.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	DepthStencilState2.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	DepthStencilState2.StencilEnable = TRUE;
	DepthStencilState2.DepthEnable = TRUE;
	DepthStencilState2.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
	m_PSO2.SetDepthStencilState(DepthStencilState2);
	auto BlendState2 = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	BlendState2.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	m_PSO2.SetBlendState(BlendState2);
	m_PSO2.Finalize();
}

void CMarkDynamicEffect::__InitGeometry()
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

XMMATRIX CMarkDynamicEffect::__CreateOBBFinal(const SBoundingObject& boundingObject, float extent)
{
	auto& OBB = boundingObject.boundingOrientedBox;
	XMMATRIX T = XMMatrixTranslation(OBB.Center.x, OBB.Center.y, OBB.Center.z);
	XMMATRIX R = XMMatrixRotationQuaternion(XMLoadFloat4(&OBB.Orientation));
	XMMATRIX S = XMMatrixScaling(OBB.Extents.x + extent, OBB.Extents.y + extent, OBB.Extents.z + extent);
	return S * R * T;
}

void CMarkDynamicEffect::__UpdateGUI()
{
	ImGui::SetNextWindowSize(ImVec2(500, 80));
	ImGui::Begin("CMarkDynamicEffect");
	ImGui::SliderFloat("ExtentRadiusAppend", &m_ExtentAppend, 0.0f, 5.0f, "%.2f");
	ImGui::Checkbox("MarkPreFrameDynamicInstance", &m_MarkPreFrameDynamicInstance);
	ImGui::End();
}
