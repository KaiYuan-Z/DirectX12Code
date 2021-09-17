#define TRIANGLE_INDEX16_STRIDE 6	// 2(Uint16 Size) * 3(Indices Count Per Triangle) = 6 bytes
#define TRIANGLE_INDEX32_STRIDE 12	// 4(Uint32 Size) * 3(Indices Count Per Triangle) = 12 bytes

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
