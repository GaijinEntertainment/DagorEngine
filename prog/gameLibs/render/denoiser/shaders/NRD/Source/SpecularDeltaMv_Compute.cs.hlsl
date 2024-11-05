/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

#include "NRD.hlsli"
#include "STL.hlsli"

#include "SpecularDeltaMv_Compute.resources.hlsli"

#include "Common.hlsli"

// acos(dot(a,b)) has severe precision issues for small angles
// length(cross(a,b)) == length(a) * length(b) * sin(angle)
// dot(a,b) == length(a) * length(b) * cos(angle)
float GetAngle(float3 a, float3 b)
{
    float s = saturate(length(cross(a, b)));
    float c = saturate(dot(a, b));
    return atan2(s, c);
}

float3 LoadCandidate(float2 samplePos)
{
    return gIn_PrevDeltaSecondaryPos.SampleLevel(gLinearClamp, samplePos * gRectSizeInv, 0.f).xyz;
}

float LoadAngle(float2 samplePos, float3 primaryPos, float3 refDir)
{
    float3 candidate = LoadCandidate(samplePos);
    float3 cDir = normalize(candidate - primaryPos);
    float angle = GetAngle(cDir, refDir);
    // angle^2 behaves much better
    return angle * angle;
}

[numthreads(GROUP_X, GROUP_Y, 1)]
NRD_EXPORT void NRD_CS_MAIN(uint2 pixelPos : SV_DispatchThreadId)
{
    static const float kMotionEpsilon = 1.e-4;
    static const float kAngleEpsilon = 1.e-8;
    static const float kAngleConvergenceEpsilon = 1.e-2;
    // TODO: Expose these 2 parameters
    static const uint  kMaxNewtonMethodIterations = 5;
    static const uint  kMaxLineSearchIterations = 10;

    if (any(pixelPos >= gRectSize))
        return;

    float3 primaryPos = gIn_DeltaPrimaryPos[ pixelPos ].xyz;
    float3 secondaryPos = gIn_DeltaSecondaryPos[ pixelPos ].xyz;

    float3 mv = gIn_Mv[ WithRectOrigin( pixelPos ) ] * gMvScale.xyz;
    float2 pixelUv = float2(pixelPos + 0.5) * gRectSizeInv;
    float2 prevPixelUV = pixelUv + mv.xy;
    if( gMvScale.w != 0.0 )
        prevPixelUV = STL::Geometry::GetScreenUv( gWorldToClipPrev, primaryPos + mv );

    gOut_DeltaSecondaryPos[ pixelPos ] = secondaryPos;

    float2 initialScreenMotion = prevPixelUV - pixelUv;

    // TODO: Add detection for secondary motion.
    if (length(initialScreenMotion) >= kMotionEpsilon)
    {
        float2 prevSamplePos = prevPixelUV * gRectSize;

        float3 v = normalize(secondaryPos - primaryPos);

        float currAngle = LoadAngle(prevSamplePos, primaryPos, v);

        for (uint i = 0; i < kMaxNewtonMethodIterations; ++i)
        {
            if (currAngle < kAngleEpsilon)
            {
                gOut_DeltaMv[ pixelPos ] = prevSamplePos * gRectSizeInv - pixelUv;
                return;
            }

            const float h = 1.f;
            float da_dx   = (LoadAngle(prevSamplePos + float2(h, 0), primaryPos, v) - LoadAngle(prevSamplePos + float2(-h, 0), primaryPos, v)) / (2.f * h);
            float da_dy   = (LoadAngle(prevSamplePos + float2(0, h), primaryPos, v) - LoadAngle(prevSamplePos + float2(0, -h), primaryPos, v)) / (2.f * h);
            float d2a_dx2 = (LoadAngle(prevSamplePos + float2(h, 0), primaryPos, v) - 2 * currAngle + LoadAngle(prevSamplePos + float2(-h, 0), primaryPos, v)) / (h * h);
            float d2a_dy2 = (LoadAngle(prevSamplePos + float2(0, h), primaryPos, v) - 2 * currAngle + LoadAngle(prevSamplePos + float2(0, -h), primaryPos, v)) / (h * h);
            float d2a_dxdy = (
                LoadAngle(prevSamplePos + float2(h, h), primaryPos, v)
                - LoadAngle(prevSamplePos + float2(h, -h), primaryPos, v)
                - LoadAngle(prevSamplePos + float2(-h, h), primaryPos, v)
                + LoadAngle(prevSamplePos + float2(-h, -h), primaryPos, v)
            ) / (4.f * h * h);

            // Gradient
            float f0 = da_dx;
            float f1 = da_dy;

            // Hessian
            float j00 = d2a_dx2;
            float j01 = d2a_dxdy;
            float j11 = d2a_dy2;

            // Hessian inverse
            float tmp1 = j00*j11 - j01*j01;

            // Can't find a suitable position, fallback to primary surface mvec
            if (abs(tmp1) < 1e-15)
            {
                gOut_DeltaMv[ pixelPos ] = initialScreenMotion;
                return;
            }

            tmp1 = 1.f / tmp1;
            float J00 = j11 * tmp1;
            float J01 = -j01 * tmp1;
            float J10 = -j01 * tmp1;
            float J11 = j00 * tmp1;

            float2 displacement = -float2(J00*f0 + J01*f1, J10*f0 + J11*f1);

            // Backtracking line search with Armijo condition
            float t = 1.0;
            float alpha = 0.01; // in (0, 0.5]
            float beta = 0.5; // in (0, 1)
            float2 grad = float2(f0, f1);

            float tmp2 = alpha * dot(grad, displacement);
            uint iter = 0;
            while (LoadAngle(prevSamplePos + t * displacement, primaryPos, v) > (currAngle + t * tmp2) && iter < kMaxLineSearchIterations)
            {
                t = beta * t;
                ++iter;
            }

            prevSamplePos += t * displacement;
        }

        if (currAngle < kAngleConvergenceEpsilon)
        {
            gOut_DeltaMv[ pixelPos ] = prevSamplePos * gRectSizeInv - pixelUv;
            return;
        }
    }

    gOut_DeltaMv[ pixelPos ] = initialScreenMotion;
}
