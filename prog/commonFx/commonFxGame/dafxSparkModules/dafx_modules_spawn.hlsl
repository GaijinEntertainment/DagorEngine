#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4100)
#pragma warning(disable:4189)
#endif

#define SPAWN_MODULE_DATA_DISPATCH_SEED (SPAWN_MODULE_NOISE_VELOCITY|SPAWN_MODULE_NOISE_POS)

struct SpawnModuleData
{
#if SPAWN_MODULE_DATA_DISPATCH_SEED
  uint dispatchSeed;
#endif
};

#ifdef __cplusplus
  #define SpawnModuleData_ref SpawnModuleData&
#else
  #define SpawnModuleData_ref in out SpawnModuleData
#endif

#define INPUT_SPAWN_CONTEXT uint gid, RenData_ref rp, SimData_ref sp, ParentRenData_cref rparams, ParentSimData_cref sparams, GlobalData_cref gparams
#define SPAWN_MODULE_CONTEXT SpawnModuleData_ref md, rnd_seed_ref rnd_seed, INPUT_SPAWN_CONTEXT
#define SPAWN_MODULE_ARGS md, rnd_seed, gid, rp, sp, rparams, sparams, gparams

#define UNREF_SPAWN_MODULE_CONTEXT \
  G_UNREFERENCED( md ); \
  G_UNREFERENCED( rnd_seed ); \
  G_UNREFERENCED( rp ); \
  G_UNREFERENCED( sp ); \
  G_UNREFERENCED( rparams ); \
  G_UNREFERENCED( sparams); \
  G_UNREFERENCED( gparams ); \
  G_UNREFERENCED( gid )

DAFX_INLINE
void init_spawn_module(SPAWN_MODULE_CONTEXT)
{
#if SPAWN_MODULE_DATA_DISPATCH_SEED
  md.dispatchSeed = gid;
#endif
#if SIM_MODULE_LIFE
  sp.age = 0;
#endif
#if PARTICLES_SIM_MODULE_PHASE
  sp.fPhase = float2(dafx_frnd(rnd_seed), dafx_frnd(rnd_seed));
#endif
  UNREF_SPAWN_MODULE_CONTEXT;
}

DAFX_INLINE
float4 finish_spawn_module(SPAWN_MODULE_CONTEXT)
{
  rp.velocity = float_xyz(mul(float4(rp.velocity, 1), sparams.emitterNtm));
  rp.velocity += rparams.fxVelocity;
  rp.pos = float_xyz(mul(float4(rp.pos, 1), sparams.emitterWtm));

  float4 p4 = float4(rp.pos, 1);
  float3 wpos = float3(
    dot(p4, rparams.fxTm[0]),
    dot(p4, rparams.fxTm[1]),
    dot(p4, rparams.fxTm[2]));

  return float4(
    wpos.x,
    wpos.y,
    wpos.z,
#if PARTICLES_REND_MODULE_WIDTH
    rp.width
#else
    0.5
#endif
  );
  UNREF_SPAWN_MODULE_CONTEXT;
}

#if SPAWN_MODULE_LINEAR_LIFE
DAFX_INLINE
void spawn_module_linear_life(SPAWN_MODULE_CONTEXT)
{
  sp.lifeTime = LinearDistribution_gen(sparams.life, rnd_seed);
  UNREF_SPAWN_MODULE_CONTEXT;
}
#endif

#if SPAWN_MODULE_SPHERE_VELOCITY
DAFX_INLINE
void spawn_module_sphere_velocity(SPAWN_MODULE_CONTEXT)
{
  rp.velocity += SectorDistribution_gen(sparams.velocity, rnd_seed);
  UNREF_SPAWN_MODULE_CONTEXT;
}
#endif

#if SPAWN_MODULE_NOISE_VELOCITY
DAFX_INLINE
void spawn_module_noise_velocity(SPAWN_MODULE_CONTEXT)
{
  float3 noisedUp = gen_spherical_dir(0, 1, 0, sparams.spawnNoise, md.dispatchSeed);
  float3 noisedForward = normalize(cross(noisedUp, abs(noisedUp.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0)));
  float3 noisedRight = cross(noisedUp, noisedForward);
  rp.velocity = rp.velocity.x * noisedForward + rp.velocity.y * noisedUp + rp.velocity.z * noisedRight;
  UNREF_SPAWN_MODULE_CONTEXT;
}
#endif

#if SPAWN_MODULE_NOISE_POS
DAFX_INLINE
void spawn_module_noise_pos(SPAWN_MODULE_CONTEXT)
{
  rp.pos += CubeDistribution_gen(sparams.spawnNoisePos, md.dispatchSeed);
  UNREF_SPAWN_MODULE_CONTEXT;
}
#endif

#if SPAWN_MODULE_WIDTH
DAFX_INLINE
void spawn_module_width(SPAWN_MODULE_CONTEXT)
{
  rp.width += LinearDistribution_gen(sparams.width, rnd_seed);
  UNREF_SPAWN_MODULE_CONTEXT;
}
#endif

#if SPAWN_MODULE_CUBE_POS
DAFX_INLINE
void spawn_module_cube_pos(SPAWN_MODULE_CONTEXT)
{
  rp.pos += CubeDistribution_gen(sparams.pos, rnd_seed);
  UNREF_SPAWN_MODULE_CONTEXT;
}
#endif

#if SPAWN_MODULE_VELOCITY_BIAS_POS
DAFX_INLINE
void spawn_module_velocity_bias_pos(SPAWN_MODULE_CONTEXT)
{
  rp.pos += SafeNormalize(rp.velocity) * sparams.velocityBias * dafx_frnd(rnd_seed);
  UNREF_SPAWN_MODULE_CONTEXT;
}
#endif

DAFX_INLINE
float4 dafx_emission_shader_cb( ComputeCallDesc_cref cdesc, BufferData_ref ldata, GlobalData_cref gparams )
{
  ParentRenData rparams = unpack_parent_ren_data( ldata, cdesc.parentRenOffset );
  ParentSimData sparams = unpack_parent_sim_data( ldata, cdesc.parentSimOffset );

  #ifdef __cplusplus
    RenData rp;
    SimData sp;
    memset( &rp, 0, sizeof( RenData ) );
    memset( &sp, 0, sizeof( SimData ) );
  #else
    RenData rp = (RenData)0;
    SimData sp = (SimData)0;
  #endif

  rnd_seed_t rnd_seed = dafx_calc_instance_rnd_seed( cdesc.gid, cdesc.rndSeed );
  uint gid = dafx_calc_instance_rnd_seed( cdesc.rndSeed, cdesc.rndSeed );

  SpawnModuleData md;
  init_spawn_module(SPAWN_MODULE_ARGS);

#if SPAWN_MODULE_LINEAR_LIFE
  spawn_module_linear_life(SPAWN_MODULE_ARGS);
#endif

#if SPAWN_MODULE_SPHERE_VELOCITY
  spawn_module_sphere_velocity(SPAWN_MODULE_ARGS);
#endif

#if SPAWN_MODULE_NOISE_VELOCITY
  spawn_module_noise_velocity(SPAWN_MODULE_ARGS);
#endif

#if SPAWN_MODULE_NOISE_POS
  spawn_module_noise_pos(SPAWN_MODULE_ARGS);
#endif

#if SPAWN_MODULE_WIDTH
  spawn_module_width(SPAWN_MODULE_ARGS);
#endif

#if SPAWN_MODULE_CUBE_POS
  spawn_module_cube_pos(SPAWN_MODULE_ARGS);
#endif

#if SPAWN_MODULE_VELOCITY_BIAS_POS
  spawn_module_velocity_bias_pos(SPAWN_MODULE_ARGS);
#endif

  float4 cull = finish_spawn_module(SPAWN_MODULE_ARGS);
  pack_ren_data( rp, ldata, cdesc.dataRenOffsetCurrent );
  pack_sim_data( sp, ldata, cdesc.dataSimOffsetCurrent );
  return cull;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
