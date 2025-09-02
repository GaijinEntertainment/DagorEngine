#ifndef ANIMCHAR_ADDITIONAL_DATA_TYPES_INCLUDED
#define ANIMCHAR_ADDITIONAL_DATA_TYPES_INCLUDED

#define AAD_PLANE_CUTTING     0
#define AAD_VEHICLE_TRACKS    0
#define AAD_CAPSULE_APPROX    0
#define AAD_PORTAL_INDEX_TM   0
#define AAD_MODEL_TM          1
#define AAD_CLOTH_WIND_PARAMS 2
#define AAD_VEHICLE_CAMO      2
#define AAD_DECALS_TYPE       3
#define AAD_MATERIAL_PARAMS   4

// raw data, passed directly to rendering, can overlap with each other (unless there is a design error in shaders)
#define AAD_RAW_XRAY_INDEX          6
#define AAD_RAW_ANIM_TO_TEX_PARAMS  6
#define AAD_RAW_PLACING_COLOR       6
#define AAD_RAW_INITIAL_TM__HASHVAL 6

// todo: should be splitted into separate parameters. Now there are: burning, projective blood decals, torn wounds
#define AAD_HUMAN_COMBINED_PARAMS 7

// 2 float4 rows for 2*4*32 bits, for masking a maximum of 256 bones
#define ADDITIONAL_BONE_MTX_OFFSET 2

#endif