#pragma once

#include "MathUtility.h"
#include "BoundingObject.h"
#include "TextureLoader.h"
#include "StructuredBuffer.h"
#include "ByteAddressBuffer.h"

#define MODEL_TYPE_UNKNOWN	0
#define MODEL_TYPE_BASIC	1
#define MODEL_TYPE_SDF		2


class CModelSDF;

class CModel
{
public:

    enum
    {
        attrib_mask_0 = (1 << 0),
        attrib_mask_1 = (1 << 1),
        attrib_mask_2 = (1 << 2),
        attrib_mask_3 = (1 << 3),
        attrib_mask_4 = (1 << 4),
        attrib_mask_5 = (1 << 5),
        attrib_mask_6 = (1 << 6),
        attrib_mask_7 = (1 << 7),
        attrib_mask_8 = (1 << 8),
        attrib_mask_9 = (1 << 9),
        attrib_mask_10 = (1 << 10),
        attrib_mask_11 = (1 << 11),
        attrib_mask_12 = (1 << 12),
        attrib_mask_13 = (1 << 13),
        attrib_mask_14 = (1 << 14),
        attrib_mask_15 = (1 << 15),

        // friendly name aliases
        attrib_mask_position = attrib_mask_0,
        attrib_mask_texcoord0 = attrib_mask_1,
        attrib_mask_normal = attrib_mask_2,
        attrib_mask_tangent = attrib_mask_3,
        attrib_mask_bitangent = attrib_mask_4,
    };

    enum
    {
        attrib_0 = 0,
        attrib_1 = 1,
        attrib_2 = 2,
        attrib_3 = 3,
        attrib_4 = 4,
        attrib_5 = 5,
        attrib_6 = 6,
        attrib_7 = 7,
        attrib_8 = 8,
        attrib_9 = 9,
        attrib_10 = 10,
        attrib_11 = 11,
        attrib_12 = 12,
        attrib_13 = 13,
        attrib_14 = 14,
        attrib_15 = 15,

        // friendly name aliases
        attrib_position = attrib_0,
        attrib_texcoord0 = attrib_1,
        attrib_normal = attrib_2,
        attrib_tangent = attrib_3,
        attrib_bitangent = attrib_4,

        maxAttribs = 16
    };

    enum
    {
        attrib_format_none = 0,
        attrib_format_ubyte,
        attrib_format_byte,
        attrib_format_ushort,
        attrib_format_short,
        attrib_format_float,

        attrib_formats
    };

	enum EIndexDataType
	{
		index_type_uint16,
		index_type_uint32
	};

	enum ETextureOffset
	{
		kDiffuseOffset = 0,
		kSpecularOffset,
		kEmissiveOffset,
		kNormalOffset,
		kLightmapOffset,
		kReflectionOffset
	};

    struct SHeader
    {
		uint32_t meshCount = 0;
        uint32_t materialCount = 0;
        uint32_t vertexDataByteSize = 0;
        uint32_t indexDataByteSize = 0;
        uint32_t vertexDataByteSizeDepth = 0;
		EIndexDataType indexDataType = index_type_uint16;
		SBoundingObject boundingObject;
    };

	struct SAttrib
	{
		uint16_t offset = 0; // byte offset from the start of the vertex
		uint16_t normalized = 0; // if true, integer formats are interpreted as [-1, 1] or [0, 1]
		uint16_t components = 0; // 1-4
		uint16_t format = 0;
	};

	struct SMesh
    {
		SBoundingObject boundingObject;

        unsigned int materialIndex = 0;

        unsigned int attribsEnabled = 0;
        unsigned int attribsEnabledDepth = 0;
        unsigned int vertexStride = 0;
        unsigned int vertexStrideDepth = 0;
        SAttrib attrib[maxAttribs];
        SAttrib attribDepth[maxAttribs];

        unsigned int vertexDataByteOffset = 0;
        unsigned int vertexCount = 0;
        unsigned int indexDataByteOffset = 0;
        unsigned int indexCount = 0;

        unsigned int vertexDataByteOffsetDepth = 0;
        unsigned int vertexCountDepth = 0;
    };
   
    struct SMaterial
    {
        XMFLOAT3 diffuse = XMFLOAT3(0, 0, 0);
		XMFLOAT3 specular = XMFLOAT3(0, 0, 0);
		XMFLOAT3 ambient = XMFLOAT3(0, 0, 0);
		XMFLOAT3 emissive = XMFLOAT3(0, 0, 0);
		XMFLOAT3 transparent = XMFLOAT3(0, 0, 0); // light passing through a transparent surface is multiplied by this filter color
        float opacity = 0;
        float shininess = 0; // specular exponent
        float specularStrength = 0; // multiplier on top of specular color

        enum {maxTexPath = 128};
        enum {texCount = 6};
        char texDiffusePath[maxTexPath];
        char texSpecularPath[maxTexPath];
        char texEmissivePath[maxTexPath];
        char texNormalPath[maxTexPath];
        char texLightmapPath[maxTexPath];
        char texReflectionPath[maxTexPath];

        enum {maxMaterialName = 128};
        char name[maxMaterialName];
    };
    
	CModel();
	CModel(const std::wstring& modelName);
	virtual ~CModel();

	static CModel* CreateModel(const std::wstring& modelName, UINT modelType);

	void Clear();

	virtual bool Load(const char* filename) { return _LoadH3D(filename); }

	const SBoundingObject& GetBoundingObject() const { return m_Header.boundingObject; }
	const D3D12_CPU_DESCRIPTOR_HANDLE* GetSRVs(uint32_t materialIdx) const { return m_SRVs + materialIdx * 6; }
	const SMesh& GetMesh(uint32_t meshIdx) const { _ASSERTE(&m_pMesh[meshIdx]); return m_pMesh[meshIdx]; }
	const SMaterial& GetMaterial(uint32_t materialIdx) const { _ASSERTE(&m_pMaterial[materialIdx]); return m_pMaterial[materialIdx]; }
	const unsigned char* GetVertexDataCpuCopy() const { return m_pVertexData; }
	const unsigned char* GetIndexDataCpuCopy() const { return m_pIndexData; }
	const CStructuredBuffer& GetVertexBuffer() const { return m_VertexBuffer; }
	const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const { return m_VertexBufferView; }
	const CByteAddressBuffer& GetIndexBuffer() const { return m_IndexBuffer; }
	const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const { return m_IndexBufferView; }
	uint32_t GetVertexStride() const { return m_VertexStride; }
	const unsigned char* GetVertexDataDepthCpuCopy() const { return m_pVertexDataDepth; }
	const unsigned char* GetIndexDataDepthCpuCopy() const { return m_pIndexDataDepth; }
	const CStructuredBuffer& GetVertexBufferDepth() const { return m_VertexBufferDepth; }
	const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferViewDepth() const { return m_VertexBufferViewDepth; }
	const CByteAddressBuffer& GetIndexBufferDepth() const { return m_IndexBufferDepth; }
	const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferViewDepth() const { return m_IndexBufferViewDepth; }
	uint32_t GetVertexStrideDepth() const { return m_VertexStrideDepth; }
	const SHeader& GetHeader() const { return m_Header; }
	const uint32_t GetIndexDataTypeSize() const { return (m_Header.indexDataType == index_type_uint16) ? sizeof(uint16_t) : sizeof(uint32_t); }
	const std::wstring& GetModelName() const { return m_ModelName; }
	const std::wstring& GetModelFileName() const { return m_FileName; }
	const UINT GetModelType() const { return m_ModelType; }
	CModelSDF* QuerySdfModel();

protected:

	bool _LoadH3D(const char *filename);
	bool _SaveH3D(const char *filename) const;

	void _ComputeMeshBoundingBox(unsigned int meshIndex, SBoundingObject& boundingObject) const;
	void _ComputeGlobalBoundingBox(SBoundingObject& boundingObject) const;
	void _ComputeAllBoundingBoxes();

    void _ReleaseTextureSRVs();
    void _LoadTextures();

	std::wstring m_ModelName;
	std::wstring m_FileName;

	UINT m_ModelType;

	SHeader m_Header;

	SMesh* m_pMesh;

	SMaterial* m_pMaterial;

	// for general rendering(use "PosTexNormTanBitan" input layout)
	unsigned char* m_pVertexData;
	unsigned char* m_pIndexData;
	CStructuredBuffer m_VertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
	CByteAddressBuffer m_IndexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;
	uint32_t m_VertexStride;

	// optimized for depth-only rendering(use "Pos" input layout.), depth vertex count
	// usually less than vertex count, because shadow does not need too much details.
	unsigned char* m_pVertexDataDepth;
	unsigned char* m_pIndexDataDepth;
	CStructuredBuffer m_VertexBufferDepth;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferViewDepth;
	CByteAddressBuffer m_IndexBufferDepth;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferViewDepth;
	uint32_t m_VertexStrideDepth;

    D3D12_CPU_DESCRIPTOR_HANDLE* m_SRVs;
};
