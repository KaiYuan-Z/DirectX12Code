#pragma once
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "../../Core/Model.h"

class AssimpModel : public CModel
{
public:

	enum
	{
		format_none = 0,
		format_h3d, // native format

		formats,
	};
	static const char *s_FormatString[];
	static int FormatFromFilename(const char *filename);

	AssimpModel() : m_NeedToOptimize(false), m_IndexDataType(index_type_uint16){}
	AssimpModel(bool NeedToOptimize, EIndexDataType ExpectedIndexDataType) : m_NeedToOptimize(NeedToOptimize), m_IndexDataType(ExpectedIndexDataType){}

	virtual bool Load(const char* filename) override;
	bool Save(const char* filename) const;

private:

	bool __LoadAssimp(const char *filename, EIndexDataType indexDataType);

	template <typename IndexType>
	void __LoadIndexData(const aiMesh* srcMesh, SMesh* dstMesh)
	{
		_ASSERTE(sizeof(IndexType) == sizeof(uint16_t) || sizeof(IndexType) == sizeof(uint32_t));
		
		IndexType *dstIndex = (IndexType*)(m_pIndexData + dstMesh->indexDataByteOffset);
		IndexType *dstIndexDepth = (IndexType*)(m_pIndexDataDepth + dstMesh->indexDataByteOffset);
		for (unsigned int f = 0; f < srcMesh->mNumFaces; f++)
		{
			assert(srcMesh->mFaces[f].mNumIndices == 3);

			*dstIndex++ = srcMesh->mFaces[f].mIndices[0];
			*dstIndex++ = srcMesh->mFaces[f].mIndices[1];
			*dstIndex++ = srcMesh->mFaces[f].mIndices[2];

			*dstIndexDepth++ = srcMesh->mFaces[f].mIndices[0];
			*dstIndexDepth++ = srcMesh->mFaces[f].mIndices[1];
			*dstIndexDepth++ = srcMesh->mFaces[f].mIndices[2];
		}
	}

	void __Optimize();
	void __OptimizeRemoveDuplicateVertices(bool depth);
	void __ptimizePostTransform(bool depth);
	void __ptimizePreTransform(bool depth);

	bool m_NeedToOptimize;
	EIndexDataType m_IndexDataType;
};

