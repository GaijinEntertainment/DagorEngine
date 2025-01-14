#ifndef LENS_FLARE_INFO_HLSLI_INCLUDED
#define LENS_FLARE_INFO_HLSLI_INCLUDED

struct ManualLightFlareData
{
  float3 color;
  float fadeoutDistance;
  float4 lightPos;

  uint flags;
  uint flareConfigId;
  float exposurePowParam;
  uint padding;
};

#define MANUAL_LIGHT_FLARE_DATA_FLAGS__IS_SUN (1<<0)
#define MANUAL_LIGHT_FLARE_DATA_FLAGS__USE_OCCLUSION (1<<1)

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

struct LensFlareInstanceData
{
    float4 color_intensity;

    float2 light_screen_pos;
    float2 rotation_sin_cos;

    float2 radial_distances;
    float raw_depth;
    float padding;
};


#define LENS_FLARE_DATA_FLAGS__INVERTED_GRADIENT (1<<0)
#define LENS_FLARE_DATA_FLAGS__USE_LIGHT_COLOR (1<<1)
#define LENS_FLARE_DATA_FLAGS__AUTO_ROTATION (1<<2)
#define LENS_FLARE_DATA_FLAGS__RADIAL_DISTORTION (1<<3)
#define LENS_FLARE_DATA_FLAGS__RADIAL_DISTORTION_REL_TO_CENTER (1<<4)


// Only used for the sun for now, no reason to run more threads. Later can be increased if needed.
#define LENS_FLARE_MANUAL_LIGHT_THREADS 1
#define LENS_FLARE_DYNAMIC_LIGHT_THREADS 64

// Occlusion is calculated in an area max (LENS_FLARE_OCCLUSION_DEPTH_TEXELS x LENS_FLARE_OCCLUSION_DEPTH_TEXELS)
#define LENS_FLARE_OCCLUSION_DEPTH_TEXELS 16

#endif