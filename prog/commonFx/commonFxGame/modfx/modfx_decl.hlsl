#include "modfx_gpu_placement_flags.hlsl"

#ifndef DAFX_MODFX_DECL_HLSL
#define DAFX_MODFX_DECL_HLSL

// decl/mods sizes
#define MODFX_RDECL_BITS 32
#define MODFX_SDECL_BITS 32

#define MODFX_RMODS_BITS 64
#define MODFX_SMODS_BITS 64

// helpers
#define DAFX_PRELOAD_PARENT_DATA 1
#define DAFX_PRELOAD_PARENT_REN_STRUCT ModfxParentRenData
#define DAFX_PRELOAD_PARENT_SIM_STRUCT ModfxParentSimData

#ifdef __cplusplus
  #define ModfxParentRenData_cref const ModfxParentRenData&
  #define ModfxParentSimData_cref const ModfxParentSimData&
#else
  #define ModfxParentRenData_cref ModfxParentRenData
  #define ModfxParentSimData_cref ModfxParentSimData
#endif

#define MODFX_PREBAKE_CURVE_STEPS_LIMIT 32
#define MODFX_PREBAKE_GRAD_STEPS_LIMIT 16

#define DECL_ENABLED(p, a) ( (p) & ( 1 << (a) ) )
#define FLAG_ENABLED(p, a) ( (p) & ( 1 << (a) ) )
#define MOD_ENABLED(p, a) ( p[((a)/32)] & ( 1 << ((a)%32) ) )

// if size changed, also change decl/mods in ModfxParentRenData/ModfxParentSimData
G_STATIC_ASSERT( MODFX_RDECL_BITS <= 32 );
G_STATIC_ASSERT( MODFX_SDECL_BITS <= 32 );
G_STATIC_ASSERT( MODFX_RMODS_BITS <= 64 );
G_STATIC_ASSERT( MODFX_SMODS_BITS <= 64 );

// render decl
#define MODFX_RDECL_POS 0
#define MODFX_RDECL_POS_SIZE 12
#define MODFX_RDECL_POS_ENABLED(a) DECL_ENABLED(a, MODFX_RDECL_POS)

#define MODFX_RDECL_POS_OFFSET 1
#define MODFX_RDECL_POS_OFFSET_SIZE 12
#define MODFX_RDECL_POS_OFFSET_ENABLED(a) DECL_ENABLED(a, MODFX_RDECL_POS_OFFSET)

#define MODFX_RDECL_RADIUS 2
#define MODFX_RDECL_RADIUS_SIZE 4 // TODO: pack
#define MODFX_RDECL_RADIUS_ENABLED(a) DECL_ENABLED(a, MODFX_RDECL_RADIUS)

#define MODFX_RDECL_COLOR 3
#define MODFX_RDECL_COLOR_SIZE 4
#define MODFX_RDECL_COLOR_ENABLED(a) DECL_ENABLED(a, MODFX_RDECL_COLOR)

#define MODFX_RDECL_ANGLE 4
#define MODFX_RDECL_ANGLE_SIZE 4 // TODO: pack
#define MODFX_RDECL_ANGLE_ENABLED(a) DECL_ENABLED(a, MODFX_RDECL_ANGLE)

#define MODFX_RDECL_UP_VEC 5
#define MODFX_RDECL_UP_VEC_SIZE 4
#define MODFX_RDECL_UP_VEC_ENABLED(a) DECL_ENABLED(a, MODFX_RDECL_UP_VEC)

#define MODFX_RDECL_RIGHT_VEC 6
#define MODFX_RDECL_RIGHT_VEC_SIZE 4
#define MODFX_RDECL_RIGHT_VEC_ENABLED(a) DECL_ENABLED(a, MODFX_RDECL_RIGHT_VEC)

#define MODFX_RDECL_VELOCITY_LENGTH 7
#define MODFX_RDECL_VELOCITY_LENGTH_SIZE 4
#define MODFX_RDECL_VELOCITY_LENGTH_ENABLED(a) DECL_ENABLED(a, MODFX_RDECL_VELOCITY_LENGTH)

#define MODFX_RDECL_UNIQUE_ID 8
#define MODFX_RDECL_UNIQUE_ID_SIZE 4
#define MODFX_RDECL_UNIQUE_ID_ENABLED(a) DECL_ENABLED(a, MODFX_RDECL_UNIQUE_ID)

#define MODFX_RDECL_FRAME_IDX 9
#define MODFX_RDECL_FRAME_IDX_SIZE 1
#define MODFX_RDECL_FRAME_IDX_ENABLED(a) DECL_ENABLED(a, MODFX_RDECL_FRAME_IDX)

#define MODFX_RDECL_FRAME_FLAGS 10
#define MODFX_RDECL_FRAME_FLAGS_SIZE 1
#define MODFX_RDECL_FRAME_FLAGS_ENABLED(a) DECL_ENABLED(a, MODFX_RDECL_FRAME_FLAGS)

#define MODFX_RDECL_FRAME_BLEND 11
#define MODFX_RDECL_FRAME_BLEND_SIZE 1
#define MODFX_RDECL_FRAME_BLEND_ENABLED(a) DECL_ENABLED(a, MODFX_RDECL_FRAME_BLEND)

#define MODFX_RDECL_LIFE_NORM 12
#define MODFX_RDECL_LIFE_NORM_SIZE 1
#define MODFX_RDECL_LIFE_NORM_ENABLED(a) DECL_ENABLED(a, MODFX_RDECL_LIFE_NORM)

#define MODFX_RDECL_EMISSION_FADE 13
#define MODFX_RDECL_EMISSION_FADE_SIZE 1
#define MODFX_RDECL_EMISSION_FADE_ENABLED(a) DECL_ENABLED(a, MODFX_RDECL_EMISSION_FADE)

#define MODFX_RDECL_PACKED_MASK ( ( 1 << MODFX_RDECL_FRAME_IDX )   | \
                                  ( 1 << MODFX_RDECL_FRAME_FLAGS ) | \
                                  ( 1 << MODFX_RDECL_FRAME_BLEND ) | \
                                  ( 1 << MODFX_RDECL_LIFE_NORM )   | \
                                  ( 1 << MODFX_RDECL_EMISSION_FADE ) )

// simulation decl
#define MODFX_SDECL_LIFE 0
#define MODFX_SDECL_LIFE_SIZE 4
#define MODFX_SDECL_LIFE_ENABLED(a) DECL_ENABLED(a, MODFX_SDECL_LIFE)

#define MODFX_SDECL_VELOCITY 1
#define MODFX_SDECL_VELOCITY_SIZE 12
#define MODFX_SDECL_VELOCITY_ENABLED(a) DECL_ENABLED(a, MODFX_SDECL_VELOCITY)

#define MODFX_SDECL_EMISSION_COLOR 2
#define MODFX_SDECL_EMISSION_COLOR_SIZE 4
#define MODFX_SDECL_EMISSION_COLOR_ENABLED(a) DECL_ENABLED(a, MODFX_SDECL_EMISSION_COLOR)

#define MODFX_SDECL_SIM_FLAGS 3
#define MODFX_SDECL_SIM_FLAGS_SIZE 4
#define MODFX_SDECL_SIM_FLAGS_ENABLED(a) DECL_ENABLED(a, MODFX_SDECL_SIM_FLAGS)

#define MODFX_SDECL_COLLISION_TIME 4
#define MODFX_SDECL_COLLISION_TIME_SIZE 4 // TODO: can use 1 byte only, but needs to be combined with something else
#define MODFX_SDECL_COLLISION_TIME_ENABLED(a) DECL_ENABLED(a, MODFX_SDECL_COLLISION_TIME)

#define MODFX_SDECL_RND_SEED 8
#define MODFX_SDECL_RND_SEED_SIZE 4
#define MODFX_SDECL_RND_SEED_ENABLED(a) DECL_ENABLED(a, MODFX_SDECL_RND_SEED)

// assumes
#undef MODFX_RDECL_POS_ENABLED
#define MODFX_RDECL_POS_ENABLED(a) true

#undef MODFX_RDECL_RADIUS_ENABLED
#define MODFX_RDECL_RADIUS_ENABLED(a) true

#undef MODFX_SDECL_RND_SEED_ENABLED
#define MODFX_SDECL_RND_SEED_ENABLED(a) true

// unpacked part
struct ModfxRenData
{
  float3 pos;
  float3 pos_offset;
  float radius;
  float4 color;
  float angle;

  uint frame_idx;
  uint frame_flags;
  float frame_blend;

  float life_norm;
  float emission_fade;

  float3 up_vec;
  float3 right_vec;
  float velocity_length;

  uint unique_id;
};

struct ModfxSimData
{
  float life_norm;
  uint rnd_seed;
  uint flags;
  float3 velocity;
  float4 em_color;
  float collision_time_norm;
};

// common data
struct ModfxDeclFrameInfo
{
  uint frames_x;
  uint frames_y;
  uint boundary_id_offset;
  float inv_scale;
};
#define MODFX_FRAME_FLAGS_FLIP_X ( 1 << 0 )
#define MODFX_FRAME_FLAGS_FLIP_Y ( 1 << 1 )
#define MODFX_FRAME_FLAGS_DISABLE_LOOP ( 1 << 2 )
#define MODFX_FRAME_FLAGS_RANDOM_FLIP_X ( 1 << 3 )
#define MODFX_FRAME_FLAGS_RANDOM_FLIP_Y ( 1 << 4 )

#define MODFX_SIM_FLAGS_COLLIDED 1
#define MODFX_SIM_FLAGS_SHOULD_STOP_ROTATION 2

struct ModfxDeclPosInitBox
{
  float volume;
  float3 offset;
  float3 dims;
};

struct ModfxDeclPosInitCone
{
  float volume;
  float3 offset;
  float3 vec;
  float h1;
  float h2;
  float rad;
  float random_burst;
};

struct ModfxDeclPosInitSphere
{
  float volume;
  float3 offset;
  float radius;
};

struct ModfxDeclPosInitCylinder
{
  float volume;
  float3 offset;
  float3 vec;
  float radius;
  float height;
  float random_burst;
};

struct ModfxDeclPosInitSphereSector
{
  float3 vec;
  float radius;
  float sector;
  float volume;
  float random_burst;
};

struct ModfxDeclPosInitGpuPlacement
{
  uint flags; // MODFX_GPU_PLACEMENT_ flags
  float height_threshold;
};

struct ModfxDeclRenderPlacementParams
{
  uint flags; // MODFX_GPU_PLACEMENT_ flags
  float placement_threshold;
  float align_normals_offset;
};

struct ModfxDeclRadiusInitRnd
{
  float rad_min;
  float rad_max;
};

struct ModfxDeclColorInitRnd
{
  uint color_min;
  uint color_max;
};

struct ModfxColorEmission
{
  uint mask;
  float value;
};

struct ModfxDepthMask
{
  float depth_softness_rcp;
  float znear_softness_rcp;
  float znear_clip_offset;
};

struct ModfxDeclRotationInitRnd
{
  float angle_min;
  float angle_max;
};

struct ModfxDeclRotationDynamic
{
  float vel_min;
  float vel_max;
};

struct ModfxDeclDragInit
{
  float drag_coeff;
  float drag_to_rad_k;
};

struct ModfxDeclVelocity
{
  float vel_min;
  float vel_max;
  float vec_rnd;
};

struct ModfxDeclVelocityCone
{
  float3 yaxis;
  float3 origin;
  float border_cos;
  float center_power;
  float border_power;
};

struct ModfxDeclVelocityForceFieldVortex
{
  float3 axis_direction;
  float direction_rnd;
  float3 axis_position;
  float3 position_rnd;
  float rotation_speed_min;
  float rotation_speed_max;
  float pull_speed_min;
  float pull_speed_max;
};

struct ModfxDeclFrameInit
{
  uint start_frame_min;
  uint start_frame_max;
  uint flags;
};

struct ModfxDeclFrameAnimInit
{
  float speed_min;
  float speed_max;
};

#define MODFX_COLLIDE_WITH_DEPTH         0x1
#define MODFX_COLLIDE_WITH_DEPTH_ABOVE   0x2
#define MODFX_COLLIDE_WITH_HMAP          0x4
#define MODFX_COLLIDE_WITH_WATER         0x8
#define MODFX_STOP_ROTATION_ON_COLLISION 0x10

struct ModfxDeclCollision
{
  float radius_k;
  float reflect_power;
  float reflect_energy;
  float emitter_deadzone;
  uint  collide_flags;
  float fadeout_radius_min;
  float fadeout_radius_max;
};

struct ModfxDeclCollisionDecay
{
  float collision_decay_scale;
};

struct ModfxDeclWind
{
  float directional_force;
  float directional_freq;

  float turbulence_force;
  float turbulence_freq;

  float impulse_force;
};

struct ModfxDeclCameraVelocity
{
  float velocity_weight;
};

struct ModfxDeclShapeStaticAlignedInit
{
  float3 up_vec;
  float3 right_vec;
};

struct ModfxDeclShapeVelocityMotion
{
  float length_mul;
  float2 velocity_clamp;
};

struct ModfxDeclShapeStaticAligned
{
  float cross_fade_mul;
  uint cross_fade_pow;
  float cross_fade_threshold;
};

struct ModfxDeclLighting
{
  dafx_byte type;
  dafx_byte translucency;
  dafx_byte normal_softness;
  dafx_byte specular_power;

  dafx_byte specular_strength;
  dafx_byte sphere_normal_power;
  dafx_byte un0, un1;
  float sphere_normal_radius;
};

struct ModfxDeclPartTrimming
{
  int steps;
  float fade_mul;
  float fade_pow;
};

struct ModfxDeclRibbonParams
{
  uint uv_tile;
  float2 side_fade_params;
  float2 head_fade_params;
};

struct ModfxDeclVolfogInjectionParams
{
  float weight_rgb;
  float weight_alpha;
};

struct ModfxDeclVolShapeParams
{
  float thickness;
  float radius_pow;
};

struct ModfxDeclDistortionFadeColorColorStrength
{
  float fadeRange;
  float fadePower;
  uint  color;
  float colorStrength;
};

struct ModfxForceFieldNoise
{
  uint usePosOffset;
  float posScale;
  float powerScale;
  float powerRnd;
  float powerPerPartRnd;
};

#define MODFX_LIGHTING_TYPE_UNIFORM 0
#define MODFX_LIGHTING_TYPE_DISC 1
#define MODFX_LIGHTING_TYPE_SPHERE 2
#define MODFX_LIGHTING_TYPE_NORMALMAP 3

struct ModfxDeclExternalOmnilight
{
  float3 pos;
  float3 color;
  float radius;
};

#define MODFX_TRAIL_FLAG_LAST_POS_VALID 1
#define MODFX_TRAIL_FLAG_EMITTED_THIS_FRAME 2

// service data
#define MODFX_SERVICE_TRAIL_SIZE (3*4 + 3*4 + 1*4)
struct ModfxDeclServiceTrail
{
  float3 last_emitter_pos;
  float3 prev_last_emitter_pos;
  uint flags;
};

#define MODFX_SERVICE_UNIQUE_ID_SIZE (1*4)
struct ModfxDeclServiceUniqueId
{
  uint particles_emitted;
};

struct ModfxDeclAlphaByVelocity
{
  float vel_max;
  float vel_min;
  float inv_vel_diff;
  float neg_minvel_div_by_diff;
};

#ifdef __cplusplus
static_assert(sizeof(ModfxDeclServiceTrail) == MODFX_SERVICE_TRAIL_SIZE);
static_assert(sizeof(ModfxDeclServiceUniqueId) == MODFX_SERVICE_UNIQUE_ID_SIZE);
#endif

// flags
#define MODFX_RFLAG_USE_ETM_AS_WTM 0
#define MODFX_RFLAG_COLOR_USE_ALPHA_THRESHOLD 1
#define MODFX_RFLAG_FRAME_ANIMATED_FLIPBOOK 2
#define MODFX_RFLAG_REVERSE_ORDER 3
#define MODFX_RFLAG_TEX_COLOR_REMAP_APPLY_BASE_COLOR 4
#define MODFX_RFLAG_TEX_COLOR_REMAP_SECOND_MASK 5
#define MODFX_RFLAG_TEX_COLOR_REMAP_SECOND_MASK_APPLY_BASE_COLOR 6
#define MODFX_RFLAG_OMNI_LIGHT_ENABLED 7
#define MODFX_RFLAG_LIGHTING_SPECULART_ENABLED 8
#define MODFX_RFLAG_USE_UV_ROTATION 9
#define MODFX_RFLAG_LIGHTING_AMBIENT_ENABLED 10
#define MODFX_RFLAG_DEPTH_MASK_USE_PART_RADIUS 11
#define MODFX_RFLAG_EXTERNAL_LIGHTS_ENABLED 12
#define MODFX_RFLAG_RIBBON_UV_STATIC 13
#define MODFX_RFLAG_GAMMA_CORRECTION 14

#define MODFX_RFLAG_BLEND_ADD 29
#define MODFX_RFLAG_BLEND_ABLEND 30
#define MODFX_RFLAG_BLEND_PREMULT 31

// simulation modules

// note: hlsl optimiizer is going haywire and exec both result, shifting cur_ofs even if condition is false
#ifdef __cplusplus
#define _LL(a) data.mods_offsets[a] = MOD_ENABLED( data.mods, a ) ? dafx_load_1ui( buf, cur_ofs ) + ref_ofs : 0
#else
#define _LL(a) data.mods_offsets[a] = 0; if ( MOD_ENABLED( data.mods, a ) ) { data.mods_offsets[a] = dafx_load_1ui( buf, cur_ofs ) + ref_ofs; }
#endif

#define MODFX_RMOD_INIT_TM 0
#define MODFX_RMOD_FRAME_INFO 1
#define MODFX_RMOD_COLOR_EMISSION 2
#define MODFX_RMOD_THERMAL_EMISSION 3
#define MODFX_RMOD_THERMAL_EMISSION_FADE 4
#define MODFX_RMOD_TEX_COLOR_MATRIX 5
#define MODFX_RMOD_TEX_COLOR_REMAP 6
#define MODFX_RMOD_TEX_COLOR_REMAP_DYNAMIC 7
#define MODFX_RMOD_DEPTH_MASK 8
#define MODFX_RMOD_CUSTOM_ASPECT 9
#define MODFX_RMOD_CAMERA_OFFSET 10
#define MODFX_RMOD_PIVOT_OFFSET 11
#define MODFX_RMOD_SCREEN_CLAMP 12
#define MODFX_RMOD_SHAPE_SCREEN 13
#define MODFX_RMOD_SHAPE_VIEW_POS 14
#define MODFX_RMOD_SHAPE_STATIC_ALIGNED 15
#define MODFX_RMOD_SHAPE_ADAPTIVE_ALIGNED 16
#define MODFX_RMOD_SHAPE_VELOCITY_MOTION 17
#define MODFX_RMOD_OMNI_LIGHT_INIT 18
#define MODFX_RMOD_LIGHTING_INIT 19
#define MODFX_RMOD_DISTORTION_STRENGTH 20
#define MODFX_RMOD_DISTORTION_FADE_COLOR_COLORSTRENGTH 21
#define MODFX_RMOD_RIBBON_PARAMS 22
#define MODFX_RMOD_VOLSHAPE_PARAMS 23
#define MODFX_RMOD_RENDER_PLACEMENT_PARAMS 24
#define MODFX_RMOD_VOLFOG_INJECTION 25

#define MODFX_RMOD_TOTAL_COUNT 26

struct ModfxParentRenData
{
  uint decls;
  uint2 mods;
  uint flags;
  uint mods_offsets[MODFX_RMOD_TOTAL_COUNT];
};

void dafx_preload_parent_ren_data( BufferData_cref buf, uint parent_rofs, DAFX_OREF(ModfxParentRenData) data )
{
  uint ref_ofs = parent_rofs;
  uint cur_ofs = ref_ofs;

  data.decls = dafx_load_1ui( buf, cur_ofs );
  data.mods = dafx_load_2ui( buf, cur_ofs );
  data.flags = dafx_load_1ui( buf, cur_ofs );

  // mods without data
#ifndef __cplusplus
  data.mods_offsets[MODFX_RMOD_SHAPE_SCREEN] = 0;
  data.mods_offsets[MODFX_RMOD_SHAPE_VIEW_POS] = 0;
  data.mods_offsets[MODFX_RMOD_SHAPE_ADAPTIVE_ALIGNED] = 0;
  data.mods_offsets[MODFX_RMOD_SHAPE_VELOCITY_MOTION] = 0;
#endif

  // mods with data
  _LL( MODFX_RMOD_INIT_TM );
  _LL( MODFX_RMOD_FRAME_INFO );
  _LL( MODFX_RMOD_COLOR_EMISSION );
  _LL( MODFX_RMOD_THERMAL_EMISSION );
  _LL( MODFX_RMOD_THERMAL_EMISSION_FADE );
  _LL( MODFX_RMOD_TEX_COLOR_MATRIX );
  _LL( MODFX_RMOD_TEX_COLOR_REMAP );
  _LL( MODFX_RMOD_TEX_COLOR_REMAP_DYNAMIC );
  _LL( MODFX_RMOD_DEPTH_MASK );
  _LL( MODFX_RMOD_CUSTOM_ASPECT );
  _LL( MODFX_RMOD_CAMERA_OFFSET );
  _LL( MODFX_RMOD_PIVOT_OFFSET );
  _LL( MODFX_RMOD_SCREEN_CLAMP );
  // _LL( MODFX_RMOD_SHAPE_SCREEN );
  // _LL( MODFX_RMOD_SHAPE_VIEW_POS );
  _LL( MODFX_RMOD_SHAPE_STATIC_ALIGNED );
  // _LL( MODFX_RMOD_SHAPE_ADAPTIVE_ALIGNED );
  // _LL( MODFX_RMOD_SHAPE_VELOCITY_MOTION );
  _LL( MODFX_RMOD_OMNI_LIGHT_INIT );
  _LL( MODFX_RMOD_LIGHTING_INIT );
  _LL( MODFX_RMOD_DISTORTION_STRENGTH );
  _LL( MODFX_RMOD_DISTORTION_FADE_COLOR_COLORSTRENGTH );
  _LL( MODFX_RMOD_RIBBON_PARAMS );
  _LL( MODFX_RMOD_VOLSHAPE_PARAMS );
  _LL( MODFX_RMOD_RENDER_PLACEMENT_PARAMS );
  _LL( MODFX_RMOD_VOLFOG_INJECTION );
}

#define MODFX_SMOD_LIFE_RND 0
#define MODFX_SMOD_LIFE_OFFSET 1

#define MODFX_SMOD_POS_INIT_BOX 2
#define MODFX_SMOD_POS_INIT_CONE 3
#define MODFX_SMOD_POS_INIT_SPHERE 4
#define MODFX_SMOD_POS_INIT_CYLINDER 5
#define MODFX_SMOD_POS_INIT_SPHERE_SECTOR 6

#define MODFX_SMOD_POS_INIT_GPU_PLACEMENT 7

#define MODFX_SMOD_RADIUS_INIT 8
#define MODFX_SMOD_RADIUS_INIT_RND 9
#define MODFX_SMOD_RADIUS_OVER_PART_LIFE 10

#define MODFX_SMOD_COLOR_INIT 11
#define MODFX_SMOD_COLOR_INIT_RND 12
#define MODFX_SMOD_COLOR_OVER_PART_LIFE_GRAD 13
#define MODFX_SMOD_COLOR_OVER_PART_LIFE_CURVE 14

#define MODFX_SMOD_COLOR_EMISSION_OVER_PART_LIFE 15

#define MODFX_SMOD_COLOR_OVER_PART_IDX_CURVE 16

#define MODFX_SMOD_ROTATION_INIT 17
#define MODFX_SMOD_ROTATION_INIT_RND 18
#define MODFX_SMOD_ROTATION_DYNAMIC 19
#define MODFX_SMOD_ROTATION_OVER_PART_LIFE 20

#define MODFX_SMOD_MASS_INIT 21
#define MODFX_SMOD_DRAG_INIT 22
#define MODFX_SMOD_DRAG_OVER_PART_LIFE 23

#define MODFX_SMOD_VELOCITY_INIT_ADD_STATIC_VEC 24
#define MODFX_SMOD_VELOCITY_INIT 25
#define MODFX_SMOD_VELOCITY_INIT_POINT 26
#define MODFX_SMOD_VELOCITY_INIT_VEC 27
#define MODFX_SMOD_VELOCITY_INIT_START_SHAPE 28

#define MODFX_SMOD_VELOCITY_ADD 29
#define MODFX_SMOD_VELOCITY_ADD_POINT 30
#define MODFX_SMOD_VELOCITY_ADD_VEC 31
#define MODFX_SMOD_VELOCITY_ADD_CONE 32

#define MODFX_SMOD_VELOCITY_DECAY 33
#define MODFX_SMOD_VELOCITY_SCENE_COLLISION 34
#define MODFX_SMOD_VELOCITY_LOCAL_GRAVITY 35

#define MODFX_SMOD_VELOCITY_WIND 36
#define MODFX_SMOD_VELOCITY_WIND_ATTEN 37

#define MODFX_SMOD_FRAME_INIT 38
#define MODFX_SMOD_FRAME_ANIM_INIT 39
#define MODFX_SMOD_FRAME_ANIM_OVER_PART_LIFE 40

#define MODFX_SMOD_SHAPE_STATIC_ALIGNED_INIT 41
#define MODFX_SMOD_SHAPE_ADAPTIVE_ALIGNED_INIT 42
#define MODFX_SMOD_SHAPE_VELOCITY_MOTION_INIT 43

#define MODFX_SMOD_PART_TRIMMING 44

#define MODFX_SMOD_COLOR_EMISSION_OVERRIDE 45

#define MODFX_SMOD_VELOCITY_FORCE_FIELD_VORTEX 46
#define MODFX_SMOD_VELOCITY_FORCE_FIELD_VORTEX_ROTATION_SPEED 47
#define MODFX_SMOD_VELOCITY_FORCE_FIELD_VORTEX_PULL_SPEED 48

#define MODFX_SMOD_VELOCITY_FORCE_FIELD_NOISE 49
#define MODFX_SMOD_VELOCITY_FORCE_FIELD_NOISE_CURVE 50

#define MODFX_SMOD_COLLISION_DECAY 51

#define MODFX_SMOD_ALPHA_BY_VELOCITY 52
#define MODFX_SMOD_ALPHA_BY_VELOCITY_CURVE 53

#define MODFX_SMOD_CAMERA_VELOCITY 54

#define MODFX_SMOD_GRAVITY_TM 55

#define MODFX_SMOD_TOTAL_COUNT 56

#define MODFX_SFLAG_VELOCITY_APPLY_GRAVITY 0
#define MODFX_SFLAG_VELOCITY_APPLY_GRAVITY_TRANSFORM 1
#define MODFX_SFLAG_VELOCITY_USE_FORCE_RESOLVER 2
#define MODFX_SFLAG_VELOCITY_APPLY_EMITTER_TRANSFORM_TO_ADD 3
#define MODFX_SFLAG_PASS_VELOCITY_TO_RDECL 4
#define MODFX_SFLAG_PASS_UP_VEC_TO_RDECL_ONCE 5
#define MODFX_SFLAG_TRAIL_ENABLED 6
#define MODFX_SFLAG_ATTACH_LAST_PART_TO_EMITTER 7
#define MODFX_SFLAG_RADIUS_APPLY_EMITTER_TRANSFORM 8
#define MODFX_SFLAG_COLOR_GRAD_USE_PART_IDX_AS_KEY 9
#define MODFX_SFLAG_COLOR_CURVE_USE_PART_IDX_AS_KEY 10
#define MODFX_SFLAG_TOROIDAL_MOVEMENT 11
#define MODFX_SFLAG_WATER_FLOWMAP 12
#define MODFX_SFLAG_ALPHA_BY_EMITTER_VELOCITY 13
#define MODFX_SFLAG_GRAVITY_ZONE_PER_EMITTER 14
#define MODFX_SFLAG_GRAVITY_ZONE_PER_PARTICLE 15

struct ModfxParentSimData
{
  uint decls;
  uint2 mods;
  uint flags;
  uint mods_offsets[MODFX_SMOD_TOTAL_COUNT];
};

void dafx_preload_parent_sim_data( BufferData_cref buf, uint parent_sofs, DAFX_OREF(ModfxParentSimData) data )
{
  uint ref_ofs = parent_sofs;
  uint cur_ofs = ref_ofs;

  data.decls = dafx_load_1ui( buf, cur_ofs );
  data.mods = dafx_load_2ui( buf, cur_ofs );
  data.flags = dafx_load_1ui( buf, cur_ofs );

#ifndef __cplusplus
  data.mods_offsets[MODFX_SMOD_VELOCITY_INIT_START_SHAPE] = 0;
#endif

  _LL( MODFX_SMOD_LIFE_RND );
  _LL( MODFX_SMOD_LIFE_OFFSET );

  _LL( MODFX_SMOD_POS_INIT_BOX );
  _LL( MODFX_SMOD_POS_INIT_CONE );
  _LL( MODFX_SMOD_POS_INIT_SPHERE );
  _LL( MODFX_SMOD_POS_INIT_CYLINDER );
  _LL( MODFX_SMOD_POS_INIT_SPHERE_SECTOR );
  _LL( MODFX_SMOD_POS_INIT_GPU_PLACEMENT );

  _LL( MODFX_SMOD_RADIUS_INIT );
  _LL( MODFX_SMOD_RADIUS_INIT_RND );
  _LL( MODFX_SMOD_RADIUS_OVER_PART_LIFE );

  _LL( MODFX_SMOD_COLOR_INIT );
  _LL( MODFX_SMOD_COLOR_INIT_RND );
  _LL( MODFX_SMOD_COLOR_OVER_PART_LIFE_GRAD );
  _LL( MODFX_SMOD_COLOR_OVER_PART_LIFE_CURVE );
  _LL( MODFX_SMOD_COLOR_EMISSION_OVER_PART_LIFE );
  _LL( MODFX_SMOD_COLOR_OVER_PART_IDX_CURVE );

  _LL( MODFX_SMOD_ROTATION_INIT );
  _LL( MODFX_SMOD_ROTATION_INIT_RND );
  _LL( MODFX_SMOD_ROTATION_DYNAMIC );
  _LL( MODFX_SMOD_ROTATION_OVER_PART_LIFE );

  _LL( MODFX_SMOD_MASS_INIT );
  _LL( MODFX_SMOD_DRAG_INIT );
  _LL( MODFX_SMOD_DRAG_OVER_PART_LIFE );

  _LL( MODFX_SMOD_VELOCITY_INIT_ADD_STATIC_VEC );

  _LL( MODFX_SMOD_VELOCITY_INIT );
  _LL( MODFX_SMOD_VELOCITY_INIT_POINT );
  _LL( MODFX_SMOD_VELOCITY_INIT_VEC );
  // _LL( MODFX_SMOD_VELOCITY_INIT_START_SHAPE );

  _LL( MODFX_SMOD_VELOCITY_ADD );
  _LL( MODFX_SMOD_VELOCITY_ADD_POINT );
  _LL( MODFX_SMOD_VELOCITY_ADD_VEC );
  _LL( MODFX_SMOD_VELOCITY_ADD_CONE);

  _LL( MODFX_SMOD_VELOCITY_DECAY );
  _LL( MODFX_SMOD_VELOCITY_SCENE_COLLISION );
  _LL( MODFX_SMOD_VELOCITY_LOCAL_GRAVITY );

  _LL( MODFX_SMOD_VELOCITY_WIND );
  _LL( MODFX_SMOD_VELOCITY_WIND_ATTEN );

  _LL( MODFX_SMOD_FRAME_INIT );
  _LL( MODFX_SMOD_FRAME_ANIM_INIT );
  _LL( MODFX_SMOD_FRAME_ANIM_OVER_PART_LIFE );

  _LL( MODFX_SMOD_SHAPE_STATIC_ALIGNED_INIT );
  _LL( MODFX_SMOD_SHAPE_ADAPTIVE_ALIGNED_INIT );
  _LL( MODFX_SMOD_SHAPE_VELOCITY_MOTION_INIT );

  _LL( MODFX_SMOD_PART_TRIMMING );
  _LL( MODFX_SMOD_COLOR_EMISSION_OVERRIDE );

  _LL( MODFX_SMOD_VELOCITY_FORCE_FIELD_VORTEX );
  _LL( MODFX_SMOD_VELOCITY_FORCE_FIELD_VORTEX_ROTATION_SPEED );
  _LL( MODFX_SMOD_VELOCITY_FORCE_FIELD_VORTEX_PULL_SPEED );

  _LL( MODFX_SMOD_VELOCITY_FORCE_FIELD_NOISE );
  _LL( MODFX_SMOD_VELOCITY_FORCE_FIELD_NOISE_CURVE );

  _LL( MODFX_SMOD_COLLISION_DECAY );

  _LL( MODFX_SMOD_ALPHA_BY_VELOCITY );
  _LL( MODFX_SMOD_ALPHA_BY_VELOCITY_CURVE );

  _LL( MODFX_SMOD_CAMERA_VELOCITY );

  _LL( MODFX_SMOD_GRAVITY_TM );
}

#undef _LL

#endif