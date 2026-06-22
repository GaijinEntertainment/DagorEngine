#ifndef LIGHT_MASK_INC_HLSL
#define LIGHT_MASK_INC_HLSL

// currently omni and spot light masks are the same, but it can change in the future

#ifdef __cplusplus
enum SpotLightMaskType: uint8_t
{
  SPOT_LIGHT_MASK_NONE = 0,
  SPOT_LIGHT_MASK_ALL = 0xFF,
  SPOT_LIGHT_MASK_GI = 1 << 0,
  SPOT_LIGHT_MASK_VOLFOG = 1 << 1,
  SPOT_LIGHT_MASK_LENS_FLARE = 1 << 2,
  SPOT_LIGHT_MASK_DEFAULT = SPOT_LIGHT_MASK_GI | SPOT_LIGHT_MASK_VOLFOG,
};
#else
  #define SPOT_LIGHT_MASK_NONE 0
  #define SPOT_LIGHT_MASK_ALL 0xFF
  #define SPOT_LIGHT_MASK_GI (1 << 0)
  #define SPOT_LIGHT_MASK_VOLFOG (1 << 1)
  #define SPOT_LIGHT_MASK_LENS_FLARE (1 << 2)
  #define SPOT_LIGHT_MASK_DEFAULT (SPOT_LIGHT_MASK_GI | SPOT_LIGHT_MASK_VOLFOG)
#endif


#ifdef __cplusplus
enum OmniLightMaskType: uint8_t
{
  OMNI_LIGHT_MASK_NONE = 0,
  OMNI_LIGHT_MASK_ALL = 0xFF,
  OMNI_LIGHT_MASK_GI = 1 << 0,
  OMNI_LIGHT_MASK_VOLFOG = 1 << 1,
  OMNI_LIGHT_MASK_LENS_FLARE = 1 << 2,
  OMNI_LIGHT_MASK_DEFAULT = OMNI_LIGHT_MASK_GI | OMNI_LIGHT_MASK_VOLFOG,
};
#else
  #define OMNI_LIGHT_MASK_NONE 0
  #define OMNI_LIGHT_MASK_ALL 0xFF
  #define OMNI_LIGHT_MASK_GI (1 << 0)
  #define OMNI_LIGHT_MASK_VOLFOG (1 << 1)
  #define OMNI_LIGHT_MASK_LENS_FLARE (1 << 2)
  #define OMNI_LIGHT_MASK_DEFAULT (OMNI_LIGHT_MASK_GI | OMNI_LIGHT_MASK_VOLFOG)
#endif


#endif