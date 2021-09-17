#include "Model.h"
#include <string.h>
#include <float.h>

#include "ModelSDF.h"

CModel::CModel()
    : m_ModelName(L"")
	, m_ModelType(MODEL_TYPE_BASIC)
	, m_pMesh(nullptr)
    , m_pMaterial(nullptr)
    , m_pVertexData(nullptr)
    , m_pIndexData(nullptr)
    , m_pVertexDataDepth(nullptr)
    , m_pIndexDataDepth(nullptr)
    , m_SRVs(nullptr)
{
    Clear();
}

CModel::CModel(const std::wstring& modelName)
	: m_ModelName(modelName)
	, m_ModelType(MODEL_TYPE_BASIC)
	, m_pMesh(nullptr)
	, m_pMaterial(nullptr)
	, m_pVertexData(nullptr)
	, m_pIndexData(nullptr)
	, m_pVertexDataDepth(nullptr)
	, m_pIndexDataDepth(nullptr)
	, m_SRVs(nullptr)
{
}

CModel::~CModel()
{
    Clear();
}

CModel* CModel::CreateModel(const std::wstring& modelName, UINT modelType)
{
	CModel* pModel = nullptr;
	switch (modelType)
	{
	case MODEL_TYPE_BASIC: pModel = new CModel(modelName); break;
	case MODEL_TYPE_SDF: pModel = new CModelSDF(modelName); break;
	default:_ASSERT(false);
	}
	_ASSERTE(pModel);
	return pModel;
}

CModelSDF* CModel::QuerySdfModel()
{
	_ASSERTE(m_ModelType == MODEL_TYPE_SDF);
	return static_cast<CModelSDF*>(this);
}

void CModel::Clear()
{
    m_VertexBuffer.Destroy();
    m_IndexBuffer.Destroy();
    m_VertexBufferDepth.Destroy();
    m_IndexBufferDepth.Destroy();

    delete [] m_pMesh;
    m_pMesh = nullptr;
    m_Header.meshCount = 0;

    delete [] m_pMaterial;
    m_pMaterial = nullptr;
    m_Header.materialCount = 0;

    delete [] m_pVertexData;
    delete [] m_pIndexData;
    delete [] m_pVertexDataDepth;
    delete [] m_pIndexDataDepth;

    m_pVertexData = nullptr;
    m_Header.vertexDataByteSize = 0;
    m_pIndexData = nullptr;
    m_Header.indexDataByteSize = 0;
    m_pVertexDataDepth = nullptr;
    m_Header.vertexDataByteSizeDepth = 0;
    m_pIndexDataDepth = nullptr;

    _ReleaseTextureSRVs();

	m_Header.boundingObject.Clear();
}

void CModel::_ComputeMeshBoundingBox(unsigned int meshIndex, SBoundingObject& boundingObject) const
{
    const SMesh *mesh = m_pMesh + meshIndex;

	if (mesh->vertexCount > 0)
	{
		const DirectX::XMFLOAT3* pVertex = (DirectX::XMFLOAT3*)(m_pVertexData + mesh->vertexDataByteOffset + mesh->attrib[attrib_position].offset);
		boundingObject.ComputeFromPoints(mesh->vertexCount, pVertex, mesh->vertexStride);
	}
	else
		boundingObject.Clear();
}

void CModel::_ComputeGlobalBoundingBox(SBoundingObject& boundingObject) const
{
	unsigned int vertexStride = m_pMesh[0].vertexStride;
	
	if (m_Header.vertexDataByteSize > 0)
		boundingObject.ComputeFromPoints(m_Header.vertexDataByteSize / vertexStride, (DirectX::XMFLOAT3*)m_pVertexData, vertexStride);
	else
		boundingObject.Clear();
}

void CModel::_ComputeAllBoundingBoxes()
{
	for (unsigned int meshIndex = 0; meshIndex < m_Header.meshCount; meshIndex++)
	{
		SMesh *mesh = m_pMesh + meshIndex;
		_ComputeMeshBoundingBox(meshIndex, mesh->boundingObject);
	}
    _ComputeGlobalBoundingBox(m_Header.boundingObject);
}
