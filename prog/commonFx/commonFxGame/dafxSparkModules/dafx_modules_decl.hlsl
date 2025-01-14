#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4100)
#pragma warning(disable:4189)
#endif


#ifdef __cplusplus
  #define SimData_ref SimData&
  #define SimData_cref const SimData&
  #define RenData_ref RenData&
  #define RenData_cref const RenData&
  #define ParentRenData_cref const ParentRenData&
  #define ParentSimData_cref const ParentSimData&
#else
  #define SimData_ref in out SimData
  #define SimData_cref SimData
  #define RenData_ref in out RenData
  #define RenData_cref RenData
  #define ParentRenData_cref ParentRenData
  #define ParentSimData_cref ParentSimData
#endif

//--- Dafx Common Modules

#ifdef SPAWN_MODULE_LINEAR_LIFE
  #define SPAWN_MODULE_LINEAR_LIFE_DECL(e) e
#else
  #define SPAWN_MODULE_LINEAR_LIFE_DECL(e) DAFX_EMPTY_STRUCT(SPAWN_MODULE_LINEAR_LIFE_EMPTY)
  #define SPAWN_MODULE_LINEAR_LIFE 0
#endif

#ifdef SPAWN_MODULE_SPHERE_VELOCITY
  #define SPAWN_MODULE_SPHERE_VELOCITY_DECL(e) e
#else
  #define SPAWN_MODULE_SPHERE_VELOCITY_DECL(e) DAFX_EMPTY_STRUCT(SPAWN_MODULE_SPHERE_VELOCITY_EMPTY)
  #define SPAWN_MODULE_SPHERE_VELOCITY 0
#endif

#ifdef SPAWN_MODULE_NOISE_VELOCITY
  #define SPAWN_MODULE_NOISE_VELOCITY_DECL(e) e
#else
  #define SPAWN_MODULE_NOISE_VELOCITY_DECL(e) DAFX_EMPTY_STRUCT(SPAWN_MODULE_NOISE_VELOCITY_EMPTY)
  #define SPAWN_MODULE_NOISE_VELOCITY 0
#endif

#ifdef SPAWN_MODULE_NOISE_POS
  #define SPAWN_MODULE_NOISE_POS_DECL(e) e
#else
  #define SPAWN_MODULE_NOISE_POS_DECL(e) DAFX_EMPTY_STRUCT(SPAWN_MODULE_NOISE_POS_EMPTY)
  #define SPAWN_MODULE_NOISE_POS 0
#endif

#ifdef SPAWN_MODULE_WIDTH
  #define SPAWN_MODULE_WIDTH_DECL(e) e
#else
  #define SPAWN_MODULE_WIDTH_DECL(e) DAFX_EMPTY_STRUCT(SPAWN_MODULE_WIDTH_EMPTY)
  #define SPAWN_MODULE_WIDTH 0
#endif

#ifdef SIM_MODULE_WIDTH_MODIFIER
  #define SIM_MODULE_WIDTH_MODIFIER_DECL(e) e
#else
  #define SIM_MODULE_WIDTH_MODIFIER_DECL(e) DAFX_EMPTY_STRUCT(SIM_MODULE_WIDTH_MODIFIER_EMPTY)
  #define SIM_MODULE_WIDTH_MODIFIER 0
#endif

#ifdef SPAWN_MODULE_CUBE_POS
  #define SPAWN_MODULE_CUBE_POS_DECL(e) e
#else
  #define SPAWN_MODULE_CUBE_POS_DECL(e) DAFX_EMPTY_STRUCT(SPAWN_MODULE_CUBE_POS_EMPTY)
  #define SPAWN_MODULE_CUBE_POS 0
#endif

#ifdef SPAWN_MODULE_VELOCITY_BIAS_POS
  #define SPAWN_MODULE_VELOCITY_BIAS_POS_DECL(e) e
#else
  #define SPAWN_MODULE_VELOCITY_BIAS_POS_DECL(e) DAFX_EMPTY_STRUCT(SPAWN_MODULE_VELOCITY_BIAS_POS_EMPTY)
  #define SPAWN_MODULE_VELOCITY_BIAS_POS 0
#endif

#ifdef SPAWN_MODULE_SEED
  #define SPAWN_MODULE_SEED_DECL(e) e
#else
  #define SPAWN_MODULE_SEED_DECL(e) DAFX_EMPTY_STRUCT(SPAWN_MODULE_SEED_EMPTY)
  #define SPAWN_MODULE_SEED 0
#endif

#ifdef SIM_MODULE_LIFE
  #define SIM_MODULE_LIFE_DECL(e) e
  #if !SPAWN_MODULE_LINEAR_LIFE
    #error SPAWN_MODULE_LINEAR_LIFE REQUIRED!
  #endif
#else
  #define SIM_MODULE_LIFE_DECL(e) DAFX_EMPTY_STRUCT(SIM_MODULE_LIFE_EMPTY)
  #define SIM_MODULE_LIFE 0
#endif

#ifdef SIM_MODULE_DEPTH_COLLISION
  #define SIM_MODULE_DEPTH_COLLISION_DECL(e) e
#else
  #define SIM_MODULE_DEPTH_COLLISION_DECL(e) DAFX_EMPTY_STRUCT(SIM_MODULE_DEPTH_COLLISION_EMPTY)
  #define SIM_MODULE_DEPTH_COLLISION 0
#endif

#ifdef SIM_MODULE_COLOR
  #define SIM_MODULE_COLOR_DECL(e) e
  #define SIM_MODULE_COLOR_DECL2(e) e
  #if !SIM_MODULE_LIFE
    #error SIM_MODULE_LIFE REQUIRED!
  #endif
#else
  #define SIM_MODULE_COLOR_DECL(e) DAFX_EMPTY_STRUCT(SIM_MODULE_COLOR_EMPTY)
  #define SIM_MODULE_COLOR_DECL2(e) DAFX_EMPTY_STRUCT(SIM_MODULE_COLOR_EMPTY2)
  #define SIM_MODULE_COLOR 0
#endif

#ifdef SIM_MODULE_GRAVITY
  #define SIM_MODULE_GRAVITY_DECL(e) e
  #if !SIM_MODULE_RESOLVE_FORCES
    #error SIM_MODULE_RESOLVE_FORCES REQUIRED!
  #endif
#else
  #define SIM_MODULE_GRAVITY_DECL(e) DAFX_EMPTY_STRUCT(SIM_MODULE_GRAVITY_EMPTY)
  #define SIM_MODULE_GRAVITY 0
#endif

#ifdef SIM_MODULE_LIFT
  #define SIM_MODULE_LIFT_DECL(e) e
  #if !SIM_MODULE_RESOLVE_FORCES
    #error SIM_MODULE_RESOLVE_FORCES REQUIRED!
  #endif
#else
  #define SIM_MODULE_LIFT_DECL(e) DAFX_EMPTY_STRUCT(SIM_MODULE_LIFT_EMPTY)
  #define SIM_MODULE_LIFT 0
#endif

#ifdef SIM_MODULE_DIRECTIONAL_WIND
  #define SIM_MODULE_DIRECTIONAL_WIND_DECL(e) e
  #if !SIM_MODULE_RESOLVE_FORCES
    #error SIM_MODULE_RESOLVE_FORCES REQUIRED!
  #endif
#else
  #define SIM_MODULE_DIRECTIONAL_WIND_DECL(e) DAFX_EMPTY_STRUCT(SIM_MODULE_DIRECTIONAL_WIND_EMPTY)
  #define SIM_MODULE_DIRECTIONAL_WIND 0
#endif

#ifdef SIM_MODULE_TURBULENT_WIND
  #define SIM_MODULE_TURBULENT_WIND_DECL(e) e
  #if !SIM_MODULE_RESOLVE_FORCES
    #error SIM_MODULE_RESOLVE_FORCES REQUIRED!
  #endif
#else
  #define SIM_MODULE_TURBULENT_WIND_DECL(e) DAFX_EMPTY_STRUCT(SIM_MODULE_TURBULENT_WIND_EMPTY)
  #define SIM_MODULE_TURBULENT_WIND 0
#endif

#ifdef SIM_MODULE_RESOLVE_FORCES
  #define SIM_MODULE_RESOLVE_FORCES_DECL(e) e
#else
  #define SIM_MODULE_RESOLVE_FORCES_DECL(e) DAFX_EMPTY_STRUCT(SIM_MODULE_RESOLVE_FORCES_EMPTY)
  #define SIM_MODULE_RESOLVE_FORCES 0
#endif

#ifdef SIM_MODULE_GRAVITY_ZONE
  #define SIM_MODULE_GRAVITY_ZONE_DECL(e) e
  #if !SIM_MODULE_RESOLVE_FORCES
    #error SIM_MODULE_RESOLVE_FORCES REQUIRED!
  #endif
#else
  #define SIM_MODULE_GRAVITY_ZONE_DECL(e) DAFX_EMPTY_STRUCT(SIM_MODULE_GRAVITY_ZONE_EMPTY)
  #define SIM_MODULE_GRAVITY_ZONE 0
#endif

//--- Dafx Derived Module Data

#define SIM_MODULE_WIND (SIM_MODULE_DIRECTIONAL_WIND | SIM_MODULE_TURBULENT_WIND)
#if SIM_MODULE_WIND
  #define SIM_MODULE_WIND_DECL(e) e
#else
  #define SIM_MODULE_WIND_DECL(e) DAFX_EMPTY_STRUCT(SIM_MODULE_WIND_EMPTY)
#endif

#define PARTICLES_SIM_MODULE_PHASE (SIM_MODULE_DEPTH_COLLISION|SIM_MODULE_DIRECTIONAL_WIND|SIM_MODULE_TURBULENT_WIND|SIM_MODULE_COLOR)
#if PARTICLES_SIM_MODULE_PHASE
  #define PARTICLES_SIM_MODULE_PHASE_DECL(a) a
#else
  #define PARTICLES_SIM_MODULE_PHASE_DECL(a) DAFX_EMPTY_STRUCT(PARTICLES_SIM_MODULE_PHASE_EMPTY)
#endif

#define PARTICLES_REN_MODULE_POS 1
#if PARTICLES_REN_MODULE_POS
  #define PARTICLES_REN_MODULE_POS_DECL(e) e
#else
  #define PARTICLES_REN_MODULE_POS_DECL(e) DAFX_EMPTY_STRUCT(PARTICLES_REN_MODULE_POS_EMPTY)
  #define PARTICLES_REN_MODULE_POS 0
#endif

#define PARTICLES_REN_MODULE_VELOCITY 1
#if PARTICLES_REN_MODULE_VELOCITY
  #define PARTICLES_REN_MODULE_VELOCITY_DECL(e) e
#else
  #define PARTICLES_REN_MODULE_VELOCITY_DECL(e) DAFX_EMPTY_STRUCT(PARTICLES_REN_MODULE_VELOCITY_EMPTY)
  #define PARTICLES_REN_MODULE_VELOCITY 0
#endif

//--- Dafx Module Declarations

#define DAFX_PARENT_SIM_DATA \
  DAFX_MODULE_DATA_DECL(6 * SPAWN_MODULE_CUBE_POS, SPAWN_MODULE_CUBE_POS_DECL(CubeDistribution pos;), SPAWN_MODULE_CUBE_POS_DECL(o.pos = CubeDistribution_load(buf, ofs);), 0) \
  DAFX_MODULE_DATA_DECL(2 * SPAWN_MODULE_WIDTH, SPAWN_MODULE_WIDTH_DECL(LinearDistribution width;), SPAWN_MODULE_WIDTH_DECL(o.width = LinearDistribution_load(buf, ofs);), 0) \
  DAFX_MODULE_DATA_DECL(2 * SPAWN_MODULE_LINEAR_LIFE, SPAWN_MODULE_LINEAR_LIFE_DECL(LinearDistribution life;), SPAWN_MODULE_LINEAR_LIFE_DECL(o.life = LinearDistribution_load(buf, ofs);), 0) \
  DAFX_MODULE_DATA_DECL(1 * SPAWN_MODULE_SEED, SPAWN_MODULE_SEED_DECL(uint seed;), SPAWN_MODULE_SEED_DECL(o.seed = dafx_load_1ui(buf, ofs);), 0) \
  DAFX_MODULE_DATA_DECL(13 * SPAWN_MODULE_SPHERE_VELOCITY, SPAWN_MODULE_SPHERE_VELOCITY_DECL(SectorDistribution velocity;), SPAWN_MODULE_SPHERE_VELOCITY_DECL(o.velocity = SectorDistribution_load(buf, ofs);), 0) \
  DAFX_MODULE_DATA_DECL(2 * SIM_MODULE_DEPTH_COLLISION, SIM_MODULE_DEPTH_COLLISION_DECL(float restitution; float friction;), SIM_MODULE_DEPTH_COLLISION_DECL(o.restitution = dafx_load_1f(buf, ofs); o.friction = dafx_load_1f(buf, ofs);), 0) \
  DAFX_MODULE_DATA_DECL(5 * SIM_MODULE_COLOR, SIM_MODULE_COLOR_DECL(uint color0; uint color1; float color1Portion; float hdrBias; uint colorEnd;), SIM_MODULE_COLOR_DECL(o.color0 = dafx_load_1ui(buf, ofs); o.color1 = dafx_load_1ui(buf, ofs); o.color1Portion = dafx_load_1f(buf, ofs); o.hdrBias = dafx_load_1f(buf, ofs); o.colorEnd = dafx_load_1ui(buf, ofs);), 0) \
  DAFX_MODULE_DATA_DECL(1 * SPAWN_MODULE_VELOCITY_BIAS_POS, SPAWN_MODULE_VELOCITY_BIAS_POS_DECL(float velocityBias;), SPAWN_MODULE_VELOCITY_BIAS_POS_DECL(o.velocityBias = dafx_load_1f(buf, ofs);), 0) \
  DAFX_MODULE_DATA_DECL(1 * SIM_MODULE_WIND, SIM_MODULE_WIND_DECL(float dragCoefficient;), SIM_MODULE_WIND_DECL(o.dragCoefficient = dafx_load_1f(buf, ofs);), 0) \
  DAFX_MODULE_DATA_DECL(4 * SIM_MODULE_TURBULENT_WIND, SIM_MODULE_TURBULENT_WIND_DECL(LinearDistribution turbulenceForce; LinearDistribution turbulenceFreq;), SIM_MODULE_TURBULENT_WIND_DECL(o.turbulenceForce = LinearDistribution_load(buf, ofs); o.turbulenceFreq = LinearDistribution_load(buf, ofs);), 0) \
  DAFX_MODULE_DATA_DECL(4 * SIM_MODULE_LIFT, SIM_MODULE_LIFT_DECL(float3 liftForce; float liftTime;), SIM_MODULE_LIFT_DECL(o.liftForce = dafx_load_3f(buf, ofs); o.liftTime = dafx_load_1f(buf, ofs);), 0) \
  DAFX_MODULE_DATA_DECL(1 * SPAWN_MODULE_NOISE_VELOCITY, SPAWN_MODULE_NOISE_VELOCITY_DECL(float spawnNoise;), SPAWN_MODULE_NOISE_VELOCITY_DECL(o.spawnNoise = dafx_load_1f(buf, ofs);), 0) \
  DAFX_MODULE_DATA_DECL(6 * SPAWN_MODULE_CUBE_POS, SPAWN_MODULE_NOISE_POS_DECL(CubeDistribution spawnNoisePos;), SPAWN_MODULE_NOISE_POS_DECL(o.spawnNoisePos = CubeDistribution_load(buf, ofs);), 0) \
  DAFX_MODULE_DATA_DECL(1 * SIM_MODULE_COLOR, SIM_MODULE_COLOR_DECL2(float hdrScale1;), SIM_MODULE_COLOR_DECL2(o.hdrScale1 = dafx_load_1f(buf, ofs);), 0) \
  DAFX_MODULE_DATA_DECL(1 * SIM_MODULE_DIRECTIONAL_WIND, SIM_MODULE_DIRECTIONAL_WIND_DECL(float windForce;), SIM_MODULE_DIRECTIONAL_WIND_DECL(o.windForce = dafx_load_1f(buf, ofs);), 0) \
  DAFX_MODULE_DATA_DECL(1 * SIM_MODULE_GRAVITY_ZONE, SIM_MODULE_GRAVITY_ZONE_DECL(uint gravity_zone;), SIM_MODULE_GRAVITY_ZONE_DECL(o.gravity_zone = dafx_load_1ui(buf, ofs);), 0) \

#define DAFX_SIM_DATA \
  DAFX_MODULE_DATA_DECL(1 * SIM_MODULE_LIFE, SIM_MODULE_LIFE_DECL(float age;), SIM_MODULE_LIFE_DECL(p.age = dafx_load_1f(buf, ofs);), SIM_MODULE_LIFE_DECL(dafx_store_1f(p.age, buf, ofs);)) \
  DAFX_MODULE_DATA_DECL(1 * SPAWN_MODULE_LINEAR_LIFE, SPAWN_MODULE_LINEAR_LIFE_DECL(float lifeTime;), SPAWN_MODULE_LINEAR_LIFE_DECL(p.lifeTime = dafx_load_1f(buf, ofs);), SPAWN_MODULE_LINEAR_LIFE_DECL(dafx_store_1f(p.lifeTime, buf, ofs);)) \
  DAFX_MODULE_DATA_DECL(2 * PARTICLES_SIM_MODULE_PHASE, PARTICLES_SIM_MODULE_PHASE_DECL(float2 fPhase;), PARTICLES_SIM_MODULE_PHASE_DECL(p.fPhase = dafx_load_2f(buf, ofs);), PARTICLES_SIM_MODULE_PHASE_DECL(dafx_store_2f(p.fPhase, buf, ofs);))

#define DAFX_REN_DATA \
  DAFX_MODULE_DATA_DECL(3 * PARTICLES_REN_MODULE_POS, PARTICLES_REN_MODULE_POS_DECL(float3 pos;), PARTICLES_REN_MODULE_POS_DECL(p.pos = dafx_load_3f(buf, ofs);), PARTICLES_REN_MODULE_POS_DECL(dafx_store_3f(p.pos, buf, ofs);)) \
  DAFX_MODULE_DATA_DECL(3 * PARTICLES_REN_MODULE_VELOCITY, PARTICLES_REN_MODULE_VELOCITY_DECL(float3 velocity;), PARTICLES_REN_MODULE_VELOCITY_DECL(p.velocity = dafx_load_3f(buf, ofs);), PARTICLES_REN_MODULE_VELOCITY_DECL(dafx_store_3f(p.velocity, buf, ofs);)) \
  DAFX_MODULE_DATA_DECL(1 * SPAWN_MODULE_WIDTH, SPAWN_MODULE_WIDTH_DECL(float width;), SPAWN_MODULE_WIDTH_DECL(p.width = dafx_load_1f(buf, ofs);), SPAWN_MODULE_WIDTH_DECL(dafx_store_1f(p.width, buf, ofs);)) \
  DAFX_MODULE_DATA_DECL(1 * SIM_MODULE_WIDTH_MODIFIER, SIM_MODULE_WIDTH_MODIFIER_DECL(float widthModifier;), SIM_MODULE_WIDTH_MODIFIER_DECL(p.widthModifier = dafx_load_1f(buf, ofs);), SIM_MODULE_WIDTH_MODIFIER_DECL(dafx_store_1f(p.widthModifier, buf, ofs);)) \
  DAFX_MODULE_DATA_DECL(2 * SIM_MODULE_COLOR, SIM_MODULE_COLOR_DECL(uint color; float hdrScale;), SIM_MODULE_COLOR_DECL(p.color = dafx_load_1ui(buf, ofs); p.hdrScale = dafx_load_1f(buf, ofs);), SIM_MODULE_COLOR_DECL(dafx_store_1ui(p.color, buf, ofs); dafx_store_1f(p.hdrScale, buf, ofs);))


//--- Dafx Internal Declarations

#define REN_MODULE 1
#define REN_MODULE_DECL(e) e

#ifdef __cplusplus
#define DAFX_EMPTY_STRUCT(a)
#else
#define DAFX_EMPTY_STRUCT(a) struct {} a;
#endif

#undef DAFX_MODULE_DATA_DECL
#define DAFX_MODULE_DATA_DECL(sz, c, u, p) c

struct ParentSimData
{
  float4x4 emitterNtm;
  float3 fxVelocity;
  CurveData widthOverLifeCurve;
  float3x3 gravityTm;
  DAFX_PARENT_SIM_DATA
};

struct ParentRenData
{
  float4x4 tm;
  uint localSpaceFlag;
  DAFX_PARENT_REN_DATA
};

struct SimData
{
  DAFX_SIM_DATA
};

struct RenData
{
  DAFX_REN_DATA
};

#undef DAFX_MODULE_DATA_DECL
#define DAFX_MODULE_DATA_DECL(sz, c, u, p) u

DAFX_INLINE
ParentSimData unpack_parent_sim_data( BufferData_cref buf, uint ofs )
{
#ifdef __cplusplus
  return *(ParentSimData*)( buf + ofs );
#else
  ParentSimData o;
  o.emitterNtm = dafx_load_44mat(buf, ofs);
  o.fxVelocity = dafx_load_3f(buf, ofs);
  o.widthOverLifeCurve = CurveData_Load(buf, ofs);
  o.gravityTm = dafx_load_33mat(buf, ofs);
  DAFX_PARENT_SIM_DATA
  return o;
#endif
}

DAFX_INLINE
ParentRenData unpack_parent_ren_data( BufferData_cref buf, uint ofs )
{
#ifdef __cplusplus
  return *(ParentRenData*)( buf + ofs );
#else
  ParentRenData o;
  o.tm = dafx_load_44mat(buf, ofs);
  o.localSpaceFlag = dafx_load_1ui(buf, ofs);
  DAFX_PARENT_REN_DATA
  return o;
#endif
}

DAFX_INLINE
SimData unpack_sim_data(BufferData_ref buf, uint ofs)
{
#ifdef __cplusplus
  return *(SimData*)( buf + ofs );
#else
  SimData p;
  DAFX_SIM_DATA
  return p;
#endif
}

DAFX_INLINE
RenData unpack_ren_data(BufferData_ref buf, uint ofs)
{
#ifdef __cplusplus
  return *(RenData*)( buf + ofs );
#else
  RenData p;
  DAFX_REN_DATA;
  return p;
#endif
}

#undef DAFX_MODULE_DATA_DECL
#define DAFX_MODULE_DATA_DECL(sz, c, u, p) p

DAFX_INLINE
void pack_sim_data(SimData_ref p, BufferData_ref buf, uint ofs)
{
  DAFX_SIM_DATA
}

DAFX_INLINE
void pack_ren_data(RenData_ref p, BufferData_ref buf, uint ofs)
{
  DAFX_REN_DATA
}

#undef DAFX_MODULE_DATA_DECL
#define DAFX_MODULE_DATA_DECL(sz, c, u, p) + (sz)


#define DAFX_PARENT_SIM_DATA_EXTRA_OFFSET_EMITTER_NTM 0
#define DAFX_PARENT_SIM_DATA_EXTRA_OFFSET_FX_VELOCITY (DAFX_PARENT_SIM_DATA_EXTRA_OFFSET_EMITTER_NTM + 4*4)
#define DAFX_PARENT_SIM_DATA_EXTRA_OFFSET_WIDTH_OVER_LIFE_HEADER (DAFX_PARENT_SIM_DATA_EXTRA_OFFSET_FX_VELOCITY + 3)
#define DAFX_PARENT_SIM_DATA_EXTRA_OFFSET_WIDTH_OVER_LIFE_BODY (DAFX_PARENT_SIM_DATA_EXTRA_OFFSET_WIDTH_OVER_LIFE_HEADER + 1)
#define DAFX_PARENT_SIM_DATA_EXTRA_OFFSET_GRAVITY_TM (DAFX_PARENT_SIM_DATA_EXTRA_OFFSET_WIDTH_OVER_LIFE_BODY + 8)
#define DAFX_PARENT_SIM_DATA_EXTRA_END (DAFX_PARENT_SIM_DATA_EXTRA_OFFSET_GRAVITY_TM + 9)

#define DAFX_PARENT_SIM_DATA_SIZE (DAFX_PARENT_SIM_DATA_EXTRA_END + DAFX_PARENT_SIM_DATA) * 4
#if DAFX_PARENT_SIM_DATA_SIZE
  G_STATIC_ASSERT(sizeof(ParentSimData) == DAFX_PARENT_SIM_DATA_SIZE);
#endif

#define DAFX_PARENT_REN_DATA_SIZE (16 + 1 + DAFX_PARENT_REN_DATA) * 4
#if DAFX_PARENT_REN_DATA_SIZE
  G_STATIC_ASSERT(sizeof(ParentRenData) == DAFX_PARENT_REN_DATA_SIZE);
#endif

#define DAFX_SIM_DATA_SIZE (DAFX_SIM_DATA) * 4
#if DAFX_SIM_DATA_SIZE
  G_STATIC_ASSERT(sizeof(SimData) == DAFX_SIM_DATA_SIZE);
#endif

#define DAFX_REN_DATA_SIZE (DAFX_REN_DATA) * 4
#if DAFX_REN_DATA_SIZE
  G_STATIC_ASSERT(sizeof(RenData) == DAFX_REN_DATA_SIZE);
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif
