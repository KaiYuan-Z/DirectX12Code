#include "MathUtil.hlsl"



namespace CrossBilateral
{
    namespace Normal
    {
        struct Parameters
        {
            float Sigma;
            float SigmaExponent;
        };

        // Get cross bilateral normal based weights.
        float4 GetWeights(
            in float3 TargetNormal,
            in float3 SampleNormals[4],
            in Parameters Params)
        {
            float4 NdotSampleN = float4(
                dot(TargetNormal, SampleNormals[0]),
                dot(TargetNormal, SampleNormals[1]),
                dot(TargetNormal, SampleNormals[2]),
                dot(TargetNormal, SampleNormals[3]));

            // Apply adjustment scale to the dot product. 
            // Values greater than 1 increase tolerance scale 
            // for unwanted inflated normal differences,
            // such as due to low-precision normal quantization.
            NdotSampleN *= Params.Sigma;

            float4 normalWeights = pow(saturate(NdotSampleN), Params.SigmaExponent);

            return normalWeights;
        }
    }

    // Linear depth.
    namespace Depth
    {
        struct Parameters
        {
            float WeightCutoff;
        };
               
        float4 GetWeights(
            in float TargetDepth,
            in float4 SampleDepths,
            in float2 Ddxy,
            in Parameters Params)
        {
            float fwidthZ = dot(1, abs(Ddxy));
            float4 weights = (fwidthZ + 1e-4) / (abs(SampleDepths - TargetDepth) + 1e-4);
            weights *= (weights > 0.5f);
            return weights;
        }
    }

    namespace Bilinear
    {
        // TargetOffset - offset from the top left ([0,0]) sample of the quad samples.
        float4 GetWeights(in float2 TargetOffset)
        {
            float4 bilinearWeights =
                float4(
                    (1 - TargetOffset.x) * (1 - TargetOffset.y),
                    TargetOffset.x * (1 - TargetOffset.y),
                    (1 - TargetOffset.x) * TargetOffset.y,
                    TargetOffset.x * TargetOffset.y);

            return bilinearWeights;
        }
    }

    namespace BilinearDepthNormal
    {
        struct Parameters
        {
            Normal::Parameters Normal;
            Depth::Parameters Depth;
        };

        float4 GetWeights(
            in float TargetDepth,
            in float3 TargetNormal,
            in float2 TargetOffset,
            in float4 SampleDepths,
            in float3 SampleNormals[4],
            in float2 Ddxy,
            Parameters Params)
        {
            float4 bilinearWeights = Bilinear::GetWeights(TargetOffset);
            float4 depthWeights = Depth::GetWeights(TargetDepth, SampleDepths, Ddxy, Params.Depth);
            float4 normalWeights = Normal::GetWeights(TargetNormal, SampleNormals, Params.Normal);

            return bilinearWeights * depthWeights * normalWeights;
        }
    }
}