#include "ModelViewer.h"
#include "../../GraphicFramework/Core/RootSignature.h"
#include "../../GraphicFramework/Core/GraphicsCore.h"
#include "../../GraphicFramework/Core/GraphicsCommon.h"

#include "CompiledShaders/ModelViewerPS.h"
#include "CompiledShaders/ModelViewerVS.h"

using Microsoft::WRL::ComPtr;
using namespace std;
using namespace DirectX;

ModelViewer::ModelViewer(std::wstring name) : CDXSample(name)
{
}

void ModelViewer::OnInit()
{
	GraphicsCore::RegisterGuiCallbakFunction(std::bind(&ModelViewer::__UpdateGUI, this));
	
	__BuildScene();
	__BuildRootSignature();
	__BuildPSO();
	__InitDisplayList();

	SShadowMapConfig ShadowMapConfig;
	ShadowMapConfig.LightIndexForCastShadow = 0;
	ShadowMapConfig.MaxModelInstanceCount = c_MaxInstanceCount;
	ShadowMapConfig.OpenBackFaceCulling = false;
	ShadowMapConfig.pScene = &m_Scene;
	ShadowMapConfig.Size = 4096;
	m_ShadowMapEffect.Init(ShadowMapConfig);
	m_ShadowMapEffect.UpdateShadowTransform({ 0.0f, 0.0f, 0.0f }, 120.0f);

	SSkyEffectConfig SkyEffectConfig;
	SkyEffectConfig.CubeMapName = L"sky_cube";
	SkyEffectConfig.RenderTargetFormat = GraphicsCore::GetBackBufferFormat();
	SkyEffectConfig.pMainCamera = m_Scene.GetCamera(0);
	m_SkyEffect.Init(SkyEffectConfig);

	GraphicsCore::IdleGPU();
}

void ModelViewer::OnKeyDown(UINT8 key)
{
	/*switch (key)
	{
	default:_ASSERT(false); break;
	}*/

	m_pCamera->OnKeyDown(key);
}

void ModelViewer::OnMouseMove(UINT x, UINT y)
{
	m_pCamera->OnMouseMove(x, y);
}

void ModelViewer::OnRightButtonDown(UINT x, UINT y)
{
	m_pCamera->OnMouseButtonDown(x, y);
}

// Update frame-based values.
void ModelViewer::OnUpdate()
{
	m_Scene.OnFrameUpdate();
}

// Render the scene.
void ModelViewer::OnRender()
{
	if (!GraphicsCore::IsWindowVisible())
	{
		return;
	}

	CGraphicsCommandList* pGraphicsCommandList = GraphicsCore::BeginGraphicsCommandList();
	_ASSERTE(pGraphicsCommandList);

	m_ShadowMapEffect.Render(pGraphicsCommandList);

	m_SkyEffect.Render(true, &GraphicsCore::GetCurrentRenderTarget(), pGraphicsCommandList);

	pGraphicsCommandList->SetViewportAndScissor(GraphicsCore::GetScreenViewport(), GraphicsCore::GetScissorRect());
	pGraphicsCommandList->TransitionResource(m_ShadowMapEffect.GetShadowMapBuffer(), D3D12_RESOURCE_STATE_DEPTH_READ, false);
	pGraphicsCommandList->TransitionResource(GraphicsCore::GetCurrentRenderTarget(), D3D12_RESOURCE_STATE_RENDER_TARGET, false);
	pGraphicsCommandList->TransitionResource(GraphicsCore::GetDepthStencil(), D3D12_RESOURCE_STATE_DEPTH_WRITE, true);
	pGraphicsCommandList->ClearDepth(GraphicsCore::GetDepthStencil());

	pGraphicsCommandList->SetRootSignature(m_RootSignature);
	pGraphicsCommandList->SetPipelineState(m_PSO);
	pGraphicsCommandList->SetRenderTarget(GraphicsCore::GetCurrentRenderTarget().GetRTV(), GraphicsCore::GetDepthStencil().GetDSV());

	pGraphicsCommandList->SetDynamicConstantBufferView(m_RootParamStartIndex, sizeof(XMMATRIX), &m_ShadowMapEffect.GetShadowTransform());
	if (m_EnableHighLight)
	{
		pGraphicsCommandList->SetConstants(m_RootParamStartIndex + 1, m_SelectedInstanceIndex, m_SelectedGrassPatchIndex);
	}
	else
	{
		pGraphicsCommandList->SetConstants(m_RootParamStartIndex + 1, 0xffffffff, 0xffffffff);
	}	
	pGraphicsCommandList->SetDynamicDescriptor(m_RootParamStartIndex + 2, 0, m_ShadowMapEffect.GetShadowMapBuffer().GetDepthSRV());

	m_SceneRender.DrawScene(pGraphicsCommandList);

	GraphicsCore::FinishCommandListAndPresent(pGraphicsCommandList);
}

void ModelViewer::__BuildRootSignature()
{
	m_RootParamStartIndex = m_SceneRender.GetRootParameterCount();
	m_RootSignature.Reset(m_SceneRender.GetRootParameterCount() + 3, m_SceneRender.GetStaticSamplerCount() + 1);
	m_RootSignature.InitStaticSampler(0, 0, GraphicsCommon::Sampler::SamplerShadowDesc);
	m_SceneRender.ConfigurateRootSignature(m_RootSignature);
	m_RootSignature[m_RootParamStartIndex].InitAsConstantBuffer(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	m_RootSignature[m_RootParamStartIndex + 1].InitAsConstants(1, 2, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature[m_RootParamStartIndex + 2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature.Finalize(L"ModelViewer", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
}

void ModelViewer::__BuildPSO()
{
	m_PSO.SetInputLayout((UINT)GraphicsCommon::InputLayout::PosTexNormTanBitan.size(), GraphicsCommon::InputLayout::PosTexNormTanBitan.data());
	m_PSO.SetRootSignature(m_RootSignature);
	m_PSO.SetVertexShader(g_pModelViewerVS, sizeof(g_pModelViewerVS));
	m_PSO.SetPixelShader(g_pModelViewerPS, sizeof(g_pModelViewerPS));
	auto RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	RasterizerState.CullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_NONE;
	m_PSO.SetRasterizerState(RasterizerState);
	m_PSO.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
	m_PSO.SetDepthStencilState(CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT));
	m_PSO.SetSampleMask(UINT_MAX);
	m_PSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	DXGI_FORMAT RenderTargetFormat = GraphicsCore::GetBackBufferFormat();
	m_PSO.SetRenderTargetFormats(1, &RenderTargetFormat, GraphicsCore::GetDepthBufferFormat());
	m_PSO.Finalize();
}

void ModelViewer::__BuildScene()
{
	// Init Scene
	m_Scene.LoadScene(L"scene.txt");
	m_pCamera = m_Scene.GetCamera(m_Scene.QueryCameraIndex(L"Camera0"))->QueryWalkthroughCamera();

	// Init Render
	SSceneForwardRenderConfig renderConfig;
	renderConfig.pScene = &m_Scene;
	renderConfig.ActiveCameraIndex = m_Scene.QueryCameraIndex(L"Camera0");
	renderConfig.MaxModelInstanceCount = c_MaxInstanceCount;
	renderConfig.MaxSceneLightCount = c_MaxLightCount;
	renderConfig.AmbientLight = { 0.35f, 0.35f, 0.35f, 1.0f };
	renderConfig.EnableNormalMap = true;
	renderConfig.EnableNormalMapCompression = true;
	m_SceneRender.InitRender(renderConfig);
}

void ModelViewer::__InitDisplayList()
{
	m_CurrentModelCount = m_Scene.GetModelCountInScene();
	m_ModelNameSet.resize(m_CurrentModelCount);
	for (UINT m = 0; m < m_CurrentModelCount; m++)
	{
		CModel* pModel = m_Scene.GetModelInScene(m);
		_ASSERTE(pModel);
		m_ModelNameSet[m] = MakeStr(pModel->GetModelName());
		m_ModelNameList[m] = m_ModelNameSet[m].data();
	}
}

void ModelViewer::__UpdateGUI()
{
	ImGui::SetNextWindowSize(ImVec2(750, 450));
	ImGui::Begin("ModelViewer");

	// Enbable High Light
	{
		ImGui::Separator();
		ImGui::Separator();
		ImGui::Text("[Active Camera Info]");
		ImGui::Separator();
		std::string camPosStr = "Camera Pos: [" + std::to_string(m_pCamera->GetPositionXMF3().x) + ", " + std::to_string(m_pCamera->GetPositionXMF3().y) + ", " + std::to_string(m_pCamera->GetPositionXMF3().z) + "]";
		ImGui::Text(camPosStr.data());
		std::string camDirStr = "Camera Dir: [" + std::to_string(m_pCamera->GetLookXMF3().x) + ", " + std::to_string(m_pCamera->GetLookXMF3().y) + ", " + std::to_string(m_pCamera->GetLookXMF3().z) + "]";
		ImGui::Text(camDirStr.data());
		ImGui::Separator();
		ImGui::Separator();
	}

	ImGui::Text("   ");

	// Global Config
	static bool isAddInstanceSameAsLast = true;
	{
		ImGui::Separator();
		ImGui::Separator();
		ImGui::Text("[Global Config]");
		ImGui::Separator();
		ImGui::Checkbox("Enable High Light", &m_EnableHighLight);
		ImGui::Separator();
		ImGui::Checkbox("Add Instance Same As Last", &isAddInstanceSameAsLast);
		ImGui::Separator();
		ImGui::Separator();
	}

	ImGui::Text("   ");

	// Model Instance
	static int modelSelectedIndex = 0;
	static int instanceSelectedIndex = 0;
	static bool needAddInstance = false;
	static bool needDeleteInstance = false;
	static bool needUpdateInstanceList = false;
	static bool needResetInstanceGUI = false;
	static bool needUpdateInstance = false;
	static bool needSeletedLastInstance = false;
	static bool isDynamic = false;
	static float instScale = 1.0f;
	static XMFLOAT3 instPos, instYawPitchRoll;
	{
		ImGui::Separator();
		ImGui::Separator();
		ImGui::Text("[Model Instance Config]");
		ImGui::Separator();
		needUpdateInstanceList = ImGui::Combo("Model Name", &modelSelectedIndex, m_ModelNameList, m_CurrentModelCount);
		needResetInstanceGUI = ImGui::Combo("Instance Name", &instanceSelectedIndex, m_InstanceNameList, m_CurrentInstanceCount);
		needAddInstance = ImGui::Button("Add Inst");
		ImGui::SameLine();
		needDeleteInstance = ImGui::Button("Del Inst");
		ImGui::Separator();
		if (instanceSelectedIndex >= 0)
		{
			needUpdateInstance |= ImGui::InputFloat("Instance Scale", &instScale, 0.005, 1.0, 4);
			ImGui::Separator();
			needUpdateInstance |= ImGui::InputFloat("Instance PosX", &instPos.x, 0.1, 1.0, 2);
			needUpdateInstance |= ImGui::InputFloat("Instance PosY", &instPos.y, 0.1, 1.0, 2);
			needUpdateInstance |= ImGui::InputFloat("Instance PosZ", &instPos.z, 0.1, 1.0, 2);
			ImGui::Separator();
			needUpdateInstance |= ImGui::InputFloat("Instance Yaw", &instYawPitchRoll.x, 1.0, 1.0, 2);
			needUpdateInstance |= ImGui::InputFloat("Instance Pitch", &instYawPitchRoll.y, 1.0, 1.0, 2);
			needUpdateInstance |= ImGui::InputFloat("Instance Roll", &instYawPitchRoll.z, 1.0, 1.0, 2);
			ImGui::Separator();
			needUpdateInstance |= ImGui::Checkbox("Instance Dynamic Hint", &isDynamic);
		}
		ImGui::Separator();
		ImGui::Separator();
	}

	ImGui::Text("   ");

	// Grass 
	auto& grassManager = m_Scene.FetchGrassManager();
	auto& grassParam = grassManager.FetchParamter();
	static int grassPatchSelectedIndex = 0;
	static bool needUpdateGrassPatchList = true;
	static bool needAddGrassPatch = false;
	static bool needDelGrassPatch = false;
	static bool needResetGrassPatchGUI = false;
	static bool needUpdateGrassPatch = false;
	static bool needSelectedLastGrassPatch = false;
	static UINT patchDim = grassParam.activePatchDim.x;
	static XMFLOAT3 windDirection = grassParam.windDirection;
	static float grassHeight = grassParam.grassHeight;
	static float grassThickness = grassParam.grassThickness;
	static float windStrength = grassParam.windStrength;
	static float positionJitterStrength = grassParam.positionJitterStrength;
	static float bendStrengthAlongTangent = grassParam.bendStrengthAlongTangent;	
	static float patchScale = 1.0f;
	static XMFLOAT3 patchPos;
	{
		ImGui::Separator();
		ImGui::Separator();
		ImGui::Text("[Grass Config]");
		ImGui::Separator();
		if (ImGui::SliderInt("Grass patch dim", (int*)(&patchDim), 1, MAX_GRASS_STRAWS_1D)) { grassParam.activePatchDim.x = patchDim;  grassParam.activePatchDim.y = patchDim; }
		ImGui::Separator();
		if (ImGui::InputFloat("Grass Height", &grassHeight, 0.01f, 1.0f, 2)) { if (grassHeight < 0.01f) grassHeight = 0.01f; grassParam.grassHeight = grassHeight; }
		if (ImGui::InputFloat("Grass Thickness", &grassThickness, 0.01f, 1.0f, 2)) { if (grassThickness < 0.01f) grassThickness = 0.01f; grassParam.grassThickness = grassThickness; }
		ImGui::Separator();
		if (ImGui::InputFloat("Grass position jitter strength", &positionJitterStrength, 0.01f, 1.0f, 2)) { if (positionJitterStrength < 0.01f) positionJitterStrength = 0.01f; grassParam.positionJitterStrength = positionJitterStrength; }
		if (ImGui::InputFloat("Grass bend strength along tangent", &bendStrengthAlongTangent, 0.01f, 1.0f, 2)) { if (bendStrengthAlongTangent < 0.01f) bendStrengthAlongTangent = 0.01f; grassParam.bendStrengthAlongTangent = bendStrengthAlongTangent; }
		ImGui::Separator();
		if (ImGui::InputFloat("Grass wind strength", &windStrength, 0.01f, 1.0f, 2)) { if (windStrength < 0.0f) windStrength = 0.0f; grassParam.windStrength = windStrength; }
		ImGui::InputFloat("Grass wind directionX", &grassParam.windDirection.x, 0.01f, 1.0f, 2);
		ImGui::InputFloat("Grass wind directionY", &grassParam.windDirection.y, 0.01f, 1.0f, 2);
		ImGui::InputFloat("Grass wind directionZ", &grassParam.windDirection.z, 0.01f, 1.0f, 2);
		ImGui::Separator();
		needResetGrassPatchGUI = ImGui::Combo("Grass Patch Name", &grassPatchSelectedIndex, m_GrassPatchNameList, m_CurrentGrassPatchCount);	
		needAddGrassPatch = ImGui::Button("Add Patch");
		ImGui::SameLine();
		needDelGrassPatch = ImGui::Button("Del Patch");
		ImGui::Separator();
		if (grassPatchSelectedIndex >= 0)
		{
			needUpdateGrassPatch |= ImGui::InputFloat("Patch Scale", &patchScale, 0.01, 1.0, 2);
			ImGui::Separator();
			needUpdateGrassPatch |= ImGui::InputFloat("Patch PosX", &patchPos.x, 0.1, 1.0, 2);
			needUpdateGrassPatch |= ImGui::InputFloat("Patch PosY", &patchPos.y, 0.1, 1.0, 2);
			needUpdateGrassPatch |= ImGui::InputFloat("Patch PosZ", &patchPos.z, 0.1, 1.0, 2);
		}
		ImGui::Separator();
		ImGui::Separator();
	}

	ImGui::Text("   ");

	// Save Scene
	static bool needSaveSceneConfig = false;
	{
		ImGui::Separator();
		ImGui::Separator();
		ImGui::Text("[Save Config]");
		ImGui::Separator();
		needSaveSceneConfig = ImGui::Button("Save Scene");
		ImGui::Separator();
		ImGui::Separator();
	}

	ImGui::End();

	// Model Instance
	{
		if (GraphicsCore::GetFrameID() == 0)
		{
			needUpdateInstanceList = true;
		}
		
		if (needAddInstance)
		{
			_ASSERTE(modelSelectedIndex < m_Scene.GetModelCountInScene());

			CModel* pModel = m_Scene.GetModelInScene(modelSelectedIndex);
			_ASSERTE(pModel);
			std::wstring ModelName = pModel->GetModelName();
			UINT modelIndex = m_Scene.QueryModelIndexInScene(ModelName);
			int lastInstIndexOfModel = m_Scene.GetModelInstanceCount(ModelName) - 1;
			UINT instId = m_CurrentInstanceCount;
			std::wstring InstName = ModelName + L"_Inst" + std::to_wstring(m_CurrentInstanceCount);
			while (m_Scene.GetInstance(InstName) != nullptr)
			{
				instId++;
				InstName = ModelName + L"_Inst" + std::to_wstring(instId);
			}						
			m_Scene.AddInstance(InstName, ModelName);			
			if (isAddInstanceSameAsLast && lastInstIndexOfModel >= 0)
			{
				auto* pLastInst = m_Scene.GetModelInstance(modelIndex, lastInstIndexOfModel);
				_ASSERTE(pLastInst);
				auto* pNewInst = m_Scene.GetModelInstance(modelIndex, lastInstIndexOfModel + 1);
				_ASSERTE(pNewInst);
				pNewInst->SetInstance(pLastInst->GetTranslation(), pLastInst->GetRotation(), pLastInst->GetScaling());
				pNewInst->SetDynamic(pLastInst->IsDynamic());
			}

			needSeletedLastInstance = true;

			needUpdateInstanceList = true;

			needAddInstance = false;
		}

		if (needDeleteInstance)
		{
			_ASSERTE(modelSelectedIndex < m_Scene.GetModelCountInScene());

			if (instanceSelectedIndex < m_Scene.GetModelInstanceCount(modelSelectedIndex))
			{
				CModelInstance* pInstance = m_Scene.GetModelInstance(modelSelectedIndex, instanceSelectedIndex);
				_ASSERTE(pInstance);
				m_Scene.DeleteInstance(pInstance->GetName());
			}

			needUpdateInstanceList = true;

			needAddInstance = false;
		}
		
		if (needUpdateInstanceList)
		{
			_ASSERTE(modelSelectedIndex < m_Scene.GetModelCountInScene());

			m_CurrentInstanceCount = m_Scene.GetModelInstanceCount(modelSelectedIndex);
			m_InstanceNameSet.resize(m_CurrentInstanceCount);
			for (UINT i = 0; i < m_CurrentInstanceCount; i++)
			{
				CModelInstance* pInstance = m_Scene.GetModelInstance(modelSelectedIndex, i);
				_ASSERTE(pInstance);
				m_InstanceNameSet[i] = MakeStr(pInstance->GetName());
				m_InstanceNameList[i] = m_InstanceNameSet[i].data();
			}
			if (m_CurrentInstanceCount > 0)
			{
				if (needSeletedLastInstance)
				{
					instanceSelectedIndex = m_CurrentInstanceCount - 1;
					needSeletedLastInstance = false;
				}
				else
				{
					instanceSelectedIndex = 0;
				}			
			}
			else
			{
				instanceSelectedIndex = -1;
			}
			needResetInstanceGUI = true;

			needUpdateInstanceList = false;
		}

		if (needResetInstanceGUI && instanceSelectedIndex >= 0)
		{
			if (instanceSelectedIndex >= 0)
			{
				_ASSERTE(modelSelectedIndex < m_Scene.GetModelCountInScene());
				_ASSERTE(instanceSelectedIndex < m_Scene.GetModelInstanceCount(modelSelectedIndex));

				CModelInstance* pInstance = m_Scene.GetModelInstance(modelSelectedIndex, instanceSelectedIndex);
				_ASSERTE(pInstance);

				m_SelectedInstanceIndex = pInstance->GetIndex();

				isDynamic = pInstance->IsDynamic();
				instScale = pInstance->GetScaling().x;
				instPos = pInstance->GetTranslation();
				instYawPitchRoll = XMFLOAT3(XMConvertToDegrees(pInstance->GetRotation().x), XMConvertToDegrees(pInstance->GetRotation().y), XMConvertToDegrees(pInstance->GetRotation().z));
			}
			else
			{
				m_SelectedInstanceIndex = 0xffffffff;
			}

			needResetGrassPatchGUI = true; // Update Grass Selected Index

			needResetInstanceGUI = false;
		}

		if (needUpdateInstance && instanceSelectedIndex >= 0)
		{
			_ASSERTE(modelSelectedIndex < m_Scene.GetModelCountInScene());
			_ASSERTE(instanceSelectedIndex < m_Scene.GetModelInstanceCount(modelSelectedIndex));

			CModelInstance* pInstance = m_Scene.GetModelInstance(modelSelectedIndex, instanceSelectedIndex);
			_ASSERTE(pInstance);

			if (instScale < 0.0001f) instScale = 0.0001f;

			pInstance->SetDynamic(isDynamic);
			pInstance->SetScaling(XMFLOAT3(instScale, instScale, instScale));
			pInstance->SetTranslation(instPos);
			pInstance->SetRotation(XMFLOAT3(XMConvertToRadians(instYawPitchRoll.x), XMConvertToRadians(instYawPitchRoll.y), XMConvertToRadians(instYawPitchRoll.z)));

			needUpdateInstance = false;
		}
	}

	// Grass
	{
		if (needAddGrassPatch)
		{
			int lastPatchIndex = grassManager.GetGrarssPatchCount() - 1;
			grassManager.AddGrassPatch(XMFLOAT3(0.0f, 0.0f, 0.0f), 1.0f);		
			if (isAddInstanceSameAsLast && lastPatchIndex >= 0)
			{	
				UINT newPatchIndex = lastPatchIndex + 1;
				grassManager.SetGrarssPatchPosition(newPatchIndex, grassManager.GetGrarssPatchPosition(lastPatchIndex));
				grassManager.SetGrarssPatchScale(newPatchIndex, grassManager.GetGrarssPatchScale(lastPatchIndex));					
			}
			needSelectedLastGrassPatch = true;
			needUpdateGrassPatchList = true;
			needAddGrassPatch = false;
		}

		if (needDelGrassPatch)
		{
			_ASSERTE(grassPatchSelectedIndex < grassManager.GetGrarssPatchCount());
			grassManager.DeleteGrassPatch(grassPatchSelectedIndex);
			needUpdateGrassPatchList = true;
			needDelGrassPatch = false;
		}

		if (needUpdateGrassPatchList)
		{
			m_CurrentGrassPatchCount = grassManager.GetGrarssPatchCount();
			m_GrassPatchNameSet.resize(m_CurrentGrassPatchCount);
			for (UINT i = 0; i < m_CurrentGrassPatchCount; i++)
			{
				m_GrassPatchNameSet[i] = "GrassPatchInst_" + std::to_string(i);
				m_GrassPatchNameList[i] = m_GrassPatchNameSet[i].data();
			}
			if (m_CurrentGrassPatchCount > 0)
			{
				if (needSelectedLastGrassPatch)
				{
					grassPatchSelectedIndex = m_CurrentGrassPatchCount - 1;
					needSelectedLastGrassPatch = false;
				}
				else
				{
					grassPatchSelectedIndex = 0;
				}
			}
			else
			{
				grassPatchSelectedIndex = -1;
			}
			needResetGrassPatchGUI = true;
			needUpdateGrassPatchList = false;
		}

		if (needResetGrassPatchGUI && grassPatchSelectedIndex >= 0)
		{
			if (grassManager.GetGrarssPatchCount() > 0)
			{
				_ASSERTE(grassPatchSelectedIndex < grassManager.GetGrarssPatchCount());
				patchScale = grassManager.GetGrarssPatchScale(grassPatchSelectedIndex);
				patchPos = grassManager.GetGrarssPatchPosition(grassPatchSelectedIndex);
				m_SelectedGrassPatchIndex = grassPatchSelectedIndex + m_Scene.GetInstanceCount();
			}
			else
			{
				m_SelectedGrassPatchIndex = 0xffffffff;
			}
		
			needResetGrassPatchGUI = false;
		}

		if (needUpdateGrassPatch && grassPatchSelectedIndex >= 0)
		{
			_ASSERTE(grassPatchSelectedIndex < grassManager.GetGrarssPatchCount());
			grassManager.SetGrarssPatchScale(grassPatchSelectedIndex, patchScale);
			grassManager.SetGrarssPatchPosition(grassPatchSelectedIndex, patchPos);
			needUpdateGrassPatch = false;
		}
	}

	if (needSaveSceneConfig)
	{
		m_Scene.SaveScene(L"scene.txt");
		
		::MessageBoxA(0, "The Scene's config is saved.", "Note", MB_OK);

		needSaveSceneConfig = false;
	}
}
