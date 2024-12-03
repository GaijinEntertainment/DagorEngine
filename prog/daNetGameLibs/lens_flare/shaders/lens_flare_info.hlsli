#ifndef LENS_FLARE_INFO_HLSLI_INCLUDED
#define LENS_FLARE_INFO_HLSLI_INCLUDED

struct LensFlareInfo
{
    float4 tintRGB_invMaxDist;

    float2 offset;
    float2 scale;

    float invGradient;
    float invFalloff;
    float intensity;
    uint vertexPosBufOffset;

    float2 rotationOffsetSinCos;
    float axisOffset2;
    uint flags;

    float roundness;
    float roundingCircleRadius;
    float roundingCircleOffset;
    float roundingCircleCos;

    float2 distortionScale;
    float distortionPow;
    float exposurePowParam;
};

struct LensFLarePreparedLightSource
{
    float4 color_intensity;

    float2 light_screen_pos;
    float2 rotation_sin_cos;

    float2 radial_distances;
    float2 padding;
};


#define LENS_FLARE_DATA_FLAGS__INVERTED_GRADIENT (1<<0)
#define LENS_FLARE_DATA_FLAGS__USE_LIGHT_COLOR (1<<1)
#define LENS_FLARE_DATA_FLAGS__AUTO_ROTATION (1<<2)
#define LENS_FLARE_DATA_FLAGS__RADIAL_DISTORTION (1<<3)
#define LENS_FLARE_DATA_FLAGS__RADIAL_DISTORTION_REL_TO_CENTER (1<<4)

// TODO increase when multiple flares are supported
#define LENS_FLARE_THREADS 1

#endif