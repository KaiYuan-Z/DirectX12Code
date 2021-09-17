#define M_PI     3.14159265358979323846
#define M_PI2    6.28318530717958647692
#define M_INV_PI 0.3183098861837906715

#define COLOR_LIGHT_BLUE float4(0.678431392f, 0.847058892f, 0.901960850f, 1.000000000f)

uint getIndexOfValueClosestToTheReference(in float refValue, in float2 vValues)
{
    float2 delta = abs(refValue - vValues);

    uint outIndex = delta[1] < delta[0] ? 1 : 0;

    return outIndex;
}

uint getIndexOfValueClosestToTheReference(in float refValue, in float4 vValues)
{
    float4 delta = abs(refValue - vValues);

    uint outIndex = delta[1] < delta[0] ? 1 : 0;
    outIndex = delta[2] < delta[outIndex] ? 2 : outIndex;
    outIndex = delta[3] < delta[outIndex] ? 3 : outIndex;

    return outIndex;
}

uint smallestPowerOf2GreaterThan(in uint x)
{
    // Set all the bits behind the most significant non-zero bit in x to 1.
    // Essentially giving us the largest value that is smaller than the
    // next power of 2 we're looking for.
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;

    // Return the next power of two value.
    return x + 1;
}

float floatPrecision(in float x, in uint NumMantissaBits)
{
    // Find the exponent range the value is in.
    uint nextPowerOfTwo = smallestPowerOf2GreaterThan(x);
    float exponentRange = nextPowerOfTwo - (nextPowerOfTwo >> 1);

    float MaxMantissaValue = 1 << NumMantissaBits;

    return exponentRange / MaxMantissaValue;
}

uint float2ToUint(in float2 val)
{
    uint result = 0;
    result = f32tof16(val.x);
    result |= f32tof16(val.y) << 16;
    return result;
}

float2 uintToFloat2(in uint val)
{
    float2 result;
    result.x = f16tof32(val);
    result.y = f16tof32(val >> 16);
    return result;
}

float2 octWrap(float2 v)
{
    return (1.0 - abs(v.yx)) * (v.xy >= 0.0 ? 1.0 : -1.0);
}

// Converts a 3D unit vector to a 2D vector with <0,1> range. 
float2 encodeNormal(float3 n)
{
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    n.xy = n.z >= 0.0 ? n.xy : octWrap(n.xy);
    n.xy = n.xy * 0.5 + 0.5;
    return n.xy;
}

float3 decodeNormal(float2 f)
{
    f = f * 2.0 - 1.0;

    // https://twitter.com/Stubbesaurus/status/937994790553227264
    float3 n = float3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
    float t = saturate(-n.z);
    n.xy += n.xy >= 0.0 ? -t : t;
    return normalize(n);
}

uint normal2Uint(float3 n)
{
    return float2ToUint(encodeNormal(n));
}

float3 uintToNormal(uint v)
{
    return decodeNormal(uintToFloat2(v));
}

uint4 storeNormBitanMotionDdxy(float3 norm, float3 bitan, float2 motion, float2 ddxy)
{
    return uint4(normal2Uint(norm), normal2Uint(bitan), float2ToUint(motion), float2ToUint(ddxy));
}

//========================================
// Angle Helper Functions
//========================================

float getAngleNormalized(float3 vec0, float3 vec1)
{
    float cosAngle = dot(vec0, vec1);
    return acos(cosAngle);
}

float getRotationAngleNormalized(float3 vec0, float3 vec1)
{
    float cosAngle = dot(vec0, vec1);
    float3 crossVec = cross(vec0, vec1);
    float acosAngle = acos(cosAngle);
    float signAngle = sign(crossVec.z);
    return signAngle * acosAngle + (1 - signAngle) * (M_PI2 - acosAngle);
}

float getHorizontalRotationAngleNormalized(float3 vec0, float3 norm, float3 vec1)
{
    float3 horizontalVec1 = vec1 - dot(norm, vec1) * norm;
    horizontalVec1 = normalize(horizontalVec1);
    return getRotationAngleNormalized(vec0, horizontalVec1);
}



//========================================
// Screen Space Helper Functions
//========================================

// Verify if texcoord is in range (0, 1)
bool verifyTexcoord(float2 texcoord)
{
    return (texcoord.x > 0.0f) && (texcoord.x < 1.0f) && (texcoord.y > 0.0f) && (texcoord.y < 1.0f);
}

bool isWithinBounds(in int2 index, in int2 dimensions)
{
    return (index.x >= 0) && (index.y >= 0) && (index.x < dimensions.x) && (index.y < dimensions.y);
}

uint2 texcoord2Index(uint2 Dim, float2 tex)
{
    float2 index = round(float2(Dim) * tex - float2(0.5f, 0.5f));
    index.x = clamp(index.x, 0, Dim.x - 1);
    index.y = clamp(index.y, 0, Dim.y - 1);
    return uint2(index);
}

float2 index2Texcoord(uint2 Dim, uint2 index)
{
    return (float2(index) + float2(0.5f, 0.5f)) / float2(Dim);
}

// Ndc to texcoord
double2 ndc2Texcoord(double2 ndc)
{
    double2 tex = 0.5 * ndc + 0.5;
    tex.y = 1.0 - tex.y;
    return tex;
}

float2 ndc2Texcoord(float2 ndc)
{
    float2 tex = 0.5f * ndc + 0.5f;
    tex.y = 1.0f - tex.y;
    return tex;
}

// Texcoord to ndc
double2 texcoord2Ndc(double2 tex)
{
    double x = tex.x * 2.0 - 1.0;
    double y = (1.0 - tex.y) * 2.0 - 1.0;
    return double2(x, y);
}

float2 texcoord2Ndc(float2 tex)
{
    float x = tex.x * 2.0f - 1.0f;
    float y = (1.0f - tex.y) * 2.0f - 1.0f;
    return float2(x, y);
}

// Ndc depth to view depth(row-major!!!)
float ndcDepth2ViewDepthRM(float4x4 Proj, float z_ndc)
{
	// z_ndc = A + B/viewZ, where g_Proj[2,2]=A and g_Proj[3,2]=B.
    float viewZ = Proj[3][2] / (z_ndc - Proj[2][2]);
    return viewZ;
}

// Ndc depth to view depth(line-major!!!)
float ndcDepth2ViewDepthLM(float4x4 Proj, float z_ndc)
{
	// z_ndc = A + B/viewZ, where g_Proj[2,2]=A and g_Proj[2,3]=B.
    float viewZ = Proj[2][3] / (z_ndc - Proj[2][2]);
    return viewZ;
}

float2 calcMotionVector(float4 prePosH, float4 curPixelPos, float2 winSize)
{
    float2 preTex = ndc2Texcoord(prePosH.xy / prePosH.w);
    float2 curTex = curPixelPos.xy / winSize;
    float2 motionVec = preTex - curTex;

    // Guard against inf/nan due to projection by w <= 0.
    const float epsilon = 1e-5f;
    motionVec = (prePosH.w < epsilon) ? float2(0, 0) : motionVec;

    return motionVec;
}

float2 calcMotionVector(float4 prePosH, float4 curPosH)
{
    float2 preTex = ndc2Texcoord(prePosH.xy / prePosH.w);
    float2 curTex = ndc2Texcoord(curPosH.xy / curPosH.w);
    float2 motionVec = preTex - curTex;

    // Guard against inf/nan due to projection by w <= 0.
    const float epsilon = 1e-5f;
    motionVec = (prePosH.w < epsilon) ? float2(0, 0) : motionVec;

    return motionVec;
}

// Calculate Gauss Weights
float calcGaussWeights(int radius, int r, float d0, float d1)
{
    const float Sigma = ((float) radius + 1.0f) * 0.5f;
    const float BlurFalloff = 1.f / (2.0f * Sigma * Sigma);
    float x = (float) r;
    float dz = d1 - d0;
    return exp2(-x * x * BlurFalloff - dz * dz);
}

float calcGaussWeights(int radius, int r)
{
    const float Sigma = ((float) radius + 1.0f) * 0.5f;
    const float BlurFalloff = 1.f / (2.0f * Sigma * Sigma);
    float x = (float) r;
    return exp2(-x * x * BlurFalloff);
}



//========================================
// Sample Helper Functions
//========================================

// Utility function to get a vector perpendicular to an input vector 
//    (from "Efficient Construction of Perpendicular Vectors Without Branching")
float3 getPerpendicularVector(float3 u)
{
    float3 a = abs(u);
    uint xm = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1 : 0;
    uint ym = (a.y - a.z) < 0 ? (1 ^ xm) : 0;
    uint zm = 1 ^ (xm | ym);
    return cross(u, float3(xm, ym, zm));
}

// Note: BaseZ is input norm.
void genBaseForNormal(float3 norm, inout float3 baseX, inout float3 baseY, inout float3 baseZ)
{
    float3 bitangent = getPerpendicularVector(norm);
    float3 tangent = cross(bitangent, norm);
    baseX = normalize(tangent);
    baseY = normalize(bitangent);
    baseZ = normalize(norm);
}

void genBaseForNormal(float3 norm, float3 bitan, inout float3 baseX, inout float3 baseY, inout float3 baseZ)
{
    baseZ = norm.xyz;
    baseY = bitan.xyz;
    baseX = cross(baseY, baseZ);
    baseX = normalize(baseX);
    baseY = normalize(baseY);
    baseZ = normalize(baseZ);
}

// Generates a seed for a random number generator from 2 inputs plus a backoff
uint initRand(uint val0, uint val1, uint backoff = 8)
{
    uint v0 = val0, v1 = val1, s0 = 0;

	[unroll]
    for (uint n = 0; n < backoff; n++)
    {
        s0 += 0x9e3779b9;
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
    }
    return v0;
}

// Takes our seed, updates it, and returns a pseudorandom float in [0..1]
float nextRand(inout uint s)
{
    s = (1664525u * s + 1013904223u);
    return float(s & 0x00FFFFFF) / float(0x01000000);
}

float3 getNextSphereSample(float3 baseX, float3 baseY, float3 baseZ, inout uint randSeed)
{
    float u1 = nextRand(randSeed);
    float u2 = nextRand(randSeed);

    u1 = u1 * 2.0f - 1.0f;

    float r = sqrt(1.0f - u1 * u1);
    float theta = 2.0f * M_PI * u2;

    return baseX * (r * cos(theta)) + baseY * (r * sin(theta)) + baseZ * u1;
}

float3 getNextHemisphereSample(inout uint randSeed, float3 hitNorm)
{
    float2 randVal = float2(nextRand(randSeed), nextRand(randSeed));

    float3 bitangent = getPerpendicularVector(hitNorm);
    float3 tangent = cross(bitangent, hitNorm);
    float r = sqrt(1.0f - randVal.x * randVal.x);
    float phi = 2.0f * 3.14159265f * randVal.y;

    return tangent * (r * cos(phi)) + bitangent * (r * sin(phi)) + hitNorm.xyz * randVal.x;
}

float3 getNextHemisphereSample(float3 baseX, float3 baseY, float3 baseZ, inout uint randSeed)
{
    float u1 = nextRand(randSeed);
    float u2 = nextRand(randSeed);

    float r = sqrt(1.0f - u1 * u1);
    float theta = 2.0f * M_PI * u2;

    return baseX * (r * cos(theta)) + baseY * (r * sin(theta)) + baseZ * u1;
}

float3 getNextCosHemisphereSample(float3 baseX, float3 baseY, float3 baseZ, inout uint randSeed)
{
    float u1 = nextRand(randSeed);
    float u2 = nextRand(randSeed);

    float r = sqrt(u1);
    float theta = 2.0f * M_PI * u2;

    return baseX * (r * cos(theta)) + baseY * (r * sin(theta)) + baseZ * sqrt(1 - u1);
}

float haltonSample(uint sampleIndex, uint base)
{
    float result = 0.0f;
    float f = 1.0f;

    while (sampleIndex > 0)
    {
        f = f / base;
        result += f * (sampleIndex % base);
        sampleIndex = sampleIndex / base;
    }

    return result;
}

float2 haltonSample2D(uint sampleIndex, uint base1, uint base2)
{
    return float2(haltonSample(sampleIndex, base1), haltonSample(sampleIndex, base2));
}

float3 getHaltonSphereSample(float3 baseX, float3 baseY, float3 baseZ, int sampleIndex, int p1 = 2, int p2 = 3)
{
    float u1 = haltonSample(sampleIndex, p1);
    float u2 = haltonSample(sampleIndex, p2);

    u1 = u1 * 2.0f - 1.0f;

    float r = sqrt(1.0f - u1 * u1);
    float theta = 2.0f * M_PI * u2;

    return baseX * (r * cos(theta)) + baseY * (r * sin(theta)) + baseZ * u1;
}

float3 getHaltonHemisphereSample(float3 baseX, float3 baseY, float3 baseZ, int sampleIndex, int p1 = 2, int p2 = 3)
{
    float u1 = haltonSample(sampleIndex, p1);
    float u2 = haltonSample(sampleIndex, p2);

    float r = sqrt(1 - u1 * u1);
    float theta = 2.0f * M_PI * u2;

    return baseX * (r * cos(theta)) + baseY * (r * sin(theta)) + baseZ * u1;
}

float3 getHaltonCosHemisphereSample(float3 baseX, float3 baseY, float3 baseZ, int sampleIndex, int p1 = 2, int p2 = 3)
{
    float u1 = haltonSample(sampleIndex, p1);
    float u2 = haltonSample(sampleIndex, p2);

    float r = sqrt(u1);
    float theta = 2.0f * M_PI * u2;

    return baseX * (r * cos(theta)) + baseY * (r * sin(theta)) + baseZ * sqrt(1 - u1);
}

float3 getHemisphereSample(float3 baseX, float3 baseY, float3 baseZ, float u1, float u2)
{
    float r = sqrt(1 - u1 * u1);
    float theta = 2.0f * M_PI * u2;
    return baseX * (r * cos(theta)) + baseY * (r * sin(theta)) + baseZ * u1;
}

float3 getCosHemisphereSample(float3 baseX, float3 baseY, float3 baseZ, float u1, float u2)
{
    float r = sqrt(u1);
    float theta = 2.0f * M_PI * u2;
    return baseX * (r * cos(theta)) + baseY * (r * sin(theta)) + baseZ * sqrt(1 - u1);
}


//=======================================
// Sub Range Sample Helper Functions
//=======================================
//Vertical index from top to bottom : 0 -> nVertical - 1;
//Horizontal index from 0 to 360 : 0 -> nHorizontal - 1;

uint2 vector2HemisphereIndex(uint nHorizontal, uint nVertical, float3 baseX, float3 baseZ, float3 vecNormalized)
{
    float theta = getHorizontalRotationAngleNormalized(baseX, baseZ, vecNormalized);
    float uHorizontal = theta / (2.0f * M_PI);

    float cosPhi = dot(baseZ, vecNormalized);
    float uVertical = 1.0f - cosPhi;

    uint h = uHorizontal * (nHorizontal - 1);
    uint v = uVertical * (nVertical - 1);

    return uint2(h, v);
}

uint2 vector2CosHemisphereIndex(uint nHorizontal, uint nVertical, float3 baseX, float3 baseZ, float3 vecNormalized)
{
    float theta = getHorizontalRotationAngleNormalized(baseX, baseZ, vecNormalized);
    float uHorizontal = theta / (2.0f * M_PI);

    float cosPhi = dot(baseZ, vecNormalized);
    float uVertical = 1 - cosPhi * cosPhi;

    uint h = uHorizontal * (nHorizontal - 1);
    uint v = uVertical * (nVertical - 1);

    return uint2(h, v);
}

// Cos index to cos sub range, or Uniform index to uniform sub range
float4 index2SubRangeScale(uint nHorizontal, uint nVertical, uint h, uint v)
{
    float minScaleHorizontal = float(h) / float(nHorizontal);
    float maxScaleHorizontal = float(h + 1) / float(nHorizontal);
    float minScaleVertical = float(v) / float(nVertical);
    float maxScaleVertical = float(v + 1) / float(nVertical);

    return float4(minScaleHorizontal, maxScaleHorizontal, minScaleVertical, maxScaleVertical);
}

float cosVerticalSubRangeScale2UniformVerticalSubRangeScale(float cosVerticalSubRangeScale)
{
    return 1.0f - sqrt(1.0f - cosVerticalSubRangeScale);
}

float4 cosIndex2UniformSubRangeScale(uint nHorizontal, uint nVertical, uint h, uint v)
{
    float minScaleHorizontal = float(h) / float(nHorizontal);
    float maxScaleHorizontal = float(h + 1) / float(nHorizontal);
    float cosMinScaleVertical = float(v) / float(nVertical);
    float cosMaxScaleVertical = float(v + 1) / float(nVertical);

    return float4(minScaleHorizontal, maxScaleHorizontal, cosVerticalSubRangeScale2UniformVerticalSubRangeScale(cosMinScaleVertical), cosVerticalSubRangeScale2UniformVerticalSubRangeScale(cosMaxScaleVertical));
}

float3 getNextHemisphereRangeSample(float3 baseX, float3 baseY, float3 baseZ, float minScaleHorizontal, float maxScaleHorizontal, float minScaleVertical, float maxScaleVertical, inout uint randSeed)
{
    float u1 = nextRand(randSeed);
    float u2 = nextRand(randSeed);

    u1 = minScaleVertical + (maxScaleVertical - minScaleVertical) * u1;
    u1 = 1.0f - u1;
    u2 = minScaleHorizontal + (maxScaleHorizontal - minScaleHorizontal) * u2;

    float r = sqrt(1.0f - u1 * u1);
    float theta = 2.0f * M_PI * u2;

    return baseX * (r * cos(theta)) + baseY * (r * sin(theta)) + baseZ * u1;
}

float3 getNextCosHemisphereRangeSample(float3 baseX, float3 baseY, float3 baseZ, float minScaleHorizontal, float maxScaleHorizontal, float minScaleVertical, float maxScaleVertical, inout uint randSeed)
{
    float u1 = nextRand(randSeed);
    float u2 = nextRand(randSeed);

    u1 = minScaleVertical + (maxScaleVertical - minScaleVertical) * u1;
    u2 = minScaleHorizontal + (maxScaleHorizontal - minScaleHorizontal) * u2;

    float r = sqrt(u1);
    float theta = 2.0f * M_PI * u2;

    return baseX * (r * cos(theta)) + baseY * (r * sin(theta)) + baseZ * sqrt(1 - u1);
}

float3 getHaltonHemisphereRangeSample(float3 baseX, float3 baseY, float3 baseZ, float minScaleHorizontal, float maxScaleHorizontal, float minScaleVertical, float maxScaleVertical, int sampleIndex, int p1 = 2, int p2 = 3)
{
    float u1 = haltonSample(sampleIndex, p1);
    float u2 = haltonSample(sampleIndex, p2);

    u1 = minScaleVertical + (maxScaleVertical - minScaleVertical) * u1;
    u1 = 1.0f - u1;
    u2 = minScaleHorizontal + (maxScaleHorizontal - minScaleHorizontal) * u2;

    float r = sqrt(1.0f - u1 * u1);
    float theta = 2.0f * M_PI * u2;

    return baseX * (r * cos(theta)) + baseY * (r * sin(theta)) + baseZ * u1;
}

float3 getHaltonCosHemisphereRangeSample(float3 baseX, float3 baseY, float3 baseZ, float minScaleHorizontal, float maxScaleHorizontal, float minScaleVertical, float maxScaleVertical, int sampleIndex, int p1 = 2, int p2 = 3)
{
    float u1 = haltonSample(sampleIndex, p1);
    float u2 = haltonSample(sampleIndex, p2);

    u1 = minScaleVertical + (maxScaleVertical - minScaleVertical) * u1;
    u2 = minScaleHorizontal + (maxScaleHorizontal - minScaleHorizontal) * u2;

    float r = sqrt(u1);
    float theta = 2.0f * M_PI * u2;

    return baseX * (r * cos(theta)) + baseY * (r * sin(theta)) + baseZ * sqrt(1 - u1);
}

uint index2ToIndex1(uint nHorizontal, uint indexH, uint indexV)
{
    return nHorizontal * indexV + indexH;
}

uint2 index1ToIndex2(uint nHorizontal, uint index)
{
    uint v = index / nHorizontal;
    uint h = index - nHorizontal * v;
    return uint2(h, v);
}