#ifndef DAFX_MODFX_VELOCITY_HLSL
#define DAFX_MODFX_VELOCITY_HLSL

#include "simplex_perlin_3D_deriv.hlsli"

#if MODFX_GPU_FEATURES_ENABLED

ModfxDeclCollision ModfxDeclCollision_load( BufferData_cref buf, uint ofs )
{
  ModfxDeclCollision pp;
  pp.radius_k = dafx_load_1f( buf, ofs );
  pp.reflect_power = dafx_load_1f( buf, ofs );
  pp.reflect_energy = dafx_load_1f( buf, ofs );
  pp.emitter_deadzone = dafx_load_1f( buf, ofs );
  return pp;
}

bool modfx_scene_collision_sample( float3 wpos, GlobalData_cref gdata,
  out float2 o_stc, out uint2 o_tci, out float o_proj_depth, out float o_scene_depth )
{
  float4 spos = mul( float4( wpos, 1 ), gdata.globtm );
  spos.xyz /= spos.w;

  o_tci = 0;
  o_proj_depth = 0;
  o_scene_depth = 0;

  o_stc = float2( spos.xy * float2( 0.5, -0.5 ) + float2( 0.5, 0.5 ) );
  if ( o_stc.x < 0 || o_stc.y < 0 || o_stc.x >= 1.f || o_stc.y >= 1 )
    return false;

  o_tci = o_stc.xy * gdata.depth_size.xy;

  o_proj_depth = dafx_linearize_z( spos.z, gdata );
  o_scene_depth = dafx_sample_linear_depth( o_tci, gdata );

  return true;
}

float modfx_scene_collision_sim( BufferData_ref buf, uint ofs, float4x4_cref etm, bool etm_enabled,
  float radius, float3_ref o_pos, float3_ref o_vel, float dt, GlobalData_cref gdata )
{
  ModfxDeclCollision par = ModfxDeclCollision_load( buf, ofs );

  float3 emitter_pos = etm_enabled ? float3( etm[3][0], etm[3][1], etm[3][2] ) : float3( 0, 0, 0 );
  float3 emitter_vec = emitter_pos - o_pos;
  if ( dot( emitter_vec, emitter_vec ) <  par.emitter_deadzone )
    return 1;

  float vel_len2 = dot( o_vel, o_vel );
  float vel_len =  sqrt( vel_len2 );
  float3 vel_norm = vel_len > 0 ? o_vel / vel_len : 0;

  const float cr = radius * par.radius_k;

  // trace closest depth intersection or overshoot
  float2 stc;
  uint2 tci;
  float proj_depth;
  float scene_depth;
  if ( !modfx_scene_collision_sample( o_pos, gdata, stc, tci, proj_depth, scene_depth ) )
    return 1;

  float depth_diff = scene_depth - proj_depth;
  bool invert_normal = depth_diff < 0;

  float3 trace = abs( depth_diff ) > cr ? o_vel * dt : vel_norm * abs( depth_diff ); // guessing closest point

  float2 stc_test;
  uint2 tci_test;
  float proj_depth_test;
  float scene_depth_test;
  if ( !modfx_scene_collision_sample( o_pos + trace, gdata, stc_test, tci_test, proj_depth_test, scene_depth_test ) )
    return 1;
  float depth_diff_test = scene_depth_test - proj_depth_test;

  bool is_overshoot = depth_diff > 0 && depth_diff_test < 0 && depth_diff < vel_len * cr;
  if ( is_overshoot ) // initial collision is priority
  {
    depth_diff = 0;
  }
  else if ( abs( depth_diff_test ) < abs( depth_diff ) ) // closest
  {
    stc = stc_test;
    tci = tci_test;
    depth_diff = depth_diff_test;
    scene_depth = scene_depth_test;
  }

  if ( abs( depth_diff ) > cr )
    return 1;

  uint2 tci_n0 = uint2( min(tci.x + 1, gdata.depth_size.x), tci.y );
  uint2 tci_n1 = uint2( tci.x, min(tci.y + 1, gdata.depth_size.y) );

  float3 w0 = gdata.world_view_pos + lerp_view_vec( stc ) * scene_depth;
  float3 w1 = gdata.world_view_pos + lerp_view_vec( stc + float2( gdata.depth_size_rcp.x, 0 ) ) * dafx_sample_linear_depth( tci_n0, gdata );
  float3 w2 = gdata.world_view_pos + lerp_view_vec( stc + float2( 0, gdata.depth_size_rcp.y ) ) * dafx_sample_linear_depth( tci_n1, gdata );

  float3 scene_normal = normalize( cross( w1 - w0, w2 - w0 ) );
  if ( invert_normal ) // behind the wall
    scene_normal = -scene_normal;

  float ndotv = dot( scene_normal, vel_norm );
  if ( ndotv > 0 )
    return 0;

  float3 refl = reflect( vel_norm, scene_normal );
  float3 tang = cross( normalize( cross( scene_normal, vel_norm ) ), scene_normal );

  o_vel = normalize( lerp( tang, refl, par.reflect_power ) );
  o_vel *= vel_len * par.reflect_energy;

  return 0.f;
}
#endif

DAFX_INLINE
ModfxDeclDragInit ModfxDeclDragInit_load( BufferData_cref buf, uint ofs )
{
#ifdef __cplusplus
  return *(ModfxDeclDragInit*)( buf + ofs );
#else
  ModfxDeclDragInit pp;
  pp.drag_coeff = dafx_load_1f( buf, ofs );
  pp.drag_to_rad_k = dafx_load_1f( buf, ofs );
  return pp;
#endif
}

DAFX_INLINE
void modfx_velocity_drag_init( BufferData_cref buf, uint ofs, float radius, float_ref o_drag )
{
  ModfxDeclDragInit pp = ModfxDeclDragInit_load( buf, ofs );

  float r = lerp( 1.f, radius, pp.drag_to_rad_k );
  float c_a = dafx_pi * ( r * r ); // sphere projection
  o_drag = c_a * pp.drag_coeff;
}

DAFX_INLINE
ModfxDeclVelocity ModfxDeclVelocity_load( BufferData_cref buf, uint ofs )
{
#ifdef __cplusplus
  return *(ModfxDeclVelocity*)(buf + ofs);
#else
  ModfxDeclVelocity pp;
  pp.vel_min = dafx_load_1f( buf, ofs );
  pp.vel_max = dafx_load_1f( buf, ofs );
  pp.vec_rnd = dafx_load_1f( buf, ofs );
  return pp;
#endif
}

DAFX_INLINE
void modfx_velocity_init(
  ModfxParentSimData_cref parent_sdata, BufferData_cref buf, uint ofs,
  rnd_seed_ref rnd_seed, float4x4_cref etm, bool etm_enabled, float3_cref pos, float3_cref pos_v, float3_ref o_velocity )
{
  ModfxDeclVelocity pp = ModfxDeclVelocity_load( buf, ofs );

  float len = lerp( pp.vel_min, pp.vel_max, dafx_frnd( rnd_seed ) );

  if ( parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_INIT_POINT] )
  {
    float3 offset = dafx_get_3f( buf, parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_INIT_POINT] );
    if ( etm_enabled )
      offset = mul_get_f3( float_xyz1( offset ), etm );

    o_velocity = normalize( pos - offset );
  }
  else if ( parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_INIT_VEC] )
  {
    float3 vec = dafx_get_3f( buf, parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_INIT_VEC] );
    o_velocity = vec;
  }
  else if ( MOD_ENABLED( parent_sdata.mods, MODFX_SMOD_VELOCITY_INIT_START_SHAPE ) )
  {
    o_velocity = pos_v;
  }

  o_velocity = normalize( lerp( o_velocity, dafx_srnd_vec3( rnd_seed ), pp.vec_rnd ) ) * len;
}

DAFX_INLINE
ModfxDeclVelocityCone ModfxDeclVelocityCone_load( BufferData_cref buf, uint ofs )
{
#ifdef __cplusplus
  return *(ModfxDeclVelocityCone*)( buf + ofs );
#else
  ModfxDeclVelocityCone pp;
  pp.yaxis = dafx_load_3f( buf, ofs );
  pp.origin = dafx_load_3f( buf, ofs );
  pp.border_cos = dafx_load_1f( buf, ofs );
  pp.center_power = dafx_load_1f( buf, ofs );
  pp.border_power = dafx_load_1f( buf, ofs );
  return pp;
#endif
}

DAFX_INLINE
void modfx_velocity_add(
  ModfxParentSimData_cref parent_sdata, BufferData_cref buf, uint ofs,
  rnd_seed_ref rnd_seed, float4x4_cref etm, bool etm_enabled, float3_cref pos, float3_ref o_velocity )
{
  ModfxDeclVelocity pp = ModfxDeclVelocity_load( buf, ofs );

  float3 res;
  if ( !FLAG_ENABLED( parent_sdata.flags, MODFX_SFLAG_VELOCITY_APPLY_EMITTER_TRANSFORM_TO_ADD ) )
    etm_enabled = false;

  float len = lerp( pp.vel_min, pp.vel_max, dafx_frnd( rnd_seed ) );
  if ( parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_ADD_POINT] )
  {
    float3 offset = dafx_get_3f( buf, parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_ADD_POINT] );
    if ( etm_enabled )
      offset = mul_get_f3( float_xyz1( offset ), etm );

    res = normalize( pos - offset );
  }
  else if ( parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_ADD_VEC] )
  {
    res = dafx_get_3f( buf, parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_ADD_VEC] );
    if ( etm_enabled )
      res = mul_get_f3( float_xyz0( res ), etm );
  }
  else if ( parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_ADD_CONE] )
  {
    ofs = parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_ADD_CONE];
    ModfxDeclVelocityCone pp = ModfxDeclVelocityCone_load( buf, ofs );

    float3 yaxis = pp.yaxis;
    float3 origin = pp.origin;
    if ( etm_enabled )
    {
      yaxis = mul_get_f3( float_xyz0( yaxis ), etm );
      origin = mul_get_f3( float_xyz1( origin ), etm );
    }

    float3 cur_vec = normalize( pos - origin );
    float cur_cos = dot( cur_vec, yaxis );
    if ( cur_cos < pp.border_cos )
      len = 0;

    float a = smoothstep( pp.border_cos, 1.f, cur_cos );
    len *= lerp( pp.border_power, pp.center_power, a );

    res = cur_vec;
  }

  o_velocity = normalize( lerp( res, dafx_srnd_vec3( rnd_seed ), pp.vec_rnd ) ) * len;
}

DAFX_INLINE
ModfxDeclVelocityForceFieldVortex ModfxDeclVelocityForceFieldVortex_load( BufferData_cref buf, uint ofs )
{
#ifdef __cplusplus
  return *(ModfxDeclVelocityForceFieldVortex*)( buf + ofs );
#else
  ModfxDeclVelocityForceFieldVortex pp;
  pp.axis_direction = dafx_load_3f(buf, ofs);
  pp.direction_rnd = dafx_load_1f(buf, ofs);
  pp.axis_position = dafx_load_3f(buf, ofs);
  pp.position_rnd = dafx_load_3f(buf, ofs);
  pp.rotation_speed_min = dafx_load_1f(buf, ofs);
  pp.rotation_speed_max = dafx_load_1f(buf, ofs);
  pp.pull_speed_min = dafx_load_1f(buf, ofs);
  pp.pull_speed_max = dafx_load_1f(buf, ofs);
  return pp;
#endif
}

DAFX_INLINE
void modfx_velocity_force_field_vortex(
  ModfxParentSimData_cref parent_sdata, BufferData_cref buf, uint ofs, float life_k,
  rnd_seed_ref rnd_seed, float4x4_cref etm, float3_cref pos, float3_ref o_velocity )
{
  ModfxDeclVelocityForceFieldVortex pp = ModfxDeclVelocityForceFieldVortex_load( buf, ofs );

  float rotation_speed = lerp( pp.rotation_speed_min, pp.rotation_speed_max, dafx_frnd( rnd_seed ) );
  float pull_speed = lerp( pp.pull_speed_min, pp.pull_speed_max, dafx_frnd( rnd_seed ) );

  float3 axis_position = pp.axis_position + pp.position_rnd * dafx_srnd_vec3( rnd_seed );
  axis_position = mul_get_f3( float_xyz1( axis_position ), etm );
  float3 axis_direction = normalize( lerp( pp.axis_direction, dafx_srnd_vec3( rnd_seed ), pp.direction_rnd ) );
  axis_direction = mul_get_f3( float_xyz0( axis_direction ), etm );

  if (parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_FORCE_FIELD_VORTEX_ROTATION_SPEED])
    rotation_speed *= modfx_get_1f_curve(
      buf, parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_FORCE_FIELD_VORTEX_ROTATION_SPEED], life_k );

  if (parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_FORCE_FIELD_VORTEX_PULL_SPEED])
    pull_speed *= modfx_get_1f_curve(
      buf, parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_FORCE_FIELD_VORTEX_PULL_SPEED], life_k );

  float3 to_particle = pos - axis_position;
  float3 normal = to_particle - dot( axis_direction, to_particle ) * axis_direction;
  float radius = length( normal );
  static const float EPS = 0.00001;
  normal /= max(radius, EPS);
  float3 tangent = cross( axis_direction, normal );

  o_velocity = tangent * rotation_speed * radius - normal * pull_speed;
}

DAFX_INLINE
ModfxDeclWind ModfxDeclWind_load( BufferData_cref buf, uint ofs )
{
#ifdef __cplusplus
  return *(ModfxDeclWind*)( buf + ofs );
#else
  ModfxDeclWind pp;

  pp.directional_force = dafx_load_1f( buf, ofs );
  pp.directional_freq = dafx_load_1f( buf, ofs );

  pp.turbulence_force = dafx_load_1f( buf, ofs );
  pp.turbulence_freq = dafx_load_1f( buf, ofs );

  pp.impulse_force = dafx_load_1f( buf, ofs );
  return pp;
#endif
}

DAFX_INLINE
ModfxForceFieldNoise ModfxForceFieldNoise_load( BufferData_cref buf, uint ofs)
{
#ifdef __cplusplus
  return *(ModfxForceFieldNoise*)( buf + ofs );
#else
  ModfxForceFieldNoise pp;
  pp.usePosOffset = dafx_load_1ui(buf, ofs);
  pp.posScale = dafx_load_1f(buf, ofs);
  pp.powerScale = dafx_load_1f(buf, ofs);
  pp.powerRnd = dafx_load_1f(buf, ofs);
  pp.powerPerPartRnd = dafx_load_1f(buf, ofs);
  return pp;
#endif
}

DAFX_INLINE
void modfx_velocity_force_field_noise(
  ModfxParentSimData_cref parent_sdata, BufferData_cref buf, uint ofs, float life_k,
  rnd_seed_ref rnd_seed, float3_ref o_pos, float3_ref o_ofs_pos, float3_ref o_velocity, float dt)
{
  ModfxForceFieldNoise pp = ModfxForceFieldNoise_load(buf, ofs);
  float3 sample_pos = o_pos * pp.posScale;
  float4 simplex = simplex_perlin_3D_deriv(sample_pos);
  float atten = pp.powerScale;
  if ( parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_FORCE_FIELD_NOISE_CURVE] )
    atten *= modfx_get_1f_curve( buf, parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_FORCE_FIELD_NOISE_CURVE], life_k );

  atten *= lerp(1.f, simplex.x, pp.powerRnd);
  atten = lerp(atten, atten * dafx_frnd( rnd_seed ), pp.powerPerPartRnd);

  float3 v = float_yzw(simplex) * atten;
  if ( pp.usePosOffset )
    o_ofs_pos = v;
  else
    o_velocity += v * dt;
}


DAFX_INLINE
void modfx_velocity_wind_sim(
  ModfxParentSimData_cref parent_sdata,
  BufferData_cref buf, uint ofs, float life_k, rnd_seed_ref rnd_seed, GlobalData_cref gdata, float3_cref pos, float3_ref o_vel )
{
  ModfxDeclWind pp = ModfxDeclWind_load( buf, ofs );

  float d_force = pp.directional_force * gdata.wind_power;
  float3 wind_dir = float3( gdata.wind_dir.x, 0, gdata.wind_dir.y );
  float3 pos_f = float3( fabsf( pos.x ), fabsf( pos.y ), fabsf( pos.z ) );
  pos_f += gdata.wind_scroll * d_force * wind_dir;
  pos_f *= pp.turbulence_freq;

  rnd_seed_t ts_x = rnd_seed_t( abs( int( pos_f.x ) ) + rnd_seed ) % DAFX_RND_MAX;
  rnd_seed_t ts_y = rnd_seed_t( abs( int( pos_f.y ) ) + rnd_seed ) % DAFX_RND_MAX;
  rnd_seed_t ts_z = rnd_seed_t( abs( int( pos_f.z ) ) + rnd_seed ) % DAFX_RND_MAX;

  // intentionally non-normalized
  float3 rnd_vec = float3( dafx_srnd( ts_x ), dafx_srnd( ts_y ), dafx_srnd( ts_z ) );

  float atten = 1.f;
  if ( parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_WIND_ATTEN] )
    atten = modfx_get_1f_curve( buf, parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_WIND_ATTEN], life_k );

  o_vel = wind_dir * d_force * ( 1.f + dafx_frnd( rnd_seed ) * pp.directional_freq );
  o_vel += rnd_vec * pp.turbulence_force;
  o_vel *= atten;

#ifdef __cplusplus
  if (pp.impulse_force > 0 && modfx_get_additional_wind_at_pos)
    o_vel += modfx_get_additional_wind_at_pos(pos) * pp.impulse_force;
#endif
}


DAFX_INLINE
void modfx_velocity_force_resolver(
  float dt, float mass, float drag_c, float friction_k, float3 grav_vec, float3_ref o_pos, float3_ref o_vel )
{
  // Fd = 1/2 * (pv^2CdA) - ref drag equation
  const float c_p = 1.225; // air

  const float c_f = ( 0.5 * c_p * drag_c ) / mass;

  float vel_len = length( o_vel );
  const float3 vel_norm = vel_len > 0 ? o_vel * ( 1.f / vel_len ) : float3( 0, 0, 0 );

  const float iter_dt = dt;
  const float iter_dt_p2_half = iter_dt * iter_dt * 0.5f;

  // we cant just use v^2, since diagonal drag forces will weaker that those, which xyz axis aligned
  float drag_force = vel_len * vel_len * c_f;
  // total drag force  should not be greater that velocity len (it can happens due to high dt)
  const float drag_limit = 0.5;
  drag_force = min( drag_force, ( vel_len * drag_limit ) * ( 1.f / iter_dt ) );
  float3 drag_v = drag_force * vel_norm;

  // Störmer–Verlet
  float3 acc = ( -drag_v + grav_vec ) * friction_k;
  o_pos = o_pos + o_vel * iter_dt + acc * iter_dt_p2_half;
  o_vel = o_vel + acc * iter_dt;
}

DAFX_INLINE
void modfx_velocity_sim(
  ModfxParentSimData_cref parent_sdata, BufferData_cref buf, GlobalData_cref gdata,
  rnd_seed_ref rnd_seed, float life_k, float dt, float4x4_cref etm, bool etm_enabled,
  float radius, float3_ref o_pos, float3_ref o_ofs_pos, uint_ref o_sim_flags, float3_ref o_velocity, float_ref collision_time_norm )
{
  // Some velocity simulations should take place even for dt = 0.
  if ( dt < 0)
    return;

  if ( parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_FORCE_FIELD_NOISE] )
  {
    modfx_velocity_force_field_noise(
      parent_sdata, buf, parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_FORCE_FIELD_NOISE],
      life_k, rnd_seed, o_pos, o_ofs_pos, o_velocity, dt );
  }

  if ( dt <= 0)
    return;

  float mass = 0.f;
  float drag = 0.f;

  float3 grav_vec = float3( 0, 0, 0 );
  if ( FLAG_ENABLED( parent_sdata.flags, MODFX_SFLAG_VELOCITY_APPLY_GRAVITY ) )
  {
    const float g = -9.81f;
    bool grav_tr = FLAG_ENABLED( parent_sdata.flags, MODFX_SFLAG_VELOCITY_APPLY_GRAVITY_TRANSFORM );

    grav_vec = float3( 0, g, 0 );
    if ( etm_enabled && grav_tr )
      grav_vec = float3( etm[1][0], etm[1][1], etm[1][2] ) * g;
    else if ( !etm_enabled && grav_tr )
      grav_vec = dafx_get_3f( buf, parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_LOCAL_GRAVITY] ) * g;
  }

  if ( parent_sdata.mods_offsets[MODFX_SMOD_MASS_INIT] )
    mass = dafx_get_1f( buf, parent_sdata.mods_offsets[MODFX_SMOD_MASS_INIT] );

  if ( parent_sdata.mods_offsets[MODFX_SMOD_DRAG_INIT] )
    modfx_velocity_drag_init( buf, parent_sdata.mods_offsets[MODFX_SMOD_DRAG_INIT], radius, drag );

  if ( parent_sdata.mods_offsets[MODFX_SMOD_DRAG_OVER_PART_LIFE] )
    drag *= modfx_get_1f_curve( buf, parent_sdata.mods_offsets[MODFX_SMOD_DRAG_OVER_PART_LIFE], life_k );

  if ( parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_ADD] )
  {
    float3 add_v;;
    modfx_velocity_add(
      parent_sdata, buf, parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_ADD], rnd_seed, etm, etm_enabled, o_pos, add_v );

    if ( parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_DECAY] )
      add_v *= modfx_get_1f_curve( buf, parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_DECAY], life_k );

    o_velocity += add_v * dt;
  }

  if ( parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_FORCE_FIELD_VORTEX] )
  {
    float3 add_v;
    modfx_velocity_force_field_vortex(
      parent_sdata, buf, parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_FORCE_FIELD_VORTEX], life_k, rnd_seed, etm, o_pos, add_v );
    o_velocity += add_v * dt;
  }

  if ( parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_WIND] )
  {
    float3 add_v;
    modfx_velocity_wind_sim(
      parent_sdata, buf, parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_WIND], life_k, rnd_seed, gdata, o_pos, add_v );
    o_velocity += add_v * dt;
  }

  float friction_k = 1.f;
#if MODFX_GPU_FEATURES_ENABLED
  if ( parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_SCENE_COLLISION] )
  {
    friction_k = modfx_scene_collision_sim(
      buf, parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_SCENE_COLLISION], etm, etm_enabled, radius, o_pos, o_velocity, dt, gdata );
    if (friction_k<1)
    {
      if ( !(o_sim_flags & MODFX_SIM_FLAGS_COLLIDED) && MODFX_SDECL_COLLISION_TIME_ENABLED( parent_sdata.decls ) )
        collision_time_norm = life_k;
      o_sim_flags |= MODFX_SIM_FLAGS_COLLIDED;
    }
  }
#endif

  if ( FLAG_ENABLED( parent_sdata.flags, MODFX_SFLAG_VELOCITY_USE_FORCE_RESOLVER ) )
  {
    modfx_velocity_force_resolver( dt, mass, drag, friction_k, grav_vec, o_pos, o_velocity );
  }
  else
  {
    o_velocity += grav_vec * dt;
    o_pos += o_velocity * dt;
  }

  G_UNREFERENCED( gdata );
  G_UNREFERENCED( o_sim_flags );
  G_UNREFERENCED(collision_time_norm);
}

DAFX_INLINE
ModfxDeclShapeVelocityMotion ModfxDeclShapeVelocityMotion_load( BufferData_cref buf, uint ofs )
{
#ifdef __cplusplus
  return *(ModfxDeclShapeVelocityMotion*)( buf + ofs );
#else
  ModfxDeclShapeVelocityMotion pp;
  pp.length_mul = dafx_load_1f( buf, ofs );
  pp.velocity_clamp = dafx_load_2f( buf, ofs );
  return pp;
#endif
}

DAFX_INLINE
void modfx_velocity_to_render( ModfxParentSimData_cref parent_sdata, BufferData_cref buf,
  float3_cref vel, float3_ref o_up_vec, float_ref o_vel_length )
{
  if ( FLAG_ENABLED( parent_sdata.flags, MODFX_SFLAG_PASS_VELOCITY_TO_RDECL ) )
  {
    o_vel_length = length( vel );
    o_up_vec = o_vel_length > 0.0001 ? vel / o_vel_length : float3( 0, 0, 0 );
  }

  uint ofs = parent_sdata.mods_offsets[MODFX_SMOD_SHAPE_VELOCITY_MOTION_INIT];
  if ( ofs )
  {
    ModfxDeclShapeVelocityMotion pp = ModfxDeclShapeVelocityMotion_load( buf, ofs );
    float d = pp.velocity_clamp.y - pp.velocity_clamp.x;
    float v = o_vel_length - pp.velocity_clamp.x;
    float k = d > 0 ? ( v / d ) : ( v > 0 );
    o_vel_length = saturate( k ) * pp.length_mul;
  }
}

#endif
