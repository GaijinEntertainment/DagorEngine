/*
Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.

NVIDIA CORPORATION and its licensors retain all intellectual property
and proprietary rights in and to this software, related documentation
and any modifications thereto. Any use, reproduction, disclosure or
distribution of this software and related documentation without an express
license agreement from NVIDIA CORPORATION is strictly prohibited.
*/

float4 UnpackPrevNormalRoughness(float4 packedData)
{
    float4 result;
    result.rgb = _NRD_SafeNormalize(packedData.rgb * 2.0 - 1.0);
    result.a = packedData.a;

    return result;
}

float4 PackPrevNormalRoughness(float4 normalRoughness)
{
    float4 result;
    result.rgb = normalRoughness.xyz * 0.5 + 0.5;
    result.a = normalRoughness.a;

    return result;
}

float BilinearWithCustomWeightsImmediateFloat(float s00, float s10, float s01, float s11, float4 bilinearCustomWeights)
{
    float output = s00 * bilinearCustomWeights.x;
    output += s10 * bilinearCustomWeights.y;
    output += s01 * bilinearCustomWeights.z;
    output += s11 * bilinearCustomWeights.w;

    float sumWeights = dot(bilinearCustomWeights, 1.0);
    output = sumWeights < 0.0001 ? 0 : output * rcp(sumWeights);
    return output;
}


float4 BilinearWithCustomWeightsImmediateFloat4(float4 s00, float4 s10, float4 s01, float4 s11, float4 bilinearCustomWeights)
{
    float4 output = s00 * bilinearCustomWeights.x;
    output += s10 * bilinearCustomWeights.y;
    output += s01 * bilinearCustomWeights.z;
    output += s11 * bilinearCustomWeights.w;

    float sumWeights = dot(bilinearCustomWeights, 1.0);
    output = sumWeights < 0.0001 ? 0 : output * rcp(sumWeights);
    return output;
}

float4 BilinearWithCustomWeightsFloat4(Texture2D<float4> tex0, int2 position, float4 bilinearCustomWeights)
{
    float4 output = tex0[position] * bilinearCustomWeights.x;
    output += tex0[position + int2(1, 0)] * bilinearCustomWeights.y;
    output += tex0[position + int2(0, 1)] * bilinearCustomWeights.z;
    output += tex0[position + int2(1, 1)] * bilinearCustomWeights.w;

    float sumWeights = dot(bilinearCustomWeights, 1.0);
    output = sumWeights < 0.0001 ? 0 : output * rcp(sumWeights);
    return output;
}

float3 GetCurrentWorldPosFromPixelPos(int2 pixelPos, float viewZ)
{
    float2 clipSpaceXY = ((float2)pixelPos + float2(0.5, 0.5)) * gRectSizeInv * 2.0 - 1.0;

    return (gOrthoMode == 0) ?
        viewZ * (gFrustumForward.xyz + gFrustumRight.xyz * clipSpaceXY.x - gFrustumUp.xyz * clipSpaceXY.y) :
        viewZ * gFrustumForward.xyz + gFrustumRight.xyz * clipSpaceXY.x - gFrustumUp.xyz * clipSpaceXY.y;
}

float3 GetCurrentWorldPosFromClipSpaceXY(float2 clipSpaceXY, float viewZ)
{
    return (gOrthoMode == 0) ?
        viewZ * (gFrustumForward.xyz + gFrustumRight.xyz * clipSpaceXY.x - gFrustumUp.xyz * clipSpaceXY.y) :
        viewZ * gFrustumForward.xyz + gFrustumRight.xyz * clipSpaceXY.x - gFrustumUp.xyz * clipSpaceXY.y;
}

float3 GetPreviousWorldPosFromPixelPos(int2 pixelPos, float viewZ)
{
    float2 clipSpaceXY = ((float2)pixelPos + float2(0.5, 0.5)) * (1.0 / gRectSizePrev) * 2.0 - 1.0;

    return (gOrthoMode == 0) ?
        viewZ * (gPrevFrustumForward.xyz + gPrevFrustumRight.xyz * clipSpaceXY.x - gPrevFrustumUp.xyz * clipSpaceXY.y) :
        viewZ * gPrevFrustumForward.xyz + gPrevFrustumRight.xyz * clipSpaceXY.x - gPrevFrustumUp.xyz * clipSpaceXY.y;
}

float3 GetPreviousWorldPosFromClipSpaceXY(float2 clipSpaceXY, float viewZ)
{
    return (gOrthoMode == 0) ?
        viewZ * (gPrevFrustumForward.xyz + gPrevFrustumRight.xyz * clipSpaceXY.x - gPrevFrustumUp.xyz * clipSpaceXY.y) :
        viewZ * gPrevFrustumForward.xyz + gPrevFrustumRight.xyz * clipSpaceXY.x - gPrevFrustumUp.xyz * clipSpaceXY.y;
}

float GetPlaneDistanceWeight(float3 centerWorldPos, float3 centerNormal, float centerViewZ, float3 sampleWorldPos, float threshold)
{
    float distanceToCenterPointPlane = abs(dot(sampleWorldPos - centerWorldPos, centerNormal));

    return distanceToCenterPointPlane / centerViewZ > threshold ? 0.0 : 1.0;
}

float GetPlaneDistanceWeight_Atrous(float3 centerWorldPos, float3 centerNormal, float3 sampleWorldPos, float threshold)
{
    float distanceToCenterPointPlane = abs(dot(sampleWorldPos - centerWorldPos, centerNormal));

    return distanceToCenterPointPlane < threshold ? 1.0 : 0.0;
}

float2 GetNormalWeightParams_ATrous(float roughness, float numFramesInHistory, float specularReprojectionConfidence, float normalEdgeStoppingRelaxation, float specularLobeAngleFraction, float specularLobeAngleSlack)
{
    // Relaxing normal weights if not enough frames in history
    // and if specular reprojection confidence is low
    float relaxation = saturate(numFramesInHistory / 5.0);
    relaxation *= lerp(1.0, specularReprojectionConfidence, normalEdgeStoppingRelaxation);
    float f = 0.9 + 0.1 * relaxation;

    // This is the main parameter - cone angle
    float angle = STL::ImportanceSampling::GetSpecularLobeHalfAngle(roughness, specularLobeAngleFraction);

    // Increasing angle ~10x to relax rejection of the neighbors if specular reprojection confidence is low
    angle *= 10.0 - 9.0 * relaxation;

    angle += specularLobeAngleSlack;

    angle = min(STL::Math::Pi(0.5), angle);

    return float2(angle, f);
}

float GetSpecularNormalWeight_ATrous(float2 params0, float3 n0, float3 n, float3 v0, float3 v)
{
    float cosaN = dot(n0, n);
    float cosaV = dot(v0, v);
    float cosa = min(cosaN, cosaV);
    float a = STL::Math::AcosApprox(cosa);
    a = STL::Math::SmoothStep(0.0, params0.x, a);

    return saturate(1.0 - a * params0.y);
}

float GetNormalWeightParams(float roughness, float angleFraction = 0.75)
{
    float angle = STL::ImportanceSampling::GetSpecularLobeHalfAngle(roughness, angleFraction);
    angle = 1.0 / max(angle, NRD_NORMAL_ULP);

    return angle;
}

float GetNormalWeight(float3 Ncurr, float3 Nprev, float maxAngle)
{
    float cosa = dot(Ncurr, Nprev);

    float a = 1.0 / maxAngle;
    float d = STL::Math::AcosApprox(cosa);

    float w = STL::Math::SmoothStep01(1.0 - (d * a));

    return w;
}

void BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights(
    float2 samplePos, float2 invResourceSize,
    float4 bilinearCustomWeights, bool useBicubic,
    Texture2D<float4> tex0, out float4 c0 )
{
    _BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights_Init;
    _BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights_Color( c0, tex0 );
}

void BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights(
    float2 samplePos, float2 invResourceSize,
    float4 bilinearCustomWeights, bool useBicubic,
    Texture2D<float4> tex0, out float4 c0,
    Texture2D<float4> tex1, out float4 c1 )
{
    _BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights_Init;
    _BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights_Color( c0, tex0 );
    _BicubicFilterNoCornersWithFallbackToBilinearFilterWithCustomWeights_Color( c1, tex1 );
}
