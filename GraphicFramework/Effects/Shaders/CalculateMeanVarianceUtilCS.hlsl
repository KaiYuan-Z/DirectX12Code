#include "../../Core/ShaderUtility/MathUtil.hlsl"

// Desc: Calculate Local Mean and Variance via a separable kernel and using wave intrinsics.
// Requirements:
//  - Wave lane size 16 or higher.
//  - WaveReadLaneAt() with any to any to wave read lane support.
// Supports:
//  - up to 9x9 kernels.
//  - checkerboard ON/OFF input. If enabled, outputs only for active pixels.
//     Active pixel is a pixel on the checkerboard pattern and has a valid / 
//     generated value for it. The kernel is stretched in y direction 
//    to sample only from active pixels. 
// Performance:
// - 4K, 2080Ti, 9x9 kernel: 0.37ms (separable) -> 0.305 ms (separable + wave intrinsics)

#define InvalidAOCoefficientValue -1.0f

cbuffer MeanVarianceCB : register(b0, space0)
{
    uint2 g_textureDim;
    uint g_kernelWidth;
    uint g_kernelRadius;
};

Texture2D<float> g_inValue : register(t0, space0);

#ifdef OUTPUT_MEAN
RWTexture2D<float> g_outMean : register(u0, space0);
#endif

#ifdef OUTPUT_VARIANCE
RWTexture2D<float> g_outVariance : register(u0, space0);
#endif

#ifdef OUTPUT_MEAN_VARIANCE
RWTexture2D<float2> g_outMeanVariance : register(u0, space0);
#endif

// Group shared memory cache for the row aggregated results.
groupshared uint PackedRowResultCache[16][8];            // 16bit float valueSum, squaredValueSum.
groupshared uint NumValuesCache[16][8]; 


// Load up to 16x16 pixels and filter them horizontally.
// The output is cached in Shared Memory and contains NumRows x 8 results.
void FilterHorizontally(in uint2 Gid, in uint GI)
{
    const uint2 GroupDim = uint2(8, 8);
    const uint NumValuesToLoadPerRowOrColumn = GroupDim.x + (g_kernelWidth - 1);

    // Processes the thread group as row-major 4x16, where each sub group of 16 threads processes one row.
    // Each thread loads up to 4 values, with the sub groups loading rows interleaved.
    // Loads up to 4x16x4 == 256 input values.
    uint2 GTid4x16_row0 = uint2(GI % 16, GI / 16);
    const int2 KernelBasePixel = (Gid * GroupDim - int(g_kernelRadius)) * int2(1, 1);
    const uint NumRowsToLoadPerThread = 4;
    const uint Row_BaseWaveLaneIndex = (WaveGetLaneIndex() / 16) * 16;

    [unroll]
    for (uint i = 0; i < NumRowsToLoadPerThread; i++)
    {
        uint2 GTid4x16 = GTid4x16_row0 + uint2(0, i * 4);
        if (GTid4x16.y >= NumValuesToLoadPerRowOrColumn)
        {
            if (GTid4x16.x < GroupDim.x)
            {
                NumValuesCache[GTid4x16.y][GTid4x16.x] = 0;
            }
            break;
        }

        // Load all the contributing columns for each row.
        int2 pixel = KernelBasePixel + GTid4x16 * int2(1, 1);
        float value = InvalidAOCoefficientValue;

        // The lane is out of bounds of the GroupDim + kernel, 
        // but could be within bounds of the input texture,
        // so don't read it from the texture.
        // However, we need to keep it as an active lane for a below split sum.
        if (GTid4x16.x < NumValuesToLoadPerRowOrColumn && isWithinBounds(pixel, g_textureDim))
        {
            value = g_inValue[pixel];
        }

        // Filter the values for the first GroupDim columns.
        {
            // Accumulate for the whole kernel width.
            float valueSum = 0;
            float squaredValueSum = 0;
            uint numValues = 0;

            // Since a row uses 16 lanes, but we only need to calculate the aggregate for the first half (8) lanes,
            // split the kernel wide aggregation among the first 8 and the second 8 lanes, and then combine them.
            
            // Initialize the first 8 lanes to the first cell contribution of the kernel. 
            // This covers the remainder of 1 in g_kernelWidth / 2 used in the loop below. 
            if (GTid4x16.x < GroupDim.x && value != InvalidAOCoefficientValue)
            {
                valueSum = value;
                squaredValueSum = value * value;
                numValues++;
            }

            // Get the lane index that has the first value for a kernel in this lane.
            uint Row_KernelStartLaneIndex =
                Row_BaseWaveLaneIndex
                + 1     // Skip over the already accumulated first cell of the kernel.
                + (GTid4x16.x < GroupDim.x
                    ? GTid4x16.x
                    : (GTid4x16.x - GroupDim.x) + g_kernelRadius);

            for (uint c = 0; c < g_kernelRadius; c++)
            {
                uint laneToReadFrom = Row_KernelStartLaneIndex + c;
                float cValue = WaveReadLaneAt(value, laneToReadFrom);
                if (cValue != InvalidAOCoefficientValue)
                {
                    valueSum += cValue;
                    squaredValueSum += cValue * cValue;
                    numValues++;
                }
            }
            
            // Combine the sub-results.
            uint laneToReadFrom = min(WaveGetLaneCount() - 1, Row_BaseWaveLaneIndex + GTid4x16.x + GroupDim.x);
            valueSum += WaveReadLaneAt(valueSum, laneToReadFrom);
            squaredValueSum += WaveReadLaneAt(squaredValueSum, laneToReadFrom);
            numValues += WaveReadLaneAt(numValues, laneToReadFrom);

            // Store only the valid results, i.e. first GroupDim columns.
            if (GTid4x16.x < GroupDim.x)
            {
                PackedRowResultCache[GTid4x16.y][GTid4x16.x] = float2ToUint(float2(valueSum, squaredValueSum));
                NumValuesCache[GTid4x16.y][GTid4x16.x] = numValues;
            }
        }
    }
}

void FilterVertically(uint2 DTid, in uint2 GTid)
{
    float valueSum = 0;
    float squaredValueSum = 0;
    uint numValues = 0;

    uint2 pixel = int2(DTid.x, DTid.y * 1);

    float4 val1, val2;
    // Accumulate for the whole kernel.
    for (uint r = 0; r < g_kernelWidth; r++)
    {
        uint rowID = GTid.y + r;
        uint rNumValues = NumValuesCache[rowID][GTid.x];

        if (rNumValues > 0)
        {
            float2 unpackedRowSum = uintToFloat2(PackedRowResultCache[rowID][GTid.x]);
            float rValueSum = unpackedRowSum.x;
            float rSquaredValueSum = unpackedRowSum.y;

            valueSum += rValueSum;
            squaredValueSum += rSquaredValueSum;
            numValues += rNumValues;
        }
    }

    // Calculate mean and variance.
    float invN = 1.f / max(numValues, 1);
    float mean = invN * valueSum;

    // Apply Bessel's correction to the estimated variance, multiply by N/N-1, 
    // since the true population mean is not known; it is only estimated as the sample mean.
    float besselCorrection = numValues / float(max(numValues, 2) - 1);
    float variance = besselCorrection * (invN * squaredValueSum - mean * mean);

    variance = max(0, variance);    // Ensure variance doesn't go negative due to imprecision.
    
    //variance = sqrt(variance);

#ifdef OUTPUT_MEAN
    g_outMean[pixel] = numValues > 0 ? mean : InvalidAOCoefficientValue;
#endif
    
#ifdef OUTPUT_VARIANCE
    g_outVariance[pixel] = numValues > 0 ? variance : InvalidAOCoefficientValue;
#endif
    
#ifdef OUTPUT_MEAN_VARIANCE
g_outMeanVariance[pixel] = numValues > 0 ? float2(mean, variance) : InvalidAOCoefficientValue;
#endif
}


[numthreads(8, 8, 1)]
void main(uint2 Gid : SV_GroupID, uint2 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex, uint2 DTid : SV_DispatchThreadID)
{
    FilterHorizontally(Gid, GI);
    GroupMemoryBarrierWithGroupSync();

    FilterVertically(DTid, GTid);
}
