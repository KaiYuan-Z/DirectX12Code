#include "GBufferEffect.h"
#include "../Core/GraphicsCommon.h"

#include "CompiledShaders/GBufferEffectPS.h"
#include "CompiledShaders/GBufferEffectVS.h"

CGBufferEffect::CGBufferEffect()
{
	m_FirstRootParamIndex = m_GBufferRender.GetRootParameterCount();
}


CGBufferEffect::~CGBufferEffect()
{
	__ReleaseBuffers();
}

void CGBufferEffect::Init(const SGBufferConfig& config)
{
	_ASSERTE(config.Width > 0 && config.Height > 0);

	m_GBufferConfig = config;

	m_ViewPort.TopLeftX = m_ViewPort.TopLeftY = 0;
	m_ViewPort.Width = (float)config.Width;
	m_ViewPort.Height = (float)config.Height;
	m_ViewPort.MinDepth = D3D12_MIN_DEPTH;
	m_ViewPort.MaxDepth = D3D12_MAX_DEPTH;
	m_ScissorRect.left = (LONG)m_ViewPort.TopLeftX;
	m_ScissorRect.top = (LONG)m_ViewPort.TopLeftY;
	m_ScissorRect.right = (LONG)m_ViewPort.TopLeftX + (LONG)m_ViewPort.Width;
	m_ScissorRect.bottom = (LONG)m_ViewPort.TopLeftY + (LONG)m_ViewPort.Height;

	m_GBufferRender.InitRender(config.SceneRenderConfig);

	__InitBuffers();
	__InitRootRootSignature();
	__InitPSO();
}

void CGBufferEffect::Render(CGraphicsCommandList* pCommandList, GraphicsCommon::EShaderingType Type)
{
	_ASSERTE(pCommandList);

	std::swap(m_pCurPosBuffer, m_pPrePosBuffer);
	std::swap(m_pCurNormBuffer, m_pPreNormBuffer);
	std::swap(m_pCurDiffBuffer, m_pPreDiffBuffer);
	std::swap(m_pCurSpecBuffer, m_pPreSpecBuffer);
	std::swap(m_pCurMotionBuffer, m_pPreMotionBuffer);
	std::swap(m_pCurDdxyBuffer, m_pPreDdxyBuffer);
	std::swap(m_pCurLinearDepthBuffer, m_pPreLinearDepthBuffer);
	std::swap(m_pCurIdBuffer, m_pPreIdBuffer);
	std::swap(m_pCurDepthBuffer, m_pPreDepthBuffer);

	pCommandList->TransitionResource(*m_pCurPosBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, false);
	pCommandList->TransitionResource(*m_pCurNormBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, false);
	pCommandList->TransitionResource(*m_pCurDiffBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, false);
	pCommandList->TransitionResource(*m_pCurSpecBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, false);
	pCommandList->TransitionResource(*m_pCurMotionBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, false);
	pCommandList->TransitionResource(*m_pCurDdxyBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, false);
	pCommandList->TransitionResource(*m_pCurLinearDepthBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, false);
	pCommandList->TransitionResource(*m_pCurIdBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, false);
	pCommandList->TransitionResource(*m_pCurDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);

	pCommandList->ClearColor(*m_pCurPosBuffer);
	pCommandList->ClearColor(*m_pCurNormBuffer);
	pCommandList->ClearColor(*m_pCurDiffBuffer);
	pCommandList->ClearColor(*m_pCurSpecBuffer);
	pCommandList->ClearColor(*m_pCurMotionBuffer);
	pCommandList->ClearColor(*m_pCurDdxyBuffer);
	pCommandList->ClearColor(*m_pCurLinearDepthBuffer);
	pCommandList->ClearColor(*m_pCurIdBuffer);
	pCommandList->ClearDepthAndStencil(*m_pCurDepthBuffer);

	pCommandList->SetViewportAndScissor(m_ViewPort, m_ScissorRect);
	pCommandList->SetRootSignature(m_RootSignature);
	pCommandList->SetPipelineState(m_PSO);

	D3D12_CPU_DESCRIPTOR_HANDLE RTVs[8] = 
	{   m_pCurPosBuffer->GetRTV(),
		m_pCurDiffBuffer->GetRTV(),
		m_pCurSpecBuffer->GetRTV(), 
		m_pCurNormBuffer->GetRTV(),
		m_pCurMotionBuffer->GetRTV(),
		m_pCurDdxyBuffer->GetRTV(),
		m_pCurLinearDepthBuffer->GetRTV(),
		m_pCurIdBuffer->GetRTV() };
	pCommandList->SetRenderTargets(8, RTVs, m_pCurDepthBuffer->GetDSV());

	bool EnablePRB = (Type == GraphicsCommon::EShaderingType::kPbr);
	pCommandList->SetConstant(m_FirstRootParamIndex, EnablePRB, 0);

	m_GBufferRender.DrawScene(pCommandList);
}

void CGBufferEffect::__InitBuffers()
{
	m_pPrePosBuffer = new CColorBuffer(); _ASSERTE(m_pPrePosBuffer);
	m_pPrePosBuffer->Create2D(L"GBuffer-PosBuffer0", m_GBufferConfig.Width, m_GBufferConfig.Height, 1, DXGI_FORMAT_R32G32B32A32_FLOAT);
	m_pPreDiffBuffer = new CColorBuffer(); _ASSERTE(m_pPreDiffBuffer);
	m_pPreDiffBuffer->Create2D(L"GBuffer-DiffBuffer0", m_GBufferConfig.Width, m_GBufferConfig.Height, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
	m_pPreSpecBuffer = new CColorBuffer(); _ASSERTE(m_pPreSpecBuffer);
	m_pPreSpecBuffer->Create2D(L"GBuffer-SpecBuffer0", m_GBufferConfig.Width, m_GBufferConfig.Height, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
	m_pPreNormBuffer = new CColorBuffer(); _ASSERTE(m_pPreNormBuffer);
	m_pPreNormBuffer->Create2D(L"GBuffer-NormBuffer0", m_GBufferConfig.Width, m_GBufferConfig.Height, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
	m_pPreMotionBuffer = new CColorBuffer(); _ASSERTE(m_pPreMotionBuffer);
	m_pPreMotionBuffer->Create2D(L"GBuffer-MotionBuffer0", m_GBufferConfig.Width, m_GBufferConfig.Height, 1, DXGI_FORMAT_R16G16_FLOAT);
	m_pPreDdxyBuffer = new CColorBuffer(); _ASSERTE(m_pPreDdxyBuffer);
	m_pPreDdxyBuffer->Create2D(L"GBuffer-DdxyBuffer0", m_GBufferConfig.Width, m_GBufferConfig.Height, 1, DXGI_FORMAT_R16G16_FLOAT);
	m_pPreLinearDepthBuffer = new CColorBuffer(); _ASSERTE(m_pPreLinearDepthBuffer);
	m_pPreLinearDepthBuffer->Create2D(L"GBuffer-LinearDepthBuffer0", m_GBufferConfig.Width, m_GBufferConfig.Height, 1, DXGI_FORMAT_R32G32_FLOAT);
	m_pPreIdBuffer = new CColorBuffer(CColor(-1.0f, -1.0f, -1.0f, -1.0f)); _ASSERTE(m_pPreIdBuffer);
	m_pPreIdBuffer->Create2D(L"GBuffer-IdBuffer0", m_GBufferConfig.Width, m_GBufferConfig.Height, 1, DXGI_FORMAT_R32G32B32A32_SINT);

	m_pCurPosBuffer = new CColorBuffer(); _ASSERTE(m_pCurPosBuffer);
	m_pCurPosBuffer->Create2D(L"GBuffer-PosBuffer1", m_GBufferConfig.Width, m_GBufferConfig.Height, 1, DXGI_FORMAT_R32G32B32A32_FLOAT);
	m_pCurDiffBuffer = new CColorBuffer(); _ASSERTE(m_pCurDiffBuffer);
	m_pCurDiffBuffer->Create2D(L"GBuffer-DiffBuffer1", m_GBufferConfig.Width, m_GBufferConfig.Height, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
	m_pCurSpecBuffer = new CColorBuffer(); _ASSERTE(m_pCurSpecBuffer);
	m_pCurSpecBuffer->Create2D(L"GBuffer-SpecBuffer1", m_GBufferConfig.Width, m_GBufferConfig.Height, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
	m_pCurNormBuffer = new CColorBuffer(); _ASSERTE(m_pCurNormBuffer);
	m_pCurNormBuffer->Create2D(L"GBuffer-NormBuffer1", m_GBufferConfig.Width, m_GBufferConfig.Height, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
	m_pCurMotionBuffer = new CColorBuffer(); _ASSERTE(m_pCurMotionBuffer);
	m_pCurMotionBuffer->Create2D(L"GBuffer-MotionBuffer1", m_GBufferConfig.Width, m_GBufferConfig.Height, 1, DXGI_FORMAT_R16G16_FLOAT);
	m_pCurDdxyBuffer = new CColorBuffer(); _ASSERTE(m_pCurDdxyBuffer);
	m_pCurDdxyBuffer->Create2D(L"GBuffer-DdxyBuffer1", m_GBufferConfig.Width, m_GBufferConfig.Height, 1, DXGI_FORMAT_R16G16_FLOAT);
	m_pCurLinearDepthBuffer = new CColorBuffer(); _ASSERTE(m_pCurLinearDepthBuffer);
	m_pCurLinearDepthBuffer->Create2D(L"GBuffer-LinearDepthBuffer1", m_GBufferConfig.Width, m_GBufferConfig.Height, 1, DXGI_FORMAT_R32G32_FLOAT);
	m_pCurIdBuffer = new CColorBuffer(CColor(-1.0f, -1.0f, -1.0f, -1.0f)); _ASSERTE(m_pCurIdBuffer);
	m_pCurIdBuffer->Create2D(L"GBuffer-IdBuffer1", m_GBufferConfig.Width, m_GBufferConfig.Height, 1, DXGI_FORMAT_R32G32B32A32_SINT);

	m_pPreDepthBuffer = new CDepthBuffer(); _ASSERTE(m_pPreDepthBuffer);
	m_pPreDepthBuffer->Create2D(L"GBuffer-Depth0", m_GBufferConfig.Width, m_GBufferConfig.Height, m_GBufferConfig.DepthStencilBufferFormat);
	m_pPreDepthBuffer->SetClearDepth(1.0f);
	m_pPreDepthBuffer->SetClearStencil(0);

	m_pCurDepthBuffer = new CDepthBuffer(); _ASSERTE(m_pPreDepthBuffer);
	m_pCurDepthBuffer->Create2D(L"GBuffer-Depth1", m_GBufferConfig.Width, m_GBufferConfig.Height, m_GBufferConfig.DepthStencilBufferFormat);
	m_pCurDepthBuffer->SetClearDepth(1.0f);
	m_pCurDepthBuffer->SetClearStencil(0);
}

void CGBufferEffect::__InitRootRootSignature()
{
	m_RootSignature.Reset(m_GBufferRender.GetRootParameterCount() + 1, m_GBufferRender.GetStaticSamplerCount());
	m_GBufferRender.ConfigurateRootSignature(m_RootSignature);
	m_RootSignature[m_FirstRootParamIndex].InitAsConstants(0, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	m_RootSignature.Finalize(L"GBufferEffect", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
}

void CGBufferEffect::__InitPSO()
{
	m_PSO.SetInputLayout((UINT)GraphicsCommon::InputLayout::PosTexNormTanBitan.size(), GraphicsCommon::InputLayout::PosTexNormTanBitan.data());
	m_PSO.SetRootSignature(m_RootSignature);
	m_PSO.SetVertexShader(g_pGBufferEffectVS,sizeof(g_pGBufferEffectVS));
	m_PSO.SetPixelShader(g_pGBufferEffectPS, sizeof(g_pGBufferEffectPS));
	auto RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	RasterizerState.CullMode =  m_GBufferConfig.OpenBackFaceCulling ?  D3D12_CULL_MODE::D3D12_CULL_MODE_BACK : D3D12_CULL_MODE::D3D12_CULL_MODE_NONE;
	m_PSO.SetRasterizerState(RasterizerState);
	m_PSO.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
	m_PSO.SetDepthStencilState(CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT));
	m_PSO.SetSampleMask(UINT_MAX);
	m_PSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	DXGI_FORMAT RenderTargetFormats[8] = 
	{   DXGI_FORMAT_R32G32B32A32_FLOAT, 
		DXGI_FORMAT_R16G16B16A16_FLOAT, 
		DXGI_FORMAT_R16G16B16A16_FLOAT, 
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		DXGI_FORMAT_R16G16_FLOAT,
		DXGI_FORMAT_R16G16_FLOAT,
		DXGI_FORMAT_R32G32_FLOAT,
		DXGI_FORMAT_R32G32B32A32_SINT };
	m_PSO.SetRenderTargetFormats(8, RenderTargetFormats, m_GBufferConfig.DepthStencilBufferFormat);
	m_PSO.Finalize();
}

void CGBufferEffect::__ReleaseBuffers()
{	
	SAFE_DELETE(m_pPrePosBuffer);
	SAFE_DELETE(m_pPreNormBuffer);
	SAFE_DELETE(m_pPreDiffBuffer);
	SAFE_DELETE(m_pPreSpecBuffer);
	SAFE_DELETE(m_pPreMotionBuffer);
	SAFE_DELETE(m_pPreDdxyBuffer);
	SAFE_DELETE(m_pPreLinearDepthBuffer);
	SAFE_DELETE(m_pPreIdBuffer);
	SAFE_DELETE(m_pCurPosBuffer);
	SAFE_DELETE(m_pCurNormBuffer);
	SAFE_DELETE(m_pCurDiffBuffer);
	SAFE_DELETE(m_pCurSpecBuffer);
	SAFE_DELETE(m_pCurMotionBuffer);
	SAFE_DELETE(m_pCurDdxyBuffer);
	SAFE_DELETE(m_pCurLinearDepthBuffer);
	SAFE_DELETE(m_pCurIdBuffer);
	SAFE_DELETE(m_pPreDepthBuffer);
	SAFE_DELETE(m_pCurDepthBuffer);
}
