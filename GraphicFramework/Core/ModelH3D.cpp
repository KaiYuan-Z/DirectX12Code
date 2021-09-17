#include "Model.h"
#include "Utility.h"
#include "TextureLoader.h"
#include "GraphicsCore.h"
#include "GraphicsCommon.h"
#include <stdio.h>


bool CModel::_LoadH3D(const char *filename)
{
	std::string ModelFileName(filename);
	m_FileName = MakeWStr(ModelFileName);
	
	std::string FileName = GraphicsCommon::KeywordStr::H3dModelLibRoot + filename;
	FILE *file = nullptr;
    if (0 != fopen_s(&file, FileName.c_str(), "rb"))
        return false;

    bool ok = false;

    if (1 != fread(&m_Header, sizeof(SHeader), 1, file)) goto h3d_load_fail;

    m_pMesh = new SMesh [m_Header.meshCount];
    m_pMaterial = new SMaterial [m_Header.materialCount];

    if (m_Header.meshCount > 0)
        if (1 != fread(m_pMesh, sizeof(SMesh) * m_Header.meshCount, 1, file)) goto h3d_load_fail;
    if (m_Header.materialCount > 0)
        if (1 != fread(m_pMaterial, sizeof(SMaterial) * m_Header.materialCount, 1, file)) goto h3d_load_fail;

    m_VertexStride = m_pMesh[0].vertexStride;
    m_VertexStrideDepth = m_pMesh[0].vertexStrideDepth;
#if _DEBUG
    for (uint32_t meshIndex = 1; meshIndex < m_Header.meshCount; ++meshIndex)
    {
        const SMesh& mesh = m_pMesh[meshIndex];
        ASSERT(mesh.vertexStride == m_VertexStride);
        ASSERT(mesh.vertexStrideDepth == m_VertexStrideDepth);
    }
    for (uint32_t meshIndex = 0; meshIndex < m_Header.meshCount; ++meshIndex)
    {
        const SMesh& mesh = m_pMesh[meshIndex];

        ASSERT( mesh.attribsEnabled ==
            (attrib_mask_position | attrib_mask_texcoord0 | attrib_mask_normal | attrib_mask_tangent | attrib_mask_bitangent) );
        ASSERT(mesh.attrib[0].components == 3 && mesh.attrib[0].format == CModel::attrib_format_float); // position
        ASSERT(mesh.attrib[1].components == 2 && mesh.attrib[1].format == CModel::attrib_format_float); // texcoord0
        ASSERT(mesh.attrib[2].components == 3 && mesh.attrib[2].format == CModel::attrib_format_float); // normal
        ASSERT(mesh.attrib[3].components == 3 && mesh.attrib[3].format == CModel::attrib_format_float); // tangent
        ASSERT(mesh.attrib[4].components == 3 && mesh.attrib[4].format == CModel::attrib_format_float); // bitangent

        ASSERT( mesh.attribsEnabledDepth ==
            (attrib_mask_position) );
        ASSERT(mesh.attrib[0].components == 3 && mesh.attrib[0].format == CModel::attrib_format_float); // position
    }
#endif

    m_pVertexData = new unsigned char[ m_Header.vertexDataByteSize ];
    m_pIndexData = new unsigned char[ m_Header.indexDataByteSize ];
    m_pVertexDataDepth = new unsigned char[ m_Header.vertexDataByteSizeDepth ];
    m_pIndexDataDepth = new unsigned char[ m_Header.indexDataByteSize ];

    if (m_Header.vertexDataByteSize > 0)
        if (1 != fread(m_pVertexData, m_Header.vertexDataByteSize, 1, file)) goto h3d_load_fail;
    if (m_Header.indexDataByteSize > 0)
        if (1 != fread(m_pIndexData, m_Header.indexDataByteSize, 1, file)) goto h3d_load_fail;

    if (m_Header.vertexDataByteSizeDepth > 0)
        if (1 != fread(m_pVertexDataDepth, m_Header.vertexDataByteSizeDepth, 1, file)) goto h3d_load_fail;
    if (m_Header.indexDataByteSize > 0)
        if (1 != fread(m_pIndexDataDepth, m_Header.indexDataByteSize, 1, file)) goto h3d_load_fail;

	uint32_t IndexDataTypeSize = (m_Header.indexDataType == index_type_uint16) ? sizeof(uint16_t) : sizeof(uint32_t);

    m_VertexBuffer.Create(L"VertexBuffer", m_Header.vertexDataByteSize / m_VertexStride, m_VertexStride, m_pVertexData);
	m_VertexBufferView = m_VertexBuffer.CreateVertexBufferView();
    m_IndexBuffer.Create(L"IndexBuffer", m_Header.indexDataByteSize / IndexDataTypeSize, IndexDataTypeSize, m_pIndexData);
	m_IndexBufferView = m_IndexBuffer.CreateIndexBufferView();
    //delete [] m_pVertexData;
    //m_pVertexData = nullptr;
    //delete [] m_pIndexData;
    //m_pIndexData = nullptr;

    m_VertexBufferDepth.Create(L"VertexBufferDepth", m_Header.vertexDataByteSizeDepth / m_VertexStrideDepth, m_VertexStrideDepth, m_pVertexDataDepth);
	m_VertexBufferViewDepth = m_VertexBufferDepth.CreateVertexBufferView();
    m_IndexBufferDepth.Create(L"IndexBufferDepth", m_Header.indexDataByteSize / IndexDataTypeSize, IndexDataTypeSize, m_pIndexDataDepth);
	m_IndexBufferViewDepth = m_IndexBufferDepth.CreateIndexBufferView();
    //delete [] m_pVertexDataDepth;
    //m_pVertexDataDepth = nullptr;
    //delete [] m_pIndexDataDepth;
    //m_pIndexDataDepth = nullptr;

    _LoadTextures();

    ok = true;

h3d_load_fail:

    if (EOF == fclose(file))
        ok = false;

	_ComputeAllBoundingBoxes();

    return ok;
}

bool CModel::_SaveH3D(const char *filename) const
{
    FILE *file = nullptr;
    if (0 != fopen_s(&file, filename, "wb"))
        return false;

    bool ok = false;

    if (1 != fwrite(&m_Header, sizeof(SHeader), 1, file)) goto h3d_save_fail;

    if (m_Header.meshCount > 0)
        if (1 != fwrite(m_pMesh, sizeof(SMesh) * m_Header.meshCount, 1, file)) goto h3d_save_fail;
    if (m_Header.materialCount > 0)
        if (1 != fwrite(m_pMaterial, sizeof(SMaterial) * m_Header.materialCount, 1, file)) goto h3d_save_fail;

    if (m_Header.vertexDataByteSize > 0)
        if (1 != fwrite(m_pVertexData, m_Header.vertexDataByteSize, 1, file)) goto h3d_save_fail;
    if (m_Header.indexDataByteSize > 0)
        if (1 != fwrite(m_pIndexData, m_Header.indexDataByteSize, 1, file)) goto h3d_save_fail;

    if (m_Header.vertexDataByteSizeDepth > 0)
        if (1 != fwrite(m_pVertexDataDepth, m_Header.vertexDataByteSizeDepth, 1, file)) goto h3d_save_fail;
    if (m_Header.indexDataByteSize > 0)
        if (1 != fwrite(m_pIndexDataDepth, m_Header.indexDataByteSize, 1, file)) goto h3d_save_fail;

    ok = true;

h3d_save_fail:

    if (EOF == fclose(file))
        ok = false;

    return ok;
}

void CModel::_ReleaseTextureSRVs()
{ 
	if (m_SRVs) delete[] m_SRVs;
	m_SRVs = nullptr;
}

void CModel::_LoadTextures(void)
{
    _ReleaseTextureSRVs();

    m_SRVs = new D3D12_CPU_DESCRIPTOR_HANDLE[m_Header.materialCount * 6];

    const CManagedTexture* MatTextures[6] = {};

    for (uint32_t materialIdx = 0; materialIdx < m_Header.materialCount; ++materialIdx)
    {
        const SMaterial& pMaterial = m_pMaterial[materialIdx];

        // Load diffuse
        MatTextures[0] = TextureLoader::LoadFromFile(pMaterial.texDiffusePath, true);
        if (!MatTextures[0]->IsValid())
            MatTextures[0] = TextureLoader::LoadFromFile("default", true);

        // Load specular
        MatTextures[1] = TextureLoader::LoadFromFile(pMaterial.texSpecularPath, true);
        if (!MatTextures[1]->IsValid())
        {
            MatTextures[1] = TextureLoader::LoadFromFile(std::string(pMaterial.texDiffusePath) + "_specular", true);
            if (!MatTextures[1]->IsValid())
                MatTextures[1] = TextureLoader::LoadFromFile("default_specular", true);
        }

        // Load emissive
        //MatTextures[2] = TextureManager::LoadFromFile(pMaterial.texEmissivePath, true);

        // Load normal
        MatTextures[3] = TextureLoader::LoadFromFile(pMaterial.texNormalPath, false);
        if (!MatTextures[3]->IsValid())
        {
            MatTextures[3] = TextureLoader::LoadFromFile(std::string(pMaterial.texDiffusePath) + "_normal", false);
            if (!MatTextures[3]->IsValid())
                MatTextures[3] = TextureLoader::LoadFromFile("default_normal", false);
        }

        // Load lightmap
        //MatTextures[4] = TextureManager::LoadFromFile(pMaterial.texLightmapPath, true);

        // Load reflection
        //MatTextures[5] = TextureManager::LoadFromFile(pMaterial.texReflectionPath, true);

        m_SRVs[materialIdx * 6 + 0] = MatTextures[0]->GetSRV();
        m_SRVs[materialIdx * 6 + 1] = MatTextures[1]->GetSRV();
        m_SRVs[materialIdx * 6 + 2] = MatTextures[0]->GetSRV();
        m_SRVs[materialIdx * 6 + 3] = MatTextures[3]->GetSRV();
        m_SRVs[materialIdx * 6 + 4] = MatTextures[0]->GetSRV();
        m_SRVs[materialIdx * 6 + 5] = MatTextures[0]->GetSRV();
    }
}
