
/* 
 Ref: Christopher A. Burns, Warren A. Hunt, 
      The VisibilityBuffer: A Cache-Friendly Approach to DeferredShading, 
      Journal of Computer GraphicsTechniques (JCGT), vol. 2, no. 2, 55¨C69,2013
*/


float __dot3(float3 a, float3 b)
{
    return mad(a.x, b.x, mad(a.y, b.y, a.z * b.z));
}
float3 __cross3(float3 a, float3 b)
{
    return mad(a, b.yzx, -a.yzx * b).yzx;
}

float3 __intersect(float3 p0, float3 p1, float3 p2, float3 o, float3 d)
{
    float3 eo = o - p0;
    float3 e2 = p2 - p0;
    float3 e1 = p1 - p0;
    float3 r = __cross3(d, e2);
    float3 s = __cross3(eo, e1);
    float iV = 1.0f / __dot3(r, e1);
    float V1 = __dot3(r, eo);
    float V2 = __dot3(s, d);
    float b = V1 * iV;
    float c = V2 * iV;
    float a = 1.0f - b - c;
    return float3(a, b, c);
}

float3 getBarycentricCoordinates(
    float4x4 invViewProj, 
    float4x4 objectToWorld, 
    uint2 imageSize, 
    uint2 pixelIndex, 
    float3 camPos, 
    float3 v0, 
    float3 v1, 
    float3 v2)
{
    float4 p0 = mul(objectToWorld, float4(v0, 1.0f));
    float4 p1 = mul(objectToWorld, float4(v1, 1.0f));
    float4 p2 = mul(objectToWorld, float4(v2, 1.0f)); 
    
    float2 screenPos = (pixelIndex + 0.5f) / imageSize * 2.0f - 1.0f;
    screenPos.y = -screenPos.y;

    float4 world = mul(invViewProj, float4(screenPos, 0.0f, 1.0f));
    world.xyz /= world.w;

    float3 rayOrigin = camPos;
    float3 rayDirection = normalize(world.xyz - rayOrigin);
    
    return __intersect(p0.xyz, p1.xyz, p2.xyz, rayOrigin, rayDirection);
}

float3 interpolationAttribute(float3 vertexAttribute[3], float3 b)
{
    return mad(vertexAttribute[0], b.x, mad(vertexAttribute[1], b.y, (vertexAttribute[2] * b.z)));
}