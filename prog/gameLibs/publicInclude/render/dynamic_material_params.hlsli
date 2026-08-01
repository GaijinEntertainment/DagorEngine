#ifndef DYNAMIC_MATERIAL_PARAMS_INCLUDED
#define DYNAMIC_MATERIAL_PARAMS_INCLUDED



#ifdef __cplusplus
enum DynMatType: uint32_t
{
  DMT_INVALID,
  DMT_EMISSIVE,
  DMT_METATEX
};
#else
  #define DMT_INVALID                 0
  #define DMT_EMISSIVE                1
  #define DMT_METATEX                 2
#endif


#ifdef __cplusplus
enum DynMatEmissive: uint32_t
{
  DMEM_EMISSIVE_COLOR,
  DMEM_EMISSION_ALBEDO_MULT,
  DMEM_CNT
};
#else
  #define DMEM_EMISSIVE_COLOR         0
  #define DMEM_EMISSION_ALBEDO_MULT   1
  #define DMEM_CNT                    2
#endif




#endif