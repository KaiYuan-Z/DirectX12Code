#pragma once
#include <iostream>
#include <fstream>
#include "GraphicsCommon.h"
#include "Model.h"
#include "ColorBuffer.h"

class CModelSDF : public CModel
{
public:
	CModelSDF();
	CModelSDF(const std::wstring& modelName);
	virtual ~CModelSDF();

	virtual bool Load(const char* filename) override;

	const XMFLOAT3& GetSDFMinBox() { return m_SDFMinBox; }
	const XMFLOAT3& GetSDFMaxBox() { return m_SDFMaxBox; }
	const float GetResolution() { return m_Resolution; }
	const XMUINT3& GetDimension() { return m_Dimension; }
	const CColorBuffer& GetSDFBuffer() { return m_SDFBuffer; }

private:

	XMFLOAT3 m_SDFMinBox;
	XMFLOAT3 m_SDFMaxBox;
	XMUINT3 m_Dimension;
	float m_Resolution;
	CColorBuffer m_SDFBuffer;

	const std::string FIELDS_DIR = GraphicsCommon::KeywordStr::H3dModelLibRoot;
	const std::string H3D_FILE_ENDING = ".h3d";
	const std::string FIELD_FILE_ENDING = ".sdf";

	const char* __ReadFile(const char* path, bool binary = false);
	bool __ReadSDFFromFile(const std::string& name);
};
