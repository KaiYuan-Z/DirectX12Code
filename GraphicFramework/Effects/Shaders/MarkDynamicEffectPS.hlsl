#include "../../Core/ShaderUtility/RenderCommonUtil.hlsl"

struct VSOutput
{
    float4 extentPos : SV_Position;
    nointerpolation int instanceTag : InstanceTag;
};

int main(VSOutput vsOutput) : SV_Target0
{
    if (vsOutput.instanceTag & GRASS_INST_TAG)
    {
        return DYNAMIC_INSTANCE_MARK_2;
    }
    else
    {
        return DYNAMIC_INSTANCE_MARK_1;
    }
}
