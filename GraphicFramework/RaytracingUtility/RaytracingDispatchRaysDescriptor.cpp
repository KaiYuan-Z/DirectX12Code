#include "RaytracingDispatchRaysDescriptor.h"
#include "RaytracingShaderTable.h"
#include "RaytracingPipelineUtility.h"

using namespace RaytracingUtility;

CRaytracingDispatchRaysDescriptor::CRaytracingDispatchRaysDescriptor()
{
}
	
CRaytracingDispatchRaysDescriptor::~CRaytracingDispatchRaysDescriptor()
{
	__ReleaseResources();
}

void CRaytracingDispatchRaysDescriptor::InitDispatchRaysDesc(UINT Widht, UINT Height, const CRaytracingShaderTable* pRayGenerationShaderRecord, const CRaytracingShaderTable* pMissShaderTable, const CRaytracingShaderTable* pHitGroupTable, const CRaytracingShaderTable* pCallableShaderTable)
{
	__ReleaseResources();

	_ASSERTE(pRayGenerationShaderRecord);
	m_pRayGenShaderTable = new CRaytracingShaderTable;
	*m_pRayGenShaderTable = *pRayGenerationShaderRecord;

	if (pMissShaderTable)
	{
		m_pMissShaderTable = new CRaytracingShaderTable;
		*m_pMissShaderTable = *pMissShaderTable;
	}

	if (pHitGroupTable)
	{
		m_pHitGroupTable = new CRaytracingShaderTable;
		*m_pHitGroupTable = *pHitGroupTable;
	}

	if (pCallableShaderTable)
	{
		m_pCallableShaderTable = new CRaytracingShaderTable;
		*m_pCallableShaderTable = *pCallableShaderTable;
	}

	m_pDispatchRaysDesc = new D3D12_DISPATCH_RAYS_DESC;
	*m_pDispatchRaysDesc = MakeDispatchRaysDesc(Widht, Height, m_pRayGenShaderTable, m_pMissShaderTable, m_pHitGroupTable, m_pCallableShaderTable);
}

void CRaytracingDispatchRaysDescriptor::__ReleaseResources()
{
	if (m_pDispatchRaysDesc) { delete m_pDispatchRaysDesc; m_pDispatchRaysDesc = nullptr; }
	if (m_pRayGenShaderTable) { delete m_pRayGenShaderTable; m_pRayGenShaderTable = nullptr; }
	if (m_pMissShaderTable) { delete m_pMissShaderTable; m_pMissShaderTable = nullptr; }
	if (m_pHitGroupTable) { delete m_pHitGroupTable; m_pHitGroupTable = nullptr; }
	if (m_pCallableShaderTable) { delete m_pCallableShaderTable; m_pCallableShaderTable = nullptr; }
}
