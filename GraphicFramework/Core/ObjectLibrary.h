#pragma once
#include <vector>
#include <string>
#include <map>

#define INVALID_OBJECT_INDEX 0xffffffff

template<typename ObjectType>
class TObjectLibrary
{
public:
	TObjectLibrary()
	{
	}

	~TObjectLibrary()
	{
	}

	uint32_t AddObject(const std::wstring& objectName, const ObjectType& pObject)
	{
		if (IsObjectExist(objectName))
		{
			return INVALID_OBJECT_INDEX;
		}

		uint32_t index = (uint32_t)m_ObjectSet.size();
		m_ObjectSet.push_back(pObject); 
		m_Name2IndexMap[objectName] = index;
		m_Index2NameMap[index] = objectName;

		return index;
	}

	uint32_t EmplaceObject(const std::wstring& objectName)
	{
		if (IsObjectExist(objectName))
		{
			return INVALID_OBJECT_INDEX;
		}

		uint32_t index = (uint32_t)m_ObjectSet.size();
		m_ObjectSet.emplace_back();
		m_Name2IndexMap[objectName] = index;
		m_Index2NameMap[index] = objectName;

		return index;
	}

	bool DeleteObject(const std::wstring& objectName)
	{
		uint32_t objectEntry = QueryObjectEntry(objectName);
		if (objectEntry != INVALID_OBJECT_INDEX)
			return DeleteObject(objectEntry);
		else
			return false;
	}

	bool DeleteObject(uint32_t objectEntry)
	{
		uint32_t libSize = (uint32_t)m_ObjectSet.size();

		if (objectEntry < libSize)
		{
			if (objectEntry == libSize - 1)
			{
				m_ObjectSet.resize(libSize - 1);

				m_Name2IndexMap.erase(m_Index2NameMap[objectEntry]);
				m_Index2NameMap.erase(objectEntry);
			}
			else
			{
				uint32_t lastIndex = libSize - 1;
				std::wstring lastName = m_Index2NameMap[lastIndex];

				std::swap(m_ObjectSet[lastIndex], m_ObjectSet[objectEntry]);
				m_ObjectSet.resize(libSize - 1);

				m_Name2IndexMap.erase(m_Index2NameMap[objectEntry]);
				m_Index2NameMap.erase(lastIndex);
				m_Name2IndexMap[lastName] = objectEntry;
				m_Index2NameMap[objectEntry] = lastName;
			}

			return true;
		}
		else
			return false;

		return true;
	}

	uint32_t QueryObjectEntry(const std::wstring& objectName) const 
	{
		std::map<std::wstring, uint32_t>::const_iterator it = m_Name2IndexMap.find(objectName);
		if (it != m_Name2IndexMap.end())
			return it->second;
		else
			return INVALID_OBJECT_INDEX;
	}

	bool IsObjectExist(const std::wstring& objectName) const
	{
		return (QueryObjectEntry(objectName) != INVALID_OBJECT_INDEX);
	}

	ObjectType& operator[](uint32_t objectEntry)
	{
		_ASSERTE(objectEntry < m_ObjectSet.size());
		return m_ObjectSet[objectEntry];
	}

	const ObjectType& operator[](uint32_t objectEntry) const
	{
		_ASSERTE(objectEntry < m_ObjectSet.size());
		return m_ObjectSet[objectEntry];
	}

	uint32_t GetObjectCount() const
	{ 
		return (uint32_t)m_ObjectSet.size();
	}


private:
	std::vector<ObjectType> m_ObjectSet;
	std::map<std::wstring, uint32_t> m_Name2IndexMap;
	std::map<uint32_t, std::wstring> m_Index2NameMap;
};