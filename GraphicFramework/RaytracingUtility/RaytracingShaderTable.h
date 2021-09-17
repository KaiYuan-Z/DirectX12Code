#pragma once
#include "RaytracingBase.h"
#include "../Core/ByteAddressBuffer.h"


namespace RaytracingUtility
{
	class CShaderRecord
	{
		enum ERootArgumentType
		{
			kRootBufferView,
			kRootConstant,
			kRootDescriptorTable,
		};

		struct  SRootBufferViewArgument
		{
			D3D12_GPU_VIRTUAL_ADDRESS RootBufferGpuVirtualAddr;
		};

		struct  SRootConstantArgument
		{
			const void* pRootConstantAddr;
			UINT Root32BitConstantCount;
		};

		struct  SRootDescriptorTableArgument
		{
			D3D12_GPU_DESCRIPTOR_HANDLE RootDescriptorTableFirstHandle;
		};

		union URootArgument
		{
			SRootBufferViewArgument RootBufferViewArgument;
			SRootConstantArgument RootConstantArgument;
			SRootDescriptorTableArgument RootDescriptorTableArgument;
		};

		struct SPointerWithSize
		{
			void* ptr;
			UINT size;

			SPointerWithSize() : ptr(nullptr), size(0) {}
			SPointerWithSize(void* _ptr, UINT _size) : ptr(_ptr), size(_size) {};
		};

	public:

		friend class CRaytracingShaderTable;

		CShaderRecord() : m_ShaderRecordName(L"") {}
		~CShaderRecord() {}

		void ResetShaderRecord(void* pShaderID, UINT LocalRootArgumentCount = 0);
		void SetLocalRoot32BitConstants(UINT LocalRootIndex, UINT NumConstants, const void* pConstants);
		void SetLocalRootDescriptorTable(UINT LocalRootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle);
		void SetLocalRootBufferView(UINT LocalRootIndex, D3D12_GPU_VIRTUAL_ADDRESS BufferGpuVirtualAddr);
		void FinalizeShaderRecord(const std::wstring& ShaderRecordName = L"");

	private:

		void __Initialize();

		UINT __GetRootArgumentSize(UINT ArgumentIndex) const;
		UINT __AlignRootArgumentStartOffset(UINT StartOffset, ERootArgumentType ArgumentType) const;

		UINT __GetShaderRecordSizeInByte() const { return (UINT)m_ShaderRecordData.size(); }
		void __CopyTo(void* Dest) const;

		std::wstring m_ShaderRecordName;

		SPointerWithSize m_ShaderID;
		std::vector<std::pair<ERootArgumentType, URootArgument>> m_LocalRootArgumentSet;

		std::vector<byte> m_ShaderRecordData;
	};

	class CRaytracingShaderTable
	{
	public:

		CRaytracingShaderTable() : m_ShaderTableName(L""), m_ShaderRecordStrideSize(0) {}
		~CRaytracingShaderTable() {}

		void ResetShaderTable(UINT ShaderRecordCount);
		CShaderRecord& operator[] (size_t EntryIndex) { ASSERT(EntryIndex < m_ShaderRecordSet.size()); return m_ShaderRecordSet[EntryIndex]; }
		const CShaderRecord& operator[] (size_t EntryIndex) const { ASSERT(EntryIndex < m_ShaderRecordSet.size()); return m_ShaderRecordSet[EntryIndex]; }
		void FinalizeShaderTable(const std::wstring& ShaderTableName = L"");

		const std::wstring& GetName() const { return m_ShaderTableName; }
		D3D12_GPU_VIRTUAL_ADDRESS GetShaderTableGpuVirtualAddr() const { return m_ShaderTableData.GetGpuVirtualAddress(); }
		UINT GetShaderRecordCount() const { return (UINT)m_ShaderRecordSet.size(); }
		UINT GetShaderRecordStrideSizeInByte() const { return m_ShaderRecordStrideSize; }
		UINT GetShaderTableSizeInByte() const { return (UINT)m_ShaderTableData.GetBufferSize(); }

	private:

		std::wstring m_ShaderTableName;
		CByteAddressBuffer m_ShaderTableData;
		UINT m_ShaderRecordStrideSize;
		std::vector<CShaderRecord> m_ShaderRecordSet;
	};
}