#include "ModelAssimp.h"
#include "../../Core/BoundingObject.h"
#include <stdio.h>

void PrintHelp()
{
    printf("model_convert\n");

    printf("usage:\n");
    printf("model_convert input_file output_file\n");
}

void PrintModelStats(const CModel *model)
{
    printf("model stats:\n");
    
    const SBoundingObject& boundingObject = model->GetBoundingObject();
    printf("bounding box: Center<%f, %f, %f> Extents<%f, %f, %f>\n"
        , (float)boundingObject.boundingBox.Center.x, (float)boundingObject.boundingBox.Center.y, (float)boundingObject.boundingBox.Center.z
        , (float)boundingObject.boundingBox.Extents.x, (float)boundingObject.boundingBox.Extents.y, (float)boundingObject.boundingBox.Extents.z);

    printf("vertex data size: %u\n", model->GetHeader().vertexDataByteSize);
    printf("index data size: %u\n", model->GetHeader().indexDataByteSize);
    printf("vertex data size depth-only: %u\n", model->GetHeader().vertexDataByteSizeDepth);
    printf("\n");

    printf("mesh count: %u\n", model->GetHeader().meshCount);
    for (unsigned int meshIndex = 0; meshIndex < model->GetHeader().meshCount; meshIndex++)
    {
        const CModel::SMesh& mesh = model->GetMesh(meshIndex);

        auto printAttribFormat = [](unsigned int format) -> void
        {
            switch (format)
            {
            case CModel::attrib_format_ubyte:
                printf("ubyte");
                break;

            case CModel::attrib_format_byte:
                printf("byte");
                break;

            case CModel::attrib_format_ushort:
                printf("ushort");
                break;

            case CModel::attrib_format_short:
                printf("short");
                break;

            case CModel::attrib_format_float:
                printf("float");
                break;
            }
        };

        printf("mesh %u\n", meshIndex);
        printf("vertices: %u\n", mesh.vertexCount);
        printf("indices: %u\n", mesh.indexCount);
        printf("vertex stride: %u\n", mesh.vertexStride);
        for (int n = 0; n < CModel::maxAttribs; n++)
        {
            if (mesh.attrib[n].format == CModel::attrib_format_none)
                continue;

            printf("attrib %d: offset %u, normalized %u, components %u, format "
                , n, mesh.attrib[n].offset, mesh.attrib[n].normalized
                , mesh.attrib[n].components);
            printAttribFormat(mesh.attrib[n].format);
            printf("\n");
        }

        printf("vertices depth-only: %u\n", mesh.vertexCountDepth);
        printf("vertex stride depth-only: %u\n", mesh.vertexStrideDepth);
        for (int n = 0; n < CModel::maxAttribs; n++)
        {
            if (mesh.attribDepth[n].format == CModel::attrib_format_none)
                continue;

            printf("attrib %d: offset %u, normalized %u, components %u, format "
                , n, mesh.attribDepth[n].offset, mesh.attribDepth[n].normalized
                , mesh.attribDepth[n].components);
            printAttribFormat(mesh.attrib[n].format);
            printf("\n");
        }
    }
    printf("\n");

    printf("material count: %u\n", model->GetHeader().materialCount);
    for (unsigned int materialIndex = 0; materialIndex < model->GetHeader().materialCount; materialIndex++)
    {
        const CModel::SMaterial& material = model->GetMaterial(materialIndex);

        printf("material %u\n", materialIndex);
    }
    printf("\n");
}

int main(int argc, char **argv)
{
   /* if (argc != 3)
    {
        PrintHelp();
        return -1;
    }

    const char *input_file = argv[1];
    const char *output_file = argv[2];*/

	const char *input_file = "Models/city.obj";
	const char *output_file = "../Models/city.h3d"; 

    printf("input file %s\n", input_file);
    printf("output file %s\n", output_file);

	// (1)Remove duplicate vertices and remove duplicate depth vertices, the result 
	//   vertices count is not equal with the result depth vertices count.
	// (2)The result depth vertices count is less than the result vertices count,
	//   because rendering shadow does not need too much details, and less depth 
	//   vertices count can raise rendering efficiency.

	bool needToOptimize = false;
	CModel::EIndexDataType indexType = CModel::index_type_uint32;
	AssimpModel model(needToOptimize, indexType);

    printf("loading...\n");
    if (!model.Load(input_file))
    {
        printf("failed to load model: %s\n", input_file);
        return -1;
    }

    printf("saving...\n");
    if (!model.Save(output_file))
    {
        printf("failed to save model: %s\n", output_file);
        return -1;
    }

    printf("done\n");

    PrintModelStats(&model);

    return 0;
}
