#define GRASS_INST_TAG 0x00000100
#define DEFORMABLE_INSTANCE_TAG 0x00001000
#define DYNAMIC_INSTANCE_TAG 0x00004000

#define DYNAMIC_INSTANCE_MARK_1 1
#define DYNAMIC_INSTANCE_MARK_2 2

struct InstanceData
{
    float4x4 world;
    uint sceneModelIndex;
    uint instanceIndex;
    uint instanceTag;
    float pad;
};
