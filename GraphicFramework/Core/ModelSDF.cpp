#include "ModelSDF.h"
#include "GraphicsCore.h"

CModelSDF::CModelSDF() : CModelSDF(L"")
{
}

CModelSDF::CModelSDF(const std::wstring& modelName)
{
	m_ModelName = modelName;
	m_ModelType = MODEL_TYPE_SDF;
}

CModelSDF::~CModelSDF()
{
}

bool CModelSDF::Load(const char* filename)
{
	CModel::Load(filename);

	std::string SDFFileName(filename);
	SDFFileName.replace(SDFFileName.rfind(H3D_FILE_ENDING), H3D_FILE_ENDING.length(), FIELD_FILE_ENDING);
	
	if (!__ReadSDFFromFile(SDFFileName))
	{
		std::cout << "SDF: " << SDFFileName << " Load failed." << std::endl;
		return false;
	}

	return true;
}

const char* CModelSDF::__ReadFile(const char* path, bool binary)
{
	std::ios::openmode mode = std::ios::in;
	if (binary)
		mode = std::ios::in | std::ios::binary;
	std::ifstream ifs(path, mode);
	if (!ifs) {
		ASSERT(false, "Load file error!");
		return nullptr;
	}
	ifs.seekg(0, ifs.end);
	int length = (int)ifs.tellg();
	ifs.seekg(0, ifs.beg);
	char* buffer = nullptr;
	if (binary)
	{
		buffer = new char[length];
		ifs.read(buffer, length);
	}
	else
	{
		buffer = new char[length + 1];
		ifs.read(buffer, length);
		buffer[length] = '\0';
	}
	ifs.close();
	return buffer;
}

bool CModelSDF::__ReadSDFFromFile(const std::string& name)
{
	std::string FilePath = GraphicsCommon::KeywordStr::H3dModelLibRoot + name;
	const char* buffer = __ReadFile(FilePath.c_str(), true);
	if (buffer == nullptr) return false;

	const char* moveBuffer = buffer;

	uint32_t width, height, depth;
	float resolution;

	memcpy(&m_SDFMinBox, moveBuffer, sizeof(float) * 3);
	moveBuffer += sizeof(float) * 3;
	memcpy(&m_SDFMaxBox, moveBuffer, sizeof(float) * 3);
	moveBuffer += sizeof(float) * 3;

	memcpy(&width, moveBuffer, sizeof(uint32_t));
	moveBuffer += sizeof(uint32_t);
	memcpy(&height, moveBuffer, sizeof(uint32_t));
	moveBuffer += sizeof(uint32_t);
	memcpy(&depth, moveBuffer, sizeof(uint32_t));
	moveBuffer += sizeof(uint32_t);
	memcpy(&resolution, moveBuffer, sizeof(float));
	moveBuffer += sizeof(float);

	m_Dimension = XMUINT3(width, height, depth);
	std::vector<float> data(width * height * depth);
	memcpy(data.data(), moveBuffer, sizeof(float)*(data.size()));

	delete[] buffer;

	m_Resolution = resolution;

	GraphicsCore::STextureDataDesc TexData;
	TexData.Width = width;
	TexData.Height = height;
	TexData.Depth = depth;
	TexData.Format = DXGI_FORMAT_R32_FLOAT;
	TexData.BitSize = sizeof(float)*(data.size());
	TexData.BitData = data.data();
	m_SDFBuffer.Create3D(MakeWStr(name), width, height, depth, DXGI_FORMAT_R32_FLOAT);
	GraphicsCore::InitializeTexture(m_SDFBuffer, TexData);

	return true;
}