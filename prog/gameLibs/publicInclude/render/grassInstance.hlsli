#ifndef GRASS_INSTANCE_HLSL_INCLUDED
#define GRASS_INSTANCE_HLSL_INCLUDED

//#define UNCOMPRESSED 1
#define MAXIMUM_COMPRESSION 1
#define GRASS_WARP_SIZE_X 16
#define GRASS_WARP_SIZE_Y 16

#define BITONIC_GROUP_WIDTH 512

#define GRASS_MAX_TYPES 32
#define GRASS_MAX_CHANNELS 64

#define FAST_GRASS_MAX_CLIPMAP_CASCADES 10

#ifdef __cplusplus
  #define INITIALIZER(...)  = __VA_ARGS__
  #define INITIALIZER4(a,b,c,d)  = {a,b,c,d}
  #define INITIALIZER2(a,b)  = {a,b}
#else
  #define INITIALIZER(a)
  #define INITIALIZER4(a,b,c,d)
  #define INITIALIZER2(a,b)
#endif

#define MAX_TYPES_PER_CHANNEL 8
struct GrassChannel
{
  float4 random_weights0 INITIALIZER4(1,0,0,0);//can be compressed to uint4 (8 halves). Sum can be not equal 1, which means there is probability of no grass
  float4 random_weights1 INITIALIZER4(0,0,0,0);//can be compressed to uint4 (8 halves). Sum can be not equal 1, which means there is probability of no grass
  float density_from_weight_mul INITIALIZER(1), density_from_weight_add INITIALIZER(0);//works for decals
  uint2 random_types INITIALIZER2(0xFFFFFFFF, 0xFFFFFFFF);
};

struct GrassType
{
  #ifdef __cplusplus
  //we can actually use only this code path, not aliasing (union) with 2 float4, but 1) it may be better on some HW (due to vectorizing), 2) I am lazy to change shader code
    float height INITIALIZER(0), ht_rnd_add INITIALIZER(0);
    float hor_size INITIALIZER(0), hor_size_rnd_add INITIALIZER(0);
    float height_from_weight_mul INITIALIZER(0.75), height_from_weight_add INITIALIZER(0.25);
    float density_from_weight_mul INITIALIZER(0.999), density_from_weight_add INITIALIZER(0);
  #else
    float4 sizes;
    float4 ht_from_w__density_from_w;
  #endif
    float vertical_angle_add INITIALIZER(-0.1), vertical_angle_mul INITIALIZER(0.2);//for vertical PI/2, 0.003
    float2 size_lod_mul INITIALIZER2(1.3, 1.3);
    float fit_range INITIALIZER(0);
    float porosity INITIALIZER(0);
    float2 reserved2 INITIALIZER2(0, 0);
};

struct GrassTypesCB
{
  GrassChannel   grass_channels[GRASS_MAX_CHANNELS];
  GrassType      grass_type_params[GRASS_MAX_TYPES];
};

struct GrassColorVS
{
  float3 mask_r_color0;
  uint grassTextureType;

  float3 mask_r_color1;
  float grassVariations;

  float3 mask_g_color0;float stiffness;
  float3 mask_g_color1;float tile_tc_x;

  float3 mask_b_color0;float resv2;
  float3 mask_b_color1;float resv3;
};

struct FastGrassType
{
  float3 mask_r_color0;
  float height;

  float3 mask_r_color1;
  float texIndex;

  float3 mask_g_color0;
  float stiffness;

  float3 mask_g_color1;
  uint w_to_height_add__height_var;

  float3 mask_b_color0;
  float reserved1;

  float3 mask_b_color1;
  float reserved2;
};

#if UNCOMPRESSED
#define GrassInstance GrassInstanceUncompressed
#else
  struct GrassInstance//should be 16 bytes aligned for performance
  {
    uint position_xz;
    uint vAngle_height;
    uint random_size_rotation;//can place skew here
    uint landColor_tv;
    uint land_normal;
    uint lodNo; //can put something useful here
    float position_y;
    uint opacity_porosity;
  };
#endif

struct GrassInstanceUncompressed
{
  float3 position;//(can be compressed)
  float random;
  float height, size, rotation;//(can be compressed)
  float3 landColor;
  uint grassType;
  uint grassVariation;//can be heavily compressed!
  float3 landNormal;
  float vAngle;
  float opacity;
  float porosity;
  uint lodNo;
  bool worldYOrientation;
};

#endif