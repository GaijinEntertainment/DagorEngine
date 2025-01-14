#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4100)
#pragma warning(disable:4189)
#endif

#include <dafx_gravity_zone_funcs.hlsli>

#define SIM_MODULE_DATA_AGE_NORM (SIM_MODULE_LIFE)
#define SIM_MODULE_DATA_COLLISION (SIM_MODULE_DEPTH_COLLISION)
#define SIM_MODULE_DATA_FORCE (SIM_MODULE_RESOLVE_FORCES)
#define SIM_MODULE_DATA_WIND (SIM_MODULE_DIRECTIONAL_WIND|SIM_MODULE_TURBULENT_WIND)

#define RESTITUTION_NOISE 0.1f
#define FRICTION_NOISE 1.0f
#define DRAG_NOISE 0.1f
#define DRAG_LIMIT_MUL 0.5f
#define COLOR_END_DURATION 0.2f
#define TURBULENCE_COLLISION_DIST 1.0f
#define TURBULENCE_OFFSET 100.0f
#define TURBULENCE_SCALE 10.0f
#define LIFT_FADE_TIME 1.0f
#define PARTICLE_MASS 1.0f

struct SimModuleData
{
#if SIM_MODULE_DATA_AGE_NORM
  float ageNorm;
#endif
#if SIM_MODULE_DATA_COLLISION
  float minCollisionDist;
  bool isCollided;
#endif
#if SIM_MODULE_DATA_FORCE
  float massInv;
  float3 force;
#endif
#if SIM_MODULE_DATA_WIND
  float flowDrag;
  float3 windVelocity;
#endif
};

#ifdef __cplusplus
  #define SimModuleData_ref SimModuleData&
#else
  #define SimModuleData_ref in out SimModuleData

  #if in_editor_assume==no
    #define USE_DEPTH_COLLISION 1
  #endif
#endif

#define INPUT_SIM_CONTEXT RenData_ref rp, SimData_ref sp, ParentRenData_cref rparams, ParentSimData_cref sparams, GlobalData_cref gparams
#define SIM_MODULE_CONTEXT SimModuleData_ref md, INPUT_SIM_CONTEXT
#define SIM_MODULE_ARGS md, rp, sp, rparams, sparams, gparams

#define UNREF_INPUT_SIM_CONTEXT \
  G_UNREFERENCED( md ); \
  G_UNREFERENCED( rp ); \
  G_UNREFERENCED( sp ); \
  G_UNREFERENCED( rparams ); \
  G_UNREFERENCED( sparams); \
  G_UNREFERENCED( gparams )

#if DAFX_USE_GRAVITY_ZONE && SIM_MODULE_GRAVITY_ZONE
DAFX_INLINE
float3x3 sparkfx_gravity_zone_tm(SIM_MODULE_CONTEXT)
{
  UNREF_INPUT_SIM_CONTEXT;
  float3x3 gravityTm = m33_ident();
  BRANCH
  if (sparams.gravity_zone == GRAVITY_ZONE_DEFAULT || sparams.gravity_zone == GRAVITY_ZONE_PER_EMITTER)
    gravityTm = sparams.gravityTm;
  else BRANCH if (sparams.gravity_zone == GRAVITY_ZONE_PER_PARTICLE) {
    float3 wpos = rparams.localSpaceFlag ? mul_get_f3(float_xyz1(rp.pos), rparams.tm) : rp.pos;
    gravityTm = dafx_gravity_zone_tm(wpos, gparams.gravity_zone_count);
  }

  return gravityTm;
}
#endif

DAFX_INLINE
void init_sim_module(SIM_MODULE_CONTEXT)
{
#if SIM_MODULE_DATA_AGE_NORM
  md.ageNorm = 0;
#endif
#if SIM_MODULE_DATA_COLLISION
  md.minCollisionDist = 1000000.0;
  md.isCollided = false;
#endif
#if SIM_MODULE_DATA_FORCE
  md.massInv = 1.0 / PARTICLE_MASS;
  md.force = float3(0, 0, 0);
#endif
#if SIM_MODULE_DATA_WIND
  md.windVelocity = float3(0, 0, 0);
  // Add a minimum threshold to stabilize the air simulation
  md.flowDrag = max(sparams.dragCoefficient * (1 + DRAG_NOISE * sp.fPhase.x), 0.001f);
#endif
  UNREF_INPUT_SIM_CONTEXT;
}

DAFX_INLINE
float4 finish_sim_module(SIM_MODULE_CONTEXT, float dt)
{
  rp.pos += rp.velocity * dt;

  float3 wpos = rp.pos;
  if (rparams.localSpaceFlag)
  {
    float4 p4 = float4(rp.pos, 1);
    wpos = float3(
      dot(p4, rparams.tm[0]),
      dot(p4, rparams.tm[1]),
      dot(p4, rparams.tm[2]));
  }
  return float4(
    wpos.x,
    wpos.y,
    wpos.z,
    rp.width
  );
  UNREF_INPUT_SIM_CONTEXT;
}

#if SIM_MODULE_LIFE
DAFX_INLINE
bool sim_module_life(SIM_MODULE_CONTEXT, float dt)
{
  sp.age = sp.age + dt;
  md.ageNorm = saturate(sp.age / max(sp.lifeTime, 0.0001f));
  return sp.age <= sp.lifeTime;
  UNREF_INPUT_SIM_CONTEXT;
}
#endif

#if SIM_MODULE_GRAVITY
DAFX_INLINE
void sim_module_gravity(SIM_MODULE_CONTEXT)
{
  float3 gravityVec = rparams.localSpaceFlag ? float_xyz(mul(float_xyz0(gdata_gravity), rparams.tm)) : gdata_gravity;
  md.force += gravityVec * PARTICLE_MASS;
  UNREF_INPUT_SIM_CONTEXT;
}
#endif

#if SIM_MODULE_LIFT
DAFX_INLINE
void sim_module_lift(SIM_MODULE_CONTEXT)
{
  float3 forceVec = rparams.localSpaceFlag ? float_xyz(mul(float_xyz0(sparams.liftForce), rparams.tm)) : sparams.liftForce;
  md.force += forceVec * (1.0 - saturate((sp.age - sparams.liftTime + LIFT_FADE_TIME) / LIFT_FADE_TIME));
  UNREF_INPUT_SIM_CONTEXT;
}
#endif

#if SIM_MODULE_DEPTH_COLLISION
DAFX_INLINE
void sim_module_depth_collision(SIM_MODULE_CONTEXT, float dt)
{
#ifndef __cplusplus
  float restitution = saturate(sparams.restitution + RESTITUTION_NOISE * sp.fPhase.x);
  float friction = saturate(sparams.friction + FRICTION_NOISE * sp.fPhase.y);
  #if USE_DEPTH_COLLISION
  md.isCollided = apply_depth_collision(gparams, dt, rp.pos, rp.velocity, restitution, friction, rp.pos, rp.velocity, md.minCollisionDist);
  #else
  md.isCollided = false;
  #endif
#if SIM_MODULE_DATA_FORCE
  if (md.isCollided)
    md.force = 0;
#endif
#endif
  UNREF_INPUT_SIM_CONTEXT;
  G_UNREFERENCED(dt);
}
#endif

#if SIM_MODULE_DIRECTIONAL_WIND
DAFX_INLINE
void sim_module_directional_wind(SIM_MODULE_CONTEXT)
{
  float3 windDir = float3(gparams.wind_dir.x, 0.f, gparams.wind_dir.y);
  float3 windVec = rparams.localSpaceFlag ? float_xyz(mul(float_xyz0(windDir), rparams.tm)) : windDir;
  md.windVelocity += windVec * sparams.windForce * gparams.wind_power;
  UNREF_INPUT_SIM_CONTEXT;
}
#endif

#if SIM_MODULE_TURBULENT_WIND
DAFX_INLINE
void sim_module_turbulent_wind(SIM_MODULE_CONTEXT)
{
  // Apply turbulence after collisions to have an actual info
#ifndef __cplusplus // TODO: make simpler noise func for cpu sim
  if (!md.isCollided)
    md.windVelocity += noise_SimplexPerlin3D_Deriv(rp.pos * LinearDistribution_lerp(sparams.turbulenceFreq, sp.fPhase.x) + TURBULENCE_OFFSET).xyz *
      LinearDistribution_lerp(sparams.turbulenceForce, sp.fPhase.y) * saturate(md.minCollisionDist / TURBULENCE_COLLISION_DIST) * TURBULENCE_SCALE / sqrt(md.flowDrag);
#endif
  UNREF_INPUT_SIM_CONTEXT;
}
#endif

#if SIM_MODULE_RESOLVE_FORCES
DAFX_INLINE
void sim_module_resolve_forces(SIM_MODULE_CONTEXT, float dt)
{
#if DAFX_USE_GRAVITY_ZONE && SIM_MODULE_GRAVITY_ZONE
  md.force = mul(md.force, sparkfx_gravity_zone_tm(SIM_MODULE_ARGS));
#endif

  // resolve velocity before drag
  rp.velocity += md.force * dt * md.massInv;
  md.force = float3(0, 0, 0);

#if SIM_MODULE_DIRECTIONAL_WIND | SIM_MODULE_TURBULENT_WIND
  float3 vel = rp.velocity;
  float velLen = length(vel);
  float3 velDir = vel / max(velLen, 0.001f);

  float windVel = length(md.windVelocity);
  float3 windDir = md.windVelocity / max(windVel, 0.001f);
  float3 flowForward = windVel > 0.001 ? windDir : float3(0, 1, 0);
  float3 flowBinormal = abs(dot(flowForward, velDir)) < 0.999 ? cross(flowForward, velDir) : (abs(flowForward.x) < 0.999 ? float3(1, 0, 0) : float3(0, 0, 1));
  float3 flowTangent = cross(flowBinormal, flowForward);
  float flowTangentLen = length(flowTangent);
  flowTangent = flowTangentLen > 0.001 ? flowTangent / flowTangentLen : float3(0, 0, 0);
  float flowVelForward = dot(vel, flowForward);
  float flowVelTang = dot(vel, flowTangent);
  md.force +=
    flowForward * sign(windVel - flowVelForward) * min(pow2(windVel - flowVelForward) * rp.width * md.flowDrag, abs(flowVelForward) / max(dt, 0.001f) * PARTICLE_MASS * DRAG_LIMIT_MUL) +
    flowTangent * sign(-flowVelTang) * min(pow2(flowVelTang) * rp.width * md.flowDrag, abs(flowVelTang) / max(dt, 0.001f) * PARTICLE_MASS * DRAG_LIMIT_MUL);

  rp.velocity += md.force * dt * md.massInv;
#endif

  UNREF_INPUT_SIM_CONTEXT;
}
#endif

#if SIM_MODULE_COLOR
DAFX_INLINE
float make_impulse_curve(float value, float freq, float phase, float pause)
{
  float t = abs(frac((value + phase) * freq) - 0.5) * 2;
  t = saturate((t - pause) / (1 - pause));
  return 1 - t;
}

void sim_module_color(SIM_MODULE_CONTEXT)
{
  float color1Bias = saturate(1.0 - sparams.color1Portion);
  float4 colorStart0 = float_zyxw(unpack_uint_to_n4f(sparams.color0));
  float4 colorStart1 = float_zyxw(unpack_uint_to_n4f(sparams.color1));
  float4 colorStart = lerp(colorStart1, colorStart0, calc_biased_random(sp.fPhase.x, color1Bias));
  float4 colorEnd = float_zyxw(unpack_uint_to_n4f(sparams.colorEnd));
  float ageEnd = saturate((md.ageNorm - (1 - COLOR_END_DURATION)) / COLOR_END_DURATION);
  float impulseEnd = make_impulse_curve(sp.age, lerp(0.25, 1.0, ageEnd), sp.fPhase.x, 0.7 * ageEnd);
  float4 finalColor = lerp(colorStart, colorEnd, ageEnd * impulseEnd);
  rp.color = pack_n4f_to_uint(finalColor);

  float hdrBias = saturate(1.0 - sparams.hdrBias);
  rp.hdrScale = lerp(sparams.hdrScale1, rparams.hdrScale, calc_biased_random(sp.fPhase.y, hdrBias));

  UNREF_INPUT_SIM_CONTEXT;
}
#endif

#if SIM_MODULE_WIDTH_MODIFIER
DAFX_INLINE
float calc_curve_weight( uint steps, float life_k, uint_ref k0, uint_ref k1 )
{
  uint s = steps - 1;
  float k = s * life_k;

  k0 = (uint)k;
  k1 = min( k0 + 1, s );
  return k - (float)k0;
}

DAFX_INLINE
float sparkfx_get_1f_curve( CurveData curve_data, float life_k )
{
  life_k = saturate(life_k);

  uint k0, k1;
  float w = calc_curve_weight( 32, life_k, k0, k1 );

  // 1byte components
  uint w0 = curve_data.packedCurveData[k0 / 4];
  uint w1 = curve_data.packedCurveData[k1 / 4];

  uint4 b0 = unpack_4b( w0 );
  uint4 b1 = unpack_4b( w1 );

  float v0 = unpack_byte_to_n1f( b0[k0 % 4] );
  float v1 = unpack_byte_to_n1f( b1[k1 % 4] );

  return lerp( v0, v1, w );
}

DAFX_INLINE
void sim_module_width_modifier(SIM_MODULE_CONTEXT)
{
  rp.widthModifier = 1;

  BRANCH
  if (!sparams.widthOverLifeCurve.enabled)
    return;

  rp.widthModifier = sparkfx_get_1f_curve(sparams.widthOverLifeCurve, md.ageNorm);
  UNREF_INPUT_SIM_CONTEXT;
}
#endif

DAFX_INLINE
float4 dafx_simulation_shader_cb( ComputeCallDesc_cref cdesc, BufferData_ref ldata, GlobalData_cref gparams )
{
  ParentRenData rparams = unpack_parent_ren_data( ldata, cdesc.parentRenOffset );
  ParentSimData sparams = unpack_parent_sim_data( ldata, cdesc.parentSimOffset );

  float dt = cdesc.lod * gparams.dt;

  RenData rp = unpack_ren_data( ldata, cdesc.dataRenOffsetCurrent );
  SimData sp = unpack_sim_data( ldata, cdesc.dataSimOffsetCurrent );

  SimModuleData md;
  init_sim_module(SIM_MODULE_ARGS);

  bool is_alive = sim_module_life(SIM_MODULE_ARGS, dt);

  if ( !is_alive )
  {
    rp.width = 0;
    pack_ren_data( rp, ldata, cdesc.dataRenOffsetCurrent );
    pack_sim_data( sp, ldata, cdesc.dataSimOffsetCurrent );
    return float4( 0, 0, 0, 0 );
  }

#if SIM_MODULE_WIDTH_MODIFIER
  sim_module_width_modifier(SIM_MODULE_ARGS);
#endif

#if SIM_MODULE_GRAVITY
  sim_module_gravity(SIM_MODULE_ARGS);
#endif

#if SIM_MODULE_LIFT
  sim_module_lift(SIM_MODULE_ARGS);
#endif

#if SIM_MODULE_DEPTH_COLLISION
  sim_module_depth_collision(SIM_MODULE_ARGS, dt);
#endif

#if SIM_MODULE_DIRECTIONAL_WIND
  sim_module_directional_wind(SIM_MODULE_ARGS);
#endif

#if SIM_MODULE_TURBULENT_WIND
  sim_module_turbulent_wind(SIM_MODULE_ARGS);
#endif

#if SIM_MODULE_RESOLVE_FORCES
  sim_module_resolve_forces(SIM_MODULE_ARGS, dt);
#endif

#if SIM_MODULE_COLOR
  sim_module_color(SIM_MODULE_ARGS);
#endif

  float4 cull = finish_sim_module(SIM_MODULE_ARGS, dt);
  pack_ren_data( rp, ldata, cdesc.dataRenOffsetCurrent );
  pack_sim_data( sp, ldata, cdesc.dataSimOffsetCurrent );
  return cull;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
