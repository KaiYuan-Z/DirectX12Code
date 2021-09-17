#include "RaytracingShaderTable.h"

using namespace RaytracingUtility;


#define ALIGN(alignment, num) ((((num) + alignment - 1) / alignment) * alignment)


void CShaderRecord::ResetShaderRecord(void* pShaderID, UINT LocalRootArgumentCount)
{
	_ASSERTE(pShaderID);
	m_ShaderID.ptr = pShaderID;

	m_LocalRootArgumentSet.clear();
	m_ShaderRecordData.clear();

	if (LocalRootArgumentCount > 0) m_LocalRootArgumentSet.resize(LocalRootArgumentCount);
}

void CShaderRecord::SetLocalRoot32BitConstants(UINT LocalRootIndex, UINT NumConstants, const void* pConstants)
{
	URootArgument RootArgument;
	_ASSERTE(NumConstants > 0 && pConstants);
	RootArgument.RootConstantArgument.Root32BitConstantCount = NumConstants;
	RootArgument.RootConstantArgument.pRootConstantAddr = pConstants;
	_ASSERTE(LocalRootIndex < m_LocalRootArgumentSet.size());
	m_LocalRootArgumentSet[LocalRootIndex] = std::make_pair(ERootArgumentType::kRootConstant, RootArgument);
}

void CShaderRecord::SetLocalRootDescriptorTable(UINT LocalRootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle)
{
	URootArgument RootArgument;
	RootArgument.RootDescriptorTableArgument.RootDescriptorTableFirstHandle = FirstHandle;
	_ASSERTE(LocalRootIndex < m_LocalRootArgumentSet.size());
	m_LocalRootArgumentSet[LocalRootIndex] = std::make_pair(ERootArgumentType::kRootDescriptorTable, RootArgument);
}

void CShaderRecord::SetLocalRootBufferView(UINT LocalRootIndex, D3D12_GPU_VIRTUAL_ADDRESS BufferGpuVirtualAddr)
{
	URootArgument RootArgument;
	RootArgument.RootBufferViewArgument.RootBufferGpuVirtualAddr = BufferGpuVirtualAddr;
	_ASSERTE(LocalRootIndex < m_LocalRootArgumentSet.size());
	m_LocalRootArgumentSet[LocalRootIndex] = std::make_pair(ERootArgumentType::kRootBufferView, RootArgument);
}

void CShaderRecord::FinalizeShaderRecord(const std::wstring& ShaderRecordName)
{
	m_ShaderRecordName = ShaderRecordName;

	if (m_LocalRootArgumentSet.size() > 0)
	{
		std::vector<UINT> AlignedStartOffsets(m_LocalRootArgumentSet.size());
		UINT CurUnalignedStartOffset = m_ShaderID.size;
		ERootArgumentType CurArgumentType = m_LocalRootArgumentSet[0].first;
		AlignedStartOffsets[0] = __AlignRootArgumentStartOffset(CurUnalignedStartOffset, CurArgumentType);
		for (int i = 1; i < m_LocalRootArgumentSet.size(); i++)
		{
			CurArgumentType = m_LocalRootArgumentSet[i].first;
			CurUnalignedStartOffset = AlignedStartOffsets[i - 1] + __GetRootArgumentSize(i - 1);
			AlignedStartOffsets[i] = __AlignRootArgumentStartOffset(CurUnalignedStartOffset, CurArgumentType);
		}
		UINT LastArgumentIndex = (UINT)m_LocalRootArgumentSet.size() - 1;
		UINT UnalignedShaderRecordSize = AlignedStartOffsets[LastArgumentIndex] + __GetRootArgumentSize(LastArgumentIndex);

		UINT AlignedShaderRecordSize = ALIGN(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, UnalignedShaderRecordSize);

		m_ShaderRecordData.resize(AlignedShaderRecordSize);
		_ASSERTE(m_ShaderID.ptr && m_ShaderID.size > 0);
		memcpy(m_ShaderRecordData.data(), m_ShaderID.ptr, m_ShaderID.size);

		for (int i = 0; i < m_LocalRootArgumentSet.size(); i++)
		{
			ERootArgumentType ArgumentType = m_LocalRootArgumentSet[i].first;
			const URootArgument& Argument = m_LocalRootArgumentSet[i].second;
			byte* StartAddr = m_ShaderRecordData.data() + AlignedStartOffsets[i];

			switch (ArgumentType)
			{
			case kRootBufferView:
				memcpy(StartAddr, &Argument.RootBufferViewArgument.RootBufferGpuVirtualAddr, __GetRootArgumentSize(i));
				break;
			case kRootDescriptorTable:
				memcpy(StartAddr, &Argument.RootDescriptorTableArgument.RootDescriptorTableFirstHandle, __GetRootArgumentSize(i));
				break;
			case kRootConstant:
				memcpy(StartAddr, Argument.RootConstantArgument.pRootConstantAddr, __GetRootArgumentSize(i));
				break;
			default:_ASSERTE(false);
			}
		}
	}
	else
	{
		UINT AlignedShaderRecordSize = ALIGN(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, m_ShaderID.size);
		m_ShaderRecordData.resize(AlignedShaderRecordSize);
		_ASSERTE(m_ShaderID.ptr && m_ShaderID.size > 0);
		memcpy(m_ShaderRecordData.data(), m_ShaderID.ptr, m_ShaderID.size);
	}
}

void CShaderRecord::__Initialize()
{
	m_ShaderID.size = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	_ASSERTE(m_ShaderID.size > 0);
	m_ShaderID.ptr = nullptr;
}

UINT CShaderRecord::__GetRootArgumentSize(UINT ArgumentIndex) const
{
	_ASSERTE(ArgumentIndex < m_LocalRootArgumentSet.size());

	ERootArgumentType ArgumentType = m_LocalRootArgumentSet[ArgumentIndex].first;
	const URootArgument& RootArgument = m_LocalRootArgumentSet[ArgumentIndex].second;

	switch (ArgumentType)
	{
	case kRootBufferView: return sizeof(D3D12_GPU_VIRTUAL_ADDRESS);
	case kRootDescriptorTable: return sizeof(D3D12_GPU_DESCRIPTOR_HANDLE);
	case kRootConstant: return sizeof(uint32_t)*RootArgument.RootConstantArgument.Root32BitConstantCount;
	default:_ASSERTE(false); return 0;
	}
}

UINT CShaderRecord::__AlignRootArgumentStartOffset(UINT StartOffset, ERootArgumentType ArgumentType) const
{
	switch (ArgumentType)
	{
	case kRootBufferView: return ALIGN(sizeof(D3D12_GPU_VIRTUAL_ADDRESS), StartOffset);
	case kRootConstant: return ALIGN(sizeof(uint32_t), StartOffset);
	case kRootDescriptorTable: return ALIGN(sizeof(D3D12_GPU_DESCRIPTOR_HANDLE), StartOffset);
	default:_ASSERTE(false); return 0;
	}
}

void CShaderRecord::__CopyTo(void* Dest) const
{
	UINT ShaderRecordSize = __GetShaderRecordSizeInByte();
	_ASSERTE(ShaderRecordSize > 0);
	byte* ByteDest = static_cast<byte*>(Dest);
	_ASSERTE(&ByteDest[ShaderRecordSize - 1]);
	memcpy(ByteDest, m_ShaderRecordData.data(), ShaderRecordSize);
}

void CRaytracingShaderTable::ResetShaderTable(UINT ShaderRecordCount)
{
	_ASSERTE(ShaderRecordCount > 0);

	m_ShaderRecordSet.clear();

	m_ShaderRecordSet.resize(ShaderRecordCount);
	for (UINT i = 0; i < ShaderRecordCount; i++) m_ShaderRecordSet[i].__Initialize();
}

void CRaytracingShaderTable::FinalizeShaderTable(const std::wstring& ShaderTableName)
{
	m_ShaderTableName = ShaderTableName;

	for (int i = 0; i < m_ShaderRecordSet.size(); i++)
	{
		UINT RecordSize = m_ShaderRecordSet[i].__GetShaderRecordSizeInByte();
		if (m_ShaderRecordStrideSize < RecordSize) m_ShaderRecordStrideSize = RecordSize;
	}

	UINT ShaderTableSize = m_ShaderRecordStrideSize * (UINT)m_ShaderRecordSet.size();
	std::vector<byte> ShaderTableData(ShaderTableSize);

	UINT StartOffset = 0;
	for (int i = 0; i < m_ShaderRecordSet.size(); i++)
	{	
		m_ShaderRecordSet[i].__CopyTo(ShaderTableData.data() + StartOffset);
		StartOffset += m_ShaderRecordStrideSize;
	}

	m_ShaderTableData.Create(m_ShaderTableName + L"(Raytracing Shader Table GPU Resource)", 1, ShaderTableSize, ShaderTableData.data());
}