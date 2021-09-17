#include "../../Core/ShaderUtility/SceneGBufferRenderUtil.hlsl"

struct VSOutput
{
    float4 pos : SV_Position;
    float3 posW : PosW;
    float3 prePosW : PrePosW;
    float3 normal : Normal;
    float3 bitangent : Bitangent;
    float2 tex : TexCoord0;
    nointerpolation uint instanceID : InstanceID;
};

struct PSOutput
{
    float4 pos : SV_Target0;
    float4 diff : SV_Target1;
    float4 spec : SV_Target2;
    float4 norm : SV_Target3;//rawNorm, normMap
    float2 motion : SV_Target4;
    float2 ddxy : SV_Target5;
    float2 linearDepth : SV_Target6; // linearDepth, reprojectedlinearDepth
    int4   id : SV_Target7;
};

cbuffer cb_Config : register(b0, space0)
{
    bool g_EnablePBR;
};

PSOutput main(VSOutput vsOutput, in uint pID : SV_PrimitiveID)
{
    GBufferData gBuf;
    
    if (g_EnablePBR)
    {
        gBuf = preparePbrGBuffer(vsOutput.posW, vsOutput.normal, vsOutput.bitangent, vsOutput.tex);
    }
    else
    {
        gBuf = prepareBlinnPhongGBuffer(vsOutput.posW, vsOutput.normal, vsOutput.bitangent, vsOutput.tex);
    }
 
    float4 nonjitter_prePosH = mul(g_PreViewProjWithOutJitter, float4(vsOutput.prePosW, 1.0f));
    float4 nonjitter_curPosH = mul(g_CurViewProjWithOutJitter, float4(vsOutput.posW, 1.0f));
    float linearDepth = mul(g_CurView, float4(gBuf.posW, 1.0f)).z;
    float reprojectedlinearDepth = mul(g_PreView, float4(vsOutput.prePosW, 1.0f)).z;
    
    PSOutput gPsOut;
    gPsOut.pos = float4(gBuf.posW, 1.0f);
    gPsOut.norm = float4(encodeNormal(normalize(vsOutput.normal)), encodeNormal(gBuf.N));
    gPsOut.diff = gBuf.diffuse;
    gPsOut.spec = gBuf.specular;
    gPsOut.motion = calcMotionVector(nonjitter_prePosH, nonjitter_curPosH);
    gPsOut.linearDepth = float2(linearDepth, reprojectedlinearDepth);
    gPsOut.ddxy = float2(ddx(linearDepth), ddy(linearDepth));
    gPsOut.id = int4(g_InstanceData[vsOutput.instanceID].instanceIndex, ((g_InstanceData[vsOutput.instanceID].sceneModelIndex) << 16) + g_MeshIndex, pID, g_InstanceData[vsOutput.instanceID].instanceTag);
    return gPsOut;
}
