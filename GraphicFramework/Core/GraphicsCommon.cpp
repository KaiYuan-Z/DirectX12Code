#include "GraphicsCommon.h"


using namespace GraphicsCommon;


CSamplerDesc Sampler::SamplerLinearWrapDesc;
CSamplerDesc Sampler::SamplerAnisoWrapDesc;
CSamplerDesc Sampler::SamplerShadowDesc;
CSamplerDesc Sampler::SamplerLinearClampDesc;
CSamplerDesc Sampler::SamplerVolumeWrapDesc;
CSamplerDesc Sampler::SamplerPointClampDesc;
CSamplerDesc Sampler::SamplerPointBorderDesc;
CSamplerDesc Sampler::SamplerLinearBorderDesc;
CSamplerDesc Sampler::SamplerPointWrapDesc;
CSamplerDesc Sampler::SamplerDepthDesc;

D3D12_CPU_DESCRIPTOR_HANDLE Sampler::SamplerLinearWrap;
D3D12_CPU_DESCRIPTOR_HANDLE Sampler::SamplerAnisoWrap;
D3D12_CPU_DESCRIPTOR_HANDLE Sampler::SamplerShadow;
D3D12_CPU_DESCRIPTOR_HANDLE Sampler::SamplerLinearClamp;
D3D12_CPU_DESCRIPTOR_HANDLE Sampler::SamplerVolumeWrap;
D3D12_CPU_DESCRIPTOR_HANDLE Sampler::SamplerPointClamp;
D3D12_CPU_DESCRIPTOR_HANDLE Sampler::SamplerPointBorder;
D3D12_CPU_DESCRIPTOR_HANDLE Sampler::SamplerLinearBorder;
D3D12_CPU_DESCRIPTOR_HANDLE Sampler::SamplerPointWrap;
D3D12_CPU_DESCRIPTOR_HANDLE Sampler::SamplerDepth;

D3D12_RASTERIZER_DESC RasterizerState::RasterizerDefault;
D3D12_RASTERIZER_DESC RasterizerState::RasterizerDefaultMsaa;
D3D12_RASTERIZER_DESC RasterizerState::RasterizerDefaultCw;
D3D12_RASTERIZER_DESC RasterizerState::RasterizerDefaultCwMsaa;
D3D12_RASTERIZER_DESC RasterizerState::RasterizerTwoSided;
D3D12_RASTERIZER_DESC RasterizerState::RasterizerTwoSidedMsaa;
D3D12_RASTERIZER_DESC RasterizerState::RasterizerShadow;
D3D12_RASTERIZER_DESC RasterizerState::RasterizerShadowCW;
D3D12_RASTERIZER_DESC RasterizerState::RasterizerShadowTwoSided;

D3D12_BLEND_DESC BlendState::BlendNoColorWrite;		// XXX
D3D12_BLEND_DESC BlendState::BlendDisable;			// 1, 0
D3D12_BLEND_DESC BlendState::BlendPreMultiplied;		// 1, 1-SrcA
D3D12_BLEND_DESC BlendState::BlendTraditional;		// SrcA, 1-SrcA
D3D12_BLEND_DESC BlendState::BlendAdditive;			// 1, 1
D3D12_BLEND_DESC BlendState::BlendTraditionalAdditive;// SrcA, 1

D3D12_DEPTH_STENCIL_DESC DepthState::DepthStateDisabled;
D3D12_DEPTH_STENCIL_DESC DepthState::DepthStateReadWrite;
D3D12_DEPTH_STENCIL_DESC DepthState::DepthStateReadOnly;
D3D12_DEPTH_STENCIL_DESC DepthState::DepthStateReadOnlyReversed;
D3D12_DEPTH_STENCIL_DESC DepthState::DepthStateTestEqual;

CCommandSignature CommandSignature::DispatchIndirectCommandSignature(1);
CCommandSignature CommandSignature::DrawIndirectCommandSignature(1);


void GraphicsCommon::InitGraphicsCommon(void)
{
	Sampler::SamplerLinearWrapDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	Sampler::SamplerLinearWrap = Sampler::SamplerLinearWrapDesc.CreateDescriptor();

	Sampler::SamplerAnisoWrapDesc.MaxAnisotropy = 4;
	Sampler::SamplerAnisoWrap = Sampler::SamplerAnisoWrapDesc.CreateDescriptor();

	Sampler::SamplerShadowDesc.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	Sampler::SamplerShadowDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	Sampler::SamplerShadowDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	Sampler::SamplerShadow = Sampler::SamplerShadowDesc.CreateDescriptor();

	Sampler::SamplerLinearClampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	Sampler::SamplerLinearClampDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	Sampler::SamplerLinearClamp = Sampler::SamplerLinearClampDesc.CreateDescriptor();

	Sampler::SamplerVolumeWrapDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	Sampler::SamplerVolumeWrap = Sampler::SamplerVolumeWrapDesc.CreateDescriptor();

	Sampler::SamplerPointClampDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	Sampler::SamplerPointClampDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	Sampler::SamplerPointClamp = Sampler::SamplerPointClampDesc.CreateDescriptor();

	Sampler::SamplerLinearBorderDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	Sampler::SamplerLinearBorderDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_BORDER);
	Sampler::SamplerLinearBorderDesc.SetBorderColor(CColor(0.0f, 0.0f, 0.0f, 0.0f));
	Sampler::SamplerLinearBorder = Sampler::SamplerLinearBorderDesc.CreateDescriptor();

	Sampler::SamplerPointBorderDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	Sampler::SamplerPointBorderDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_BORDER);
	Sampler::SamplerPointBorderDesc.SetBorderColor(CColor(0.0f, 0.0f, 0.0f, 0.0f));
	Sampler::SamplerPointBorder = Sampler::SamplerPointBorderDesc.CreateDescriptor();

	Sampler::SamplerPointWrapDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	Sampler::SamplerPointWrap = Sampler::SamplerPointWrapDesc.CreateDescriptor();

	Sampler::SamplerDepthDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	Sampler::SamplerDepthDesc.SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE_BORDER);
	Sampler::SamplerDepthDesc.SetBorderColor(CColor(1.0f, 1.0f, 1.0f, 1.0f));
	Sampler::SamplerDepth = Sampler::SamplerDepthDesc.CreateDescriptor();

    // Default rasterizer states
	RasterizerState::RasterizerDefault.FillMode = D3D12_FILL_MODE_SOLID;
	RasterizerState::RasterizerDefault.CullMode = D3D12_CULL_MODE_BACK;
	RasterizerState::RasterizerDefault.FrontCounterClockwise = TRUE;
	RasterizerState::RasterizerDefault.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	RasterizerState::RasterizerDefault.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	RasterizerState::RasterizerDefault.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	RasterizerState::RasterizerDefault.DepthClipEnable = TRUE;
	RasterizerState::RasterizerDefault.MultisampleEnable = FALSE;
	RasterizerState::RasterizerDefault.AntialiasedLineEnable = FALSE;
	RasterizerState::RasterizerDefault.ForcedSampleCount = 0;
	RasterizerState::RasterizerDefault.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	RasterizerState::RasterizerDefaultMsaa = RasterizerState::RasterizerDefault;
	RasterizerState::RasterizerDefaultMsaa.MultisampleEnable = TRUE;

	RasterizerState::RasterizerDefaultCw = RasterizerState::RasterizerDefault;
	RasterizerState::RasterizerDefaultCw.FrontCounterClockwise = FALSE;

	RasterizerState::RasterizerDefaultCwMsaa = RasterizerState::RasterizerDefaultCw;
	RasterizerState::RasterizerDefaultCwMsaa.MultisampleEnable = TRUE;

	RasterizerState::RasterizerTwoSided = RasterizerState::RasterizerDefault;
	RasterizerState::RasterizerTwoSided.CullMode = D3D12_CULL_MODE_NONE;

	RasterizerState::RasterizerTwoSidedMsaa = RasterizerState::RasterizerTwoSided;
	RasterizerState::RasterizerTwoSidedMsaa.MultisampleEnable = TRUE;

    // Shadows need their own rasterizer state so we can reverse the winding of faces
	RasterizerState::RasterizerShadow = RasterizerState::RasterizerDefault;
    //RasterizerShadow.CullMode = D3D12_CULL_FRONT;  // Hacked here rather than fixing the content
	RasterizerState::RasterizerShadow.SlopeScaledDepthBias = -1.5f;
	RasterizerState::RasterizerShadow.DepthBias = -100;

	RasterizerState::RasterizerShadowTwoSided = RasterizerState::RasterizerShadow;
	RasterizerState::RasterizerShadowTwoSided.CullMode = D3D12_CULL_MODE_NONE;

	RasterizerState::RasterizerShadowCW = RasterizerState::RasterizerShadow;
	RasterizerState::RasterizerShadowCW.FrontCounterClockwise = FALSE;

	DepthState::DepthStateDisabled.DepthEnable = FALSE;
	DepthState::DepthStateDisabled.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	DepthState::DepthStateDisabled.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	DepthState::DepthStateDisabled.StencilEnable = FALSE;
	DepthState::DepthStateDisabled.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	DepthState::DepthStateDisabled.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	DepthState::DepthStateDisabled.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	DepthState::DepthStateDisabled.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	DepthState::DepthStateDisabled.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	DepthState::DepthStateDisabled.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	DepthState::DepthStateDisabled.BackFace = DepthState::DepthStateDisabled.FrontFace;

	DepthState::DepthStateReadWrite = DepthState::DepthStateDisabled;
	DepthState::DepthStateReadWrite.DepthEnable = TRUE;
	DepthState::DepthStateReadWrite.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	DepthState::DepthStateReadWrite.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;

	DepthState::DepthStateReadOnly = DepthState::DepthStateReadWrite;
	DepthState::DepthStateReadOnly.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

	DepthState::DepthStateReadOnlyReversed = DepthState::DepthStateReadOnly;
	DepthState::DepthStateReadOnlyReversed.DepthFunc = D3D12_COMPARISON_FUNC_LESS;

	DepthState::DepthStateTestEqual = DepthState::DepthStateReadOnly;
	DepthState::DepthStateTestEqual.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;

    D3D12_BLEND_DESC alphaBlend = {};
    alphaBlend.IndependentBlendEnable = FALSE;
    alphaBlend.RenderTarget[0].BlendEnable = FALSE;
    alphaBlend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    alphaBlend.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    alphaBlend.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    alphaBlend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    alphaBlend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    alphaBlend.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    alphaBlend.RenderTarget[0].RenderTargetWriteMask = 0;
	BlendState::BlendNoColorWrite = alphaBlend;

    alphaBlend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	BlendState::BlendDisable = alphaBlend;

    alphaBlend.RenderTarget[0].BlendEnable = TRUE;
	BlendState::BlendTraditional = alphaBlend;

    alphaBlend.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	BlendState::BlendPreMultiplied = alphaBlend;

    alphaBlend.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	BlendState::BlendAdditive = alphaBlend;

    alphaBlend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	BlendState::BlendTraditionalAdditive = alphaBlend;

	CommandSignature::DispatchIndirectCommandSignature[0].Dispatch();
	CommandSignature::DispatchIndirectCommandSignature.Finalize();

	CommandSignature::DrawIndirectCommandSignature[0].Draw();
	CommandSignature::DrawIndirectCommandSignature.Finalize();
}

void GraphicsCommon::DestroyGraphicsCommon(void)
{
	CommandSignature::DispatchIndirectCommandSignature.Destroy();
	CommandSignature::DrawIndirectCommandSignature.Destroy();
}