#pragma once
#include "Utility.h"
#include "SamplerDesc.h"
#include "CommandSignature.h"

namespace GraphicsCommon
{
	enum EShaderingType
	{
		kBlinnPhong,
		kPbr,
		kRaytracing
	};
	
	namespace KeywordStr
	{
		const std::string H3dModelLibRoot = "../../Resources/Models/";
		const std::string TexLibRoot = "../../Resources/Textures/"; 
		const std::string ModelTexFolderName = "Models/";
	}

	namespace KeywordWStr
	{
		const std::wstring TexLibRoot = MakeWStr(KeywordStr::TexLibRoot);
		const std::wstring ModelTexFolderName = MakeWStr(KeywordStr::ModelTexFolderName);
	}

	namespace Vertex
	{
		struct SPosTex
		{
			XMFLOAT3 Pos;
			XMFLOAT2 Tex;
		};

		struct SPosColor
		{
			XMFLOAT3 Pos;
			XMFLOAT4 Color;
		};

		struct PosTexNormTanBitan
		{
			XMFLOAT3 Pos;
			XMFLOAT2 Tex;
			XMFLOAT3 Norm;
			XMFLOAT3 Tan;
			XMFLOAT3 Bitan;
		};
	}

	namespace InputLayout
	{
		const std::vector<D3D12_INPUT_ELEMENT_DESC> Pos =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		const std::vector<D3D12_INPUT_ELEMENT_DESC> PosTex =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		const std::vector<D3D12_INPUT_ELEMENT_DESC> PosColor =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		const std::vector<D3D12_INPUT_ELEMENT_DESC> PosTexNormTanBitan =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};
	}
	
	namespace Sampler
	{
		extern CSamplerDesc SamplerLinearWrapDesc;
		extern CSamplerDesc SamplerAnisoWrapDesc;
		extern CSamplerDesc SamplerShadowDesc;
		extern CSamplerDesc SamplerLinearClampDesc;
		extern CSamplerDesc SamplerVolumeWrapDesc;
		extern CSamplerDesc SamplerPointClampDesc;
		extern CSamplerDesc SamplerPointBorderDesc;
		extern CSamplerDesc SamplerLinearBorderDesc;
		extern CSamplerDesc SamplerPointWrapDesc;
		extern CSamplerDesc SamplerDepthDesc;

		extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerLinearWrap;
		extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerAnisoWrap;
		extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerShadow;
		extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerLinearClamp;
		extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerVolumeWrap;
		extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerPointClamp;
		extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerPointBorder;
		extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerLinearBorder;
		extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerPointWrap;
		extern D3D12_CPU_DESCRIPTOR_HANDLE SamplerDepth;
	}

	namespace RasterizerState
	{
		extern D3D12_RASTERIZER_DESC RasterizerDefault;
		extern D3D12_RASTERIZER_DESC RasterizerDefaultMsaa;
		extern D3D12_RASTERIZER_DESC RasterizerDefaultCw;
		extern D3D12_RASTERIZER_DESC RasterizerDefaultCwMsaa;
		extern D3D12_RASTERIZER_DESC RasterizerTwoSided;
		extern D3D12_RASTERIZER_DESC RasterizerTwoSidedMsaa;
		extern D3D12_RASTERIZER_DESC RasterizerShadow;
		extern D3D12_RASTERIZER_DESC RasterizerShadowCW;
		extern D3D12_RASTERIZER_DESC RasterizerShadowTwoSided;
	}

	namespace BlendState
	{
		extern D3D12_BLEND_DESC BlendNoColorWrite;		// XXX
		extern D3D12_BLEND_DESC BlendDisable;			// 1, 0
		extern D3D12_BLEND_DESC BlendPreMultiplied;		// 1, 1-SrcA
		extern D3D12_BLEND_DESC BlendTraditional;		// SrcA, 1-SrcA
		extern D3D12_BLEND_DESC BlendAdditive;			// 1, 1
		extern D3D12_BLEND_DESC BlendTraditionalAdditive;// SrcA, 1
	}
 
	namespace DepthState
	{
		extern D3D12_DEPTH_STENCIL_DESC DepthStateDisabled;
		extern D3D12_DEPTH_STENCIL_DESC DepthStateReadWrite;
		extern D3D12_DEPTH_STENCIL_DESC DepthStateReadOnly;
		extern D3D12_DEPTH_STENCIL_DESC DepthStateReadOnlyReversed;
		extern D3D12_DEPTH_STENCIL_DESC DepthStateTestEqual;
	}
 
	namespace CommandSignature
	{
		extern CCommandSignature DispatchIndirectCommandSignature;
		extern CCommandSignature DrawIndirectCommandSignature;
	}

    void InitGraphicsCommon(void);
    void DestroyGraphicsCommon(void);
}
