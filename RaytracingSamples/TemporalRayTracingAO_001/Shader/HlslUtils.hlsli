// Utility functions for our ambient occlusion shader.

#define COLOR_LIGHT_BLUE float4(0.678431392f, 0.847058892f, 0.901960850f, 1.000000000f)

#define TRIANGLE_INDEX16_STRIDE 6	// 2(Uint16 Size) * 3(Indices Count Per Triangle) = 6 bytes
#define TRIANGLE_INDEX32_STRIDE 12	// 4(Uint32 Size) * 3(Indices Count Per Triangle) = 12 bytes

#define M_PI 3.14159265f

struct SPosNormTexTanBitan
{
	float3 Position;
	float2 Texcoord;
	float3 Normal;
	float3 Tangent;
	float3 Bitangent;
};

struct SModelMeshInfo
{
	uint IndexOffsetBytes;
	uint VertexOffsetBytes;
	uint VertexStride;
};

// Load three 16 bit indices from a byte addressed buffer.
uint3 load3x16BitIndices(ByteAddressBuffer indexBuffer, uint indexStartOffset, uint primitiveIndex)
{
	uint offsetBytes = indexStartOffset + primitiveIndex * TRIANGLE_INDEX16_STRIDE;

	uint3 indices;
	// ByteAdressBuffer loads must be aligned at a 4 byte boundary.
	// Since we need to read three 16 bit indices: { 0, 1, 2 } 
	// aligned at a 4 byte boundary as: { 0 1 } { 2 0 } { 1 2 } { 0 1 } ...
	// we will load 8 bytes (~ 4 indices { a b | c d }) to handle two possible index triplet layouts,
	// based on first index's offsetBytes being aligned at the 4 byte boundary or not:
	//  Aligned:     { 0 1 | 2 - }
	//  Not aligned: { - 0 | 1 2 }
	const uint dwordAlignedOffset = offsetBytes & ~3;
	const uint2 four16BitIndices = indexBuffer.Load2(dwordAlignedOffset);

	// Aligned: { 0 1 | 2 - } => retrieve first three 16bit indices
	if (dwordAlignedOffset == offsetBytes)
	{
		indices.x = four16BitIndices.x & 0xffff;
		indices.y = (four16BitIndices.x >> 16) & 0xffff;
		indices.z = four16BitIndices.y & 0xffff;
	}
	else // Not aligned: { - 0 | 1 2 } => retrieve last three 16bit indices
	{
		indices.x = (four16BitIndices.x >> 16) & 0xffff;
		indices.y = four16BitIndices.y & 0xffff;
		indices.z = (four16BitIndices.y >> 16) & 0xffff;
	}

	return indices;
}

// Load three 32 bit indices from a byte addressed buffer.
uint3 load3x32BitIndices(ByteAddressBuffer indexBuffer, uint indexStartOffset, uint primitiveIndex)
{
	uint offsetBytes = indexStartOffset + primitiveIndex * TRIANGLE_INDEX32_STRIDE;

	uint3 indices;
	// ByteAdressBuffer loads must be aligned at a 4 byte boundary.
	// Uint32 index is already 4-byte aligned.
	indices = indexBuffer.Load3(offsetBytes);

	return indices;
}

// Retrieve attribute at a hit position interpolated from vertex attributes using the hit's barycentrics.
float3 hitAttribute(float3 vertexAttribute[3], BuiltInTriangleIntersectionAttributes attr)
{
	return vertexAttribute[0] +
		attr.barycentrics.x * (vertexAttribute[1] - vertexAttribute[0]) +
		attr.barycentrics.y * (vertexAttribute[2] - vertexAttribute[0]);
}

// Retrieve hit world position.
float3 hitWorldPosition()
{
	return WorldRayOrigin() + RayTCurrent() * WorldRayDirection();
}

float getAngleNormalized(float3 vec0, float3 vec1)
{
	float cosAngle = dot(vec0, vec1);
	return acos(cosAngle);
}

float getRotationAngleNormalized(float3 vec0, float3 vec1)
{
    float cosAngle = dot(vec0, vec1);
    float3 crossVec = cross(vec0, vec1);
    return sign(crossVec.z) >= 0 ? acos(cosAngle) : 2.0f * M_PI - acos(cosAngle);
}

float getHorizontalRotationAngleNormalized(float3 vec0, float3 norm, float3 vec1)
{
    float3 horizontalVec1 = vec1 - dot(norm, vec1) * norm;
    horizontalVec1 = normalize(horizontalVec1);
    return getRotationAngleNormalized(vec0, horizontalVec1);
}

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

void genBaseForNormal(float3 norm, inout float3 baseX, inout float3 baseY, inout float3 baseZ)
{
	float3 bitangent = getPerpendicularVector(norm);
	float3 tangent = cross(bitangent, norm);
	baseX = normalize(tangent);
	baseY = normalize(bitangent);
	baseZ = normalize(norm);
}

uint2 vector2HemisphereIndex(uint nSquareX, uint nSquareY, float3 baseX, float3 baseZ, float3 vecNormalized)
{
    float theta = getHorizontalRotationAngleNormalized(baseX, baseZ, vecNormalized);
    float uTheta = theta / (2.0f * M_PI);

    float cosPhi = dot(baseZ, vecNormalized);
    float uPhi = 1.0f - cosPhi;

    uint x = uTheta * (nSquareX - 1);
    uint y = uPhi * (nSquareY - 1);

    return uint2(x, y);
}

float4 index2SubRangeScale(uint nSquareX, uint nSquareY, uint x, uint y)
{
	float minScaleSquareX = float(x) / float(nSquareX);
	float maxScaleSquareX = float(x + 1) / float(nSquareX);
	float minScaleSquareY = float(y) / float(nSquareY);
	float maxScaleSquareY = float(y + 1) / float(nSquareY);

	return float4(minScaleSquareX, maxScaleSquareX, minScaleSquareY, maxScaleSquareY);
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

float3 getNextHemisphereSample(float3 baseX, float3 baseY, float3 baseZ, inout uint randSeed)
{
	float u1 = nextRand(randSeed);
	float u2 = nextRand(randSeed);

	float r = sqrt(1.0f - u1 * u1);
	float theta = 2.0f * M_PI * u2;

	return baseX * (r * cos(theta)) + baseY * (r * sin(theta)) + baseZ * u1;
}

float3 getNextHemisphereRangeSample(float3 baseX, float3 baseY, float3 baseZ, float minScaleSquareX, float maxScaleSquareX, float minScaleSquareY, float maxScaleSquareY, inout uint randSeed)
{
	float u1 = nextRand(randSeed);
	float u2 = nextRand(randSeed);

	u1 = minScaleSquareY + (maxScaleSquareY - minScaleSquareY) * u1;
	u1 = 1.0f - u1;
	u2 = minScaleSquareX + (maxScaleSquareX - minScaleSquareX) * u2;

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

float3 getHaltonHemisphereRangeSample(float3 baseX, float3 baseY, float3 baseZ, float minScaleSquareX, float maxScaleSquareX, float minScaleSquareY, float maxScaleSquareY, int sampleIndex, int p1 = 2, int p2 = 3)
{
	float u1 = haltonSample(sampleIndex, p1);
	float u2 = haltonSample(sampleIndex, p2);

	u1 = minScaleSquareY + (maxScaleSquareY - minScaleSquareY) * u1;
	u1 = 1.0f - u1;
	u2 = minScaleSquareX + (maxScaleSquareX - minScaleSquareX) * u2;

	float r = sqrt(1.0f - u1 * u1);
	float theta = 2.0f * M_PI * u2;

	return baseX * (r * cos(theta)) + baseY * (r * sin(theta)) + baseZ * u1;
}