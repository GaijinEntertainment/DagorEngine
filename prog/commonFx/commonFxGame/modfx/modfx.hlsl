#ifndef DAFX_MODFX_HLSL
#define DAFX_MODFX_HLSL

#include "modfx_misc.hlsli"
#include "modfx_curve.hlsli"
#include "modfx_pos.hlsli"
#include "modfx_life.hlsli"
#include "modfx_radius.hlsli"
#include "modfx_color.hlsli"
#include "modfx_rotation.hlsli"
#include "modfx_velocity.hlsli"
#include "modfx_frame.hlsli"
#include "modfx_trimming.hlsli"
#include "modfx_part_data.hlsli"
#include "modfx_toroidal_movement.hlsli"
#include "modfx_render_placement.hlsli"

DAFX_INLINE
float4 modfx_get_culling( float3_cref wpos, float radius )
{
  return float4( wpos.x, wpos.y, wpos.z, radius );
}

ModfxDeclCollisionDecay ModfxDeclCollisionDecay_load( BufferData_cref buf, uint ofs )
{
  ModfxDeclCollisionDecay pp;
  pp.collision_decay_scale = dafx_load_1f( buf, ofs );
  return pp;
}

DAFX_INLINE
void modfx_apply_sim(
  ComputeCallDesc_cref cdesc,
  BufferData_ref buf,
  DAFX_CREF(ModfxParentRenData) parent_rdata,
  DAFX_CREF(ModfxParentSimData) parent_sdata,
  DAFX_REF(ModfxRenData) rdata,
  DAFX_REF(ModfxSimData) sdata,
  float dt,
  float4x4_cref etm,
  bool etm_enabled,
  bool attach_last_part,
  bool is_emission,
  GlobalData_cref gdata )
{
  modfx_life_sim( parent_sdata, buf, sdata.rnd_seed, cdesc.lifeLimitRcp, dt, sdata.life_norm );

  bool dead = sdata.life_norm > 1.f;
  sdata.life_norm = saturate( sdata.life_norm ); // we still need to fill render_data with valid values

  if ( MODFX_RDECL_RADIUS_ENABLED( parent_rdata.decls ) )
    modfx_radius_sim( parent_sdata, buf, sdata.rnd_seed, sdata.life_norm, rdata.radius );

  if ( MODFX_RDECL_COLOR_ENABLED( parent_rdata.decls ) || MODFX_RDECL_EMISSION_FADE_ENABLED( parent_rdata.decls ) )
  {
    float part_distr_k = cdesc.aliveCount > 1 ?
      saturate( 1.f - float( cdesc.idx + cdesc.start - cdesc.aliveStart ) / (float)(cdesc.aliveCount - 1) ) : 0.f;

    float grad_k = FLAG_ENABLED( parent_sdata.flags, MODFX_SFLAG_COLOR_GRAD_USE_PART_IDX_AS_KEY ) ? part_distr_k : sdata.life_norm;
    float curve_k = FLAG_ENABLED( parent_sdata.flags, MODFX_SFLAG_COLOR_CURVE_USE_PART_IDX_AS_KEY ) ? part_distr_k : sdata.life_norm;

    modfx_color_sim( parent_sdata, buf, sdata.rnd_seed, grad_k, curve_k, sdata.life_norm, part_distr_k, sdata.em_color, rdata.color, rdata.emission_fade );
  }

  bool sdeclVelocity = MODFX_SDECL_VELOCITY_ENABLED( parent_sdata.decls );
  bool rdeclPosOffset = MODFX_RDECL_POS_OFFSET_ENABLED( parent_rdata.decls );

  if (sdeclVelocity || rdeclPosOffset)
  {
    bool fullSim = !attach_last_part && !is_emission;
    if (fullSim || (rdeclPosOffset && !attach_last_part)) // we cant rollback lag dt for velocity, otherwise they will apeears in front of emitter
      modfx_velocity_sim( parent_sdata, buf, gdata, sdata.rnd_seed, sdata.life_norm, fullSim ? dt : 0, etm, etm_enabled, rdata.radius, rdata.pos, rdata.pos_offset, sdata.flags, sdata.velocity, sdata.collision_time_norm );
  }

  if ( sdeclVelocity )
  {
    if ( MODFX_SDECL_COLLISION_TIME_ENABLED( parent_sdata.decls ) && (sdata.flags & MODFX_SIM_FLAGS_COLLIDED) && parent_sdata.mods_offsets[MODFX_SMOD_COLLISION_DECAY])
    {
      ModfxDeclCollisionDecay decayParams = ModfxDeclCollisionDecay_load( buf, parent_sdata.mods_offsets[MODFX_SMOD_COLLISION_DECAY] );

      float fadeTime = min(sdata.collision_time_norm, 1 - sdata.collision_time_norm); // assuming the next part will collide approx the same time, it fades out in that interval (or sooner)
      fadeTime *= decayParams.collision_decay_scale; // extra decay speed, eg. to instantly remove parts on collision

      float collisionScale = saturate(1 - (sdata.life_norm - sdata.collision_time_norm) / max(fadeTime, 0.0001f));
      if ( FLAG_ENABLED( parent_rdata.flags, MODFX_RFLAG_BLEND_PREMULT ) )
        rdata.color *= collisionScale;
      else
        rdata.color.w *= collisionScale;
      if (collisionScale <= 0)
        rdata.radius = 0;
    }

    modfx_velocity_to_render( parent_sdata, buf, sdata.velocity, rdata.up_vec, rdata.velocity_length );
  }

  if ( MODFX_RDECL_ANGLE_ENABLED( parent_rdata.decls ) ) // MODFX_SDECL_COLLISION_TIME_ENABLED(decls) is implicitly true
  {
    float angle_life_k = sdata.life_norm;
#ifndef __cplusplus // only for gpu sim collision
    if (sdata.flags & MODFX_SIM_FLAGS_COLLIDED)
      angle_life_k = sdata.collision_time_norm; // rollback life to collision time
#endif
    modfx_rotation_sim( parent_sdata, buf, sdata.rnd_seed, angle_life_k, rdata.angle );
  }

  if ( MODFX_RDECL_FRAME_IDX_ENABLED( parent_rdata.decls ) || MODFX_RDECL_FRAME_FLAGS_ENABLED( parent_rdata.decls ) )
    modfx_frame_sim( parent_rdata, parent_sdata, buf, sdata.rnd_seed, sdata.life_norm, rdata.frame_idx, rdata.frame_flags, rdata.frame_blend );

  if ( FLAG_ENABLED( parent_sdata.flags, MODFX_SFLAG_RADIUS_APPLY_EMITTER_TRANSFORM ) )
    rdata.radius *= length( float3( etm[0][0], etm[0][1], etm[0][2] ) );

  if ( attach_last_part )
    rdata.pos = etm_enabled ? float3( etm[3][0], etm[3][1], etm[3][2] ) : float3( 0, 0, 0 );

  rdata.life_norm = sdata.life_norm;

  if ( MOD_ENABLED( parent_sdata.mods, MODFX_SMOD_PART_TRIMMING ) )
    modfx_part_trimming( parent_rdata, parent_sdata, buf, cdesc.gid, sdata.life_norm, rdata );

  if ( FLAG_ENABLED( parent_sdata.flags, MODFX_SFLAG_TOROIDAL_MOVEMENT ) && etm_enabled )
    modfx_toroidal_movement_sim( parent_sdata, buf, float3( etm[3][0], etm[3][1], etm[3][2] ), rdata.pos );

  if ( dead )
    rdata.radius = 0;

  G_UNREFERENCED(gdata);
}

DAFX_INLINE
BBox dafx_emission_shader_cb(
  ComputeCallDesc_cref cdesc,
  BufferData_ref buf,
  DAFX_CREF(ModfxParentRenData) parent_rdata,
  DAFX_CREF(ModfxParentSimData) parent_sdata,
  float lag_comp,
  GlobalData_cref gdata )
{
  ModfxRenData rdata;
  rdata.pos = float3( 0, 0, 0 );
  rdata.radius = 1.f;
  rdata.angle = 0.f;
  rdata.color = float4( 1.f, 1.f, 1.f, 1.f );
  rdata.frame_idx = 0;
  rdata.frame_flags = 0;
  rdata.frame_blend = 0;
  rdata.life_norm = 0;
  rdata.emission_fade = 1;
  rdata.up_vec = float3( 0, 0, 0 );
  rdata.right_vec = float3( 0, 0, 0 );
  rdata.pos_offset = float3( 0, 0, 0 );
  rdata.unique_id = 0;

  ModfxSimData sdata;
  sdata.life_norm = 0;
  sdata.flags = 0;
  sdata.rnd_seed = dafx_calc_instance_rnd_seed( cdesc.gid, cdesc.rndSeed );
  sdata.velocity = float3( 0, 0, 0 );
  sdata.em_color = float4( 1.f, 1.f, 1.f, 1.f );
  sdata.collision_time_norm = 0;

  bool apply_etm = !FLAG_ENABLED( parent_rdata.flags,MODFX_RFLAG_USE_ETM_AS_WTM );
  float4x4 etm = dafx_get_44mat( buf, parent_rdata.mods_offsets[MODFX_RMOD_INIT_TM] );

  // trail and unique id
  int serviceLoad_ofs = 0;

  bool last_sim_part = cdesc.idx == ( cdesc.count - 1 );
  float3 prev_last_emitter_pos;
  float3 cur_emitter_pos = float3( etm[3][0], etm[3][1], etm[3][2] );
  if ( FLAG_ENABLED( parent_sdata.flags, MODFX_SFLAG_TRAIL_ENABLED ) )
  {
    ModfxDeclServiceTrail pp = ModfxDeclServiceTrail_load( buf, cdesc.serviceDataOffset + serviceLoad_ofs );
    serviceLoad_ofs += MODFX_SERVICE_TRAIL_SIZE / 4;

    if ( last_sim_part )
      prev_last_emitter_pos = pp.last_emitter_pos;

    if ( cdesc.count > 1 && pp.flags & MODFX_TRAIL_FLAG_LAST_POS_VALID )
    {
      float k = (float)( cdesc.idx + 1 ) / (float)cdesc.count;
      etm[3][0] = lerp( pp.last_emitter_pos.x, cur_emitter_pos.x, k );
      etm[3][1] = lerp( pp.last_emitter_pos.y, cur_emitter_pos.y, k );
      etm[3][2] = lerp( pp.last_emitter_pos.z, cur_emitter_pos.z, k );

      lag_comp = 0.f;
    }
  }
  if ( MODFX_RDECL_UNIQUE_ID_ENABLED( parent_rdata.decls ) )
  {
    ModfxDeclServiceUniqueId pp = ModfxDeclServiceUniqueId_load( buf, cdesc.serviceDataOffset + serviceLoad_ofs );
    serviceLoad_ofs += MODFX_SERVICE_UNIQUE_ID_SIZE / 4;

    rdata.unique_id = pp.particles_emitted + cdesc.idx;
  }

  modfx_life_init( parent_sdata, buf, sdata.rnd_seed, sdata.life_norm );

  float3 pos_v = float3( 0, 0, 0 );
  modfx_pos_init( parent_sdata, buf, sdata.rnd_seed, cdesc.rndSeed, rdata.pos, pos_v );

  if ( apply_etm )
    rdata.pos = float_xyz( mul( float4( rdata.pos, 1.f ), etm ) );

#if MODFX_GPU_FEATURES_ENABLED
  bool allow_spawn = modfx_pos_gpu_placement(parent_sdata, buf, etm[3].y, rdata.pos, gdata);
#endif

  if ( parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_INIT] )
  {
    modfx_velocity_init(
      parent_sdata, buf, parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_INIT], sdata.rnd_seed,
      etm, apply_etm, rdata.pos, pos_v, sdata.velocity );

    if ( apply_etm )
      sdata.velocity = float_xyz( mul( float4( sdata.velocity, 0 ), etm ) );
  }

  if ( parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_INIT_ADD_STATIC_VEC] )
    sdata.velocity += dafx_get_3f( buf, parent_sdata.mods_offsets[MODFX_SMOD_VELOCITY_INIT_ADD_STATIC_VEC] );

  if ( parent_sdata.mods_offsets[MODFX_SMOD_SHAPE_STATIC_ALIGNED_INIT] )
  {
    ModfxDeclShapeStaticAlignedInit pp = ModfxDeclShapeStaticAlignedInit_load( buf, parent_sdata.mods_offsets[MODFX_SMOD_SHAPE_STATIC_ALIGNED_INIT] );
    rdata.up_vec = pp.up_vec;
    rdata.right_vec = pp.right_vec;

    if ( apply_etm )
    {
      rdata.up_vec = float_xyz( mul( float4( rdata.up_vec, 0 ), etm ) );
      rdata.right_vec = float_xyz( mul( float4( rdata.right_vec, 0 ), etm ) );
      float up_len = length( rdata.up_vec );
      float right_len = length( rdata.right_vec );

      rdata.up_vec = up_len > 0 ? rdata.up_vec / up_len : float3( 0.f, 0.f, 0.f );
      rdata.right_vec = right_len > 0 ? rdata.right_vec / right_len : float3( 0.f, 0.f, 0.f );
    }
  }

  if ( parent_sdata.mods_offsets[MODFX_SMOD_SHAPE_ADAPTIVE_ALIGNED_INIT] )
  {
    rdata.up_vec = dafx_get_3f( buf, parent_sdata.mods_offsets[MODFX_SMOD_SHAPE_ADAPTIVE_ALIGNED_INIT] );
    if ( apply_etm )
      rdata.up_vec = float_xyz( mul( float4( rdata.up_vec, 0 ), etm ) );
  }

  if ( parent_sdata.mods_offsets[MODFX_SMOD_COLOR_EMISSION_OVERRIDE] )
    sdata.em_color = unpack_e3dcolor_to_n4f( dafx_get_1ui( buf, parent_sdata.mods_offsets[MODFX_SMOD_COLOR_EMISSION_OVERRIDE] ) );

  // sim only stable seed
  sdata.rnd_seed = dafx_calc_instance_rnd_seed( cdesc.gid, sdata.rnd_seed );
  rnd_seed_t stable_rnd_seed = sdata.rnd_seed;

#if MODFX_GPU_FEATURES_ENABLED
  if (!allow_spawn)
  {
    sdata.life_norm = 2.f; // >cdesc.lifeLimit
    rdata.radius = 0.f;
  }
#endif

  bool attach_last_part = FLAG_ENABLED( parent_sdata.flags, MODFX_SFLAG_ATTACH_LAST_PART_TO_EMITTER ) && ( cdesc.idx == cdesc.count - 1 );
  modfx_apply_sim(
    cdesc, buf, parent_rdata, parent_sdata, rdata, sdata, lag_comp * gdata.dt, etm, apply_etm, attach_last_part, true, gdata );

  sdata.rnd_seed = stable_rnd_seed;
  modfx_save_ren_data(buf, cdesc.dataRenOffsetCurrent, parent_rdata.decls, parent_sdata.flags, true, rdata);
  modfx_save_sim_data(buf, cdesc.dataSimOffsetCurrent, parent_sdata.decls, true, sdata);

  // only last part can overwrite service (shared) data, so we can skip mem barrier usage
  if ( last_sim_part )
  {
    int serviceSave_ofs = 0;
    if ( FLAG_ENABLED( parent_sdata.flags, MODFX_SFLAG_TRAIL_ENABLED ) )
    {
      ModfxDeclServiceTrail pp;
      pp.prev_last_emitter_pos = prev_last_emitter_pos;
      pp.last_emitter_pos = cur_emitter_pos;
      pp.flags = MODFX_TRAIL_FLAG_LAST_POS_VALID | MODFX_TRAIL_FLAG_EMITTED_THIS_FRAME;

      ModfxDeclServiceTrail_save( pp, buf, cdesc.serviceDataOffset + serviceSave_ofs );
      serviceSave_ofs += MODFX_SERVICE_TRAIL_SIZE / 4;
    }
    if ( MODFX_RDECL_UNIQUE_ID_ENABLED( parent_rdata.decls ) )
    {
      ModfxDeclServiceUniqueId pp;
      pp.particles_emitted = rdata.unique_id + 1;

      ModfxDeclServiceUniqueId_save( pp, buf, cdesc.serviceDataOffset + serviceSave_ofs );
      serviceSave_ofs += MODFX_SERVICE_UNIQUE_ID_SIZE / 4;
    }
  }

  float3 cull_pos = apply_etm ? rdata.pos : float_xyz( mul( float4( rdata.pos, 1.f ), etm ) );
  float4 cull = modfx_get_culling( cull_pos, rdata.radius );
  return modfx_apply_placement_to_culling( parent_rdata, buf, cull );
}

DAFX_INLINE
BBox dafx_simulation_shader_cb(
  ComputeCallDesc_cref cdesc,
  BufferData_ref buf,
  DAFX_CREF(ModfxParentRenData) parent_rdata,
  DAFX_CREF(ModfxParentSimData) parent_sdata,
  GlobalData_cref gdata )
{
  ModfxSimData sdata;
  modfx_load_sim_data(buf, cdesc.dataSimOffsetCurrent, parent_sdata.decls, sdata);

  ModfxRenData rdata;
#ifdef __cplusplus
  memset(&rdata, 0, sizeof(ModfxRenData));
#else
  rdata = (ModfxRenData)0;
#endif
  rdata.radius = 1.f;
  rdata.color = float4(1, 1, 1, 1);
  rdata.emission_fade = 1.f;
  if (MODFX_RDECL_POS_ENABLED(parent_rdata.decl))
    rdata.pos = dafx_get_3f(buf, cdesc.dataRenOffsetCurrent);

  bool apply_etm = !FLAG_ENABLED( parent_rdata.flags,MODFX_RFLAG_USE_ETM_AS_WTM );
  float4x4 etm = dafx_get_44mat( buf, parent_rdata.mods_offsets[MODFX_RMOD_INIT_TM] );

  bool last_sim_part = cdesc.idx == ( cdesc.count - 1 );
  bool attach_last_part = FLAG_ENABLED( parent_sdata.flags, MODFX_SFLAG_ATTACH_LAST_PART_TO_EMITTER ) && last_sim_part;
  if ( FLAG_ENABLED( parent_sdata.flags, MODFX_SFLAG_TRAIL_ENABLED ) && attach_last_part )
  {
    // This is code is very sensitive to emission/simulation order. If emission is not first - it wont work!
    ModfxDeclServiceTrail pp = ModfxDeclServiceTrail_load( buf, cdesc.serviceDataOffset );
    if ( pp.flags & MODFX_TRAIL_FLAG_EMITTED_THIS_FRAME )
    {
      pp.flags &= ~MODFX_TRAIL_FLAG_EMITTED_THIS_FRAME;
      rdata.pos = pp.prev_last_emitter_pos;
      ModfxDeclServiceTrail_save( pp, buf, cdesc.serviceDataOffset );
      attach_last_part = false;
    }
  }

  modfx_apply_sim(
    cdesc, buf, parent_rdata, parent_sdata, rdata, sdata, gdata.dt, etm, apply_etm, attach_last_part, false, gdata );

  modfx_save_ren_data(buf, cdesc.dataRenOffsetCurrent, parent_rdata.decls, parent_sdata.flags, false, rdata);
  modfx_save_sim_data(buf, cdesc.dataSimOffsetCurrent, parent_sdata.decls, false, sdata);

  float3 cull_pos = apply_etm ? rdata.pos : float_xyz( mul( float4( rdata.pos, 1.f ), etm ) );
  float4 cull = modfx_get_culling( cull_pos, rdata.radius );
  return modfx_apply_placement_to_culling( parent_rdata, buf, cull );
}

#endif