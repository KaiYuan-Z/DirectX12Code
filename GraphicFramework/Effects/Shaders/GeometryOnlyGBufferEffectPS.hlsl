#include "../../Core/ShaderUtility/SceneGeometryOnlyRenderUtil.hlsl"
#include "../../Core/ShaderUtility/MathUtil.hlsl"

struct VSOutput
{
    float4 pos          : SV_Position;
    float3 prePosW      : PrePosW;
    float3 posW         : PosW;
    float3 normal       : Normal;
    float2 texcoord     : Texcoord;
    nointerpolation uint instanceID : InstanceID;
};

struct PSOutput
{
	float4 posW                   : SV_Target0;  
    float2 norm                   : SV_Target1;
    float2 motion                 : SV_Target2;
    float2 ddxy                   : SV_Target3;
    float2 linearDepth            : SV_Target4;
    int4   id                     : SV_Target5;
};



PSOutput main(VSOutput vsOutput, in uint pID : SV_PrimitiveID)
{
    alphaClip(vsOutput.texcoord, 0.3f);
    
    PSOutput gPsOut;
    float4 posW = float4(vsOutput.posW, 1.0f);
    float3 normal = normalize(vsOutput.normal);
    float4 nonjitter_prePosH = mul(g_PreViewProjWithOutJitter, float4(vsOutput.prePosW, 1.0f));
    float4 nonjitter_curPosH = mul(g_CurViewProjWithOutJitter, float4(vsOutput.posW, 1.0f));
    float2 motion = calcMotionVector(nonjitter_prePosH, nonjitter_curPosH);
    float linearDepth = mul(g_CurView, posW).z;
    float reprojectedlinearDepth = mul(g_PreView, float4(vsOutput.prePosW, 1.0f)).z;
    float2 linearDepthDerivative = float2(ddx(linearDepth), ddy(linearDepth));
    
    gPsOut.posW = posW;
    gPsOut.norm = encodeNormal(normal);
    gPsOut.motion = motion;
    gPsOut.ddxy = linearDepthDerivative;
    gPsOut.linearDepth = float2(linearDepth, reprojectedlinearDepth);
    gPsOut.id = int4(g_InstanceData[vsOutput.instanceID].instanceIndex, ((g_InstanceData[vsOutput.instanceID].sceneModelIndex) << 16) + g_MeshIndex, pID, g_InstanceData[vsOutput.instanceID].instanceTag);
    
    return gPsOut;
}
