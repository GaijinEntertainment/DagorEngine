#ifndef DAFX_STD_SHADER_ABLEND
#define DAFX_STD_SHADER_ABLEND 0
#endif

#ifndef DAFX_STD_SHADER_ADDITIVE
#define DAFX_STD_SHADER_ADDITIVE 1
#endif

#ifndef DAFX_STD_SHADER_ADDSMOOTH
#define DAFX_STD_SHADER_ADDSMOOTH 2
#endif

#ifndef DAFX_STD_SHADER_ATEST
#define DAFX_STD_SHADER_ATEST 3
#endif

#ifndef DAFX_STD_SHADER_PREMULTALPHA
#define DAFX_STD_SHADER_PREMULTALPHA 4
#endif

#define MIN_VEL_MUL 0.01
#define MAX_DIST_TO_PREV_RADII  4

//
// STD Cubic Curve
//
DAFX_INLINE
float std_cubic_curve_compute( float x, float4_cref c )
{
  return ((c.w*x+c.z)*x+c.y)*x+c.x;
}

DAFX_INLINE
float std_cubic_curve_sample( StdCubicCurve_cref p, float x )
{
  if (p.type == 1)
    return ((p.coef[3]*x+p.coef[2])*x+p.coef[1])*x+p.coef[0];
  if (p.type == 0)
    return p.coef[0];

  if (x <= p.coef[0])
    return std_cubic_curve_compute( x, p.spline_coef[0] );

  int c = 0;

  for (int i = 1, ie = p.type<5 ? p.type : 4; i < ie; i++)
  {
    if (x < p.coef[i])
    {
      x -= p.coef[i-1];
      c += i;
      return std_cubic_curve_compute( x, p.spline_coef[c] );
    }
  }

  x -= p.coef[p.type-2];
  c += (p.type-1);
  return std_cubic_curve_compute( x, p.spline_coef[c] );
};

//
// STD Emitter
//
DAFX_INLINE
void std_emitter_emit_point( StdEmitter_cref em, float3_cref rnd_vec, float3_ref o_pos, float3_ref o_norm, float3_ref o_vel )
{
  const uint GEOM_SPHERE = 0;
  const uint GEOM_CYLINDER = 1;

  float u = rnd_vec.x;
  float v = rnd_vec.y;
  float w = rnd_vec.z;

  float3 pos, normal;
  float a = dafx_2pi * u;
  float c, s;
  sincos( a, s, c );


  if ( em.geometryType == GEOM_SPHERE )
  {
    float r = sqrt( v * ( 1.f - v ) ) * 2.f;

    float3 dir = float3( c * r, s * r, 1.f - 2.f * v );

    if ( em.isVolumetric )
      pos = dir * ( em.radius * sqrt( w ) );
    else
      pos = dir * em.radius;

    normal = dir;
  }
  else if ( em.geometryType == GEOM_CYLINDER )
  {
    float3 dir = float3( c, 0, s );

    if ( em.isVolumetric )
      pos = dir * ( em.radius * sqrt( v ) ) + float3( 0, ( w-0.5f ) * em.size.y, 0 );
    else
      pos = dir * em.radius + float3( 0, ( v - 0.5f ) * em.size.y, 0 );

    normal = dir;
  }
  else
  {
    pos = float3( ( u - 0.5f ) * em.size.x, ( v - 0.5f ) * em.size.y, ( w - 0.5f ) * em.size.z );
    normal = float3( 0, 1, 0 );
  }


  o_pos = float_xyz( mul( float4( pos, 1 ), em.worldTm ) );
  o_norm = float_xyz( mul( float4( normal, 1 ), em.normalTm ) );
  o_vel = em.emitterVel;
}

// Features
#if FLOWPS2_GPU_FEATURES
void flowps2_apply_depth_collision( float3 wpos, float radius, in out float3 vel, float atten_k, float decay_k, GlobalData gparams )
{
  float4 opos = mul( float4( wpos, 1 ), gparams.globtm );
  float2 stc = float2( opos.xy * float2(0.5,-0.5) + float2(0.5, 0.5) * opos.w) / opos.w;

  if ( stc.x < 0 || stc.x >= 1 || stc.y < 0 || stc.y >= 1 )
    return;

  float scene_depth = linearize_z( texelFetchOffset( g_depth_tex, stc * gparams.depth_size, 0, 0 ).r, gparams.zn_zfar.zw );
  float3 scene_normal = texelFetch( g_normals_tex, stc * gparams.normals_size, 0 ).xyz * 2.f - 1.f;
  float dist = scene_depth - linearize_z( opos.z / opos.w, gparams.zn_zfar.zw );

  if ( dist < 0 )
  {
    // part is behind the wall
    // ninja hack
    dist = -dist;
    scene_normal = -scene_normal;
  }

  if ( dist > radius )
    return;

  float pvel_length = length( vel );
  float3 pvel_norm = pvel_length > 0 ? vel * rcp( pvel_length ) : 0;

  float d = dot( pvel_norm, scene_normal );
  if ( d > 0 )
    return;

  float3 refl = reflect( pvel_norm, scene_normal );

  float atten = 1.f - dist / radius;
  vel = normalize( lerp( pvel_norm, refl, saturate( atten * atten_k ) ) );
  vel *= pvel_length * decay_k;
}
#endif

//
// Flowps2
//
DAFX_INLINE
float4 flowps2_pack_null( RenData_ref rp, SimData_ref sp, ComputeCallDesc_cref cdesc, BufferData_ref ldata )
{
  rp.pos = float3( 0, 0, 0 );
  rp.color = float4( 0, 0, 0, 0 );
  rp.radius = 0;
  rp.angle = 0;
  rp.frameNo = 0;
  rp.frameDuration = 0;
  rp.lengthening = float3( 0, 0, 0 );
  rp.rnd = 0;

  sp.vel = float3( 0, 0, 0 );
  sp.life = 0;
  sp.wvel = 0;
  sp.fxScale = 0;
  sp.startColor = float4( 0, 0, 0, 0 );
  sp.angle = 0;

  pack_ren_data( rp, ldata, cdesc.dataRenOffsetCurrent );
  pack_sim_data( sp, ldata, cdesc.dataSimOffsetCurrent );

  return float4( 0, 0, 0, 0 );
}

DAFX_INLINE
void flowps2_calculate_size( RenData_ref rp, SimData_cref sp, ParentRenData_cref rparams, ParentSimData_cref sparams, float time )
{
  rp.radius = sparams.size.radius * std_cubic_curve_sample( sparams.size.sizeFunc, time ) * sp.fxScale;

  uint totalFrames = rparams.framesX * rparams.framesY;

  float phase = rp.rnd * sparams.randomPhase + sparams.startPhase;
  float curStage = totalFrames * (time * sparams.animSpeed + phase);
  uint frame = floor(curStage);
  bool blend = sparams.animatedFlipbook && sparams.animSpeed > 0 && time < ( 1.f -  1.f / ( totalFrames * sparams.animSpeed ) );
  rp.frameNo = frame % totalFrames;
  rp.frameDuration = blend ? (curStage - float(frame)) : 0;
}

DAFX_INLINE
void flowps2_calculate_color( RenData_ref rp, SimData_ref sp, ParentSimData_cref sparams, float time)
{
  const float heightRange =
    (sparams.alphaHeightRange.x >= 0 && sparams.alphaHeightRange.y > sparams.alphaHeightRange.x) ?
    (sparams.alphaHeightRange.y - sparams.alphaHeightRange.x) : (-1.f);

  float heightAlpha = 1.f;
  if (heightRange > 0.f)
  {
    if (rp.pos.y <= sparams.alphaHeightRange.x)
      heightAlpha = 0.f;
    else if (rp.pos.y < sparams.alphaHeightRange.y)
      heightAlpha = (rp.pos.y - sparams.alphaHeightRange.x) / heightRange;
  }

  float alpha = std_cubic_curve_sample( sparams.color.aFunc, time ) * sp.startColor.w;

  rp.color = float4(
    std_cubic_curve_sample( sparams.color.rFunc, time) * sp.startColor.x,
    std_cubic_curve_sample( sparams.color.gFunc, time) * sp.startColor.y,
    std_cubic_curve_sample( sparams.color.bFunc, time) * sp.startColor.z,
    alpha * heightAlpha);
}

DAFX_INLINE
void flowps2_calculate_lengthening( RenData_ref rp, SimData_ref sp, ParentSimData_cref sparams )
{
  bool isTrail = sparams.emitter.isTrail;
  rp.lengthening =
    sparams.size.lengthDt * ( isTrail ? sp.vel * rp.radius : sp.vel * ( sp.fxScale > 0 ? rp.radius / sp.fxScale : 0 ) );
}

DAFX_INLINE
bool flowps2_check_alpha( RenData_cref rp, ParentRenData_cref rparams )
{
  float a = ( rparams.shader == DAFX_STD_SHADER_ABLEND || rparams.shader == DAFX_STD_SHADER_ATEST ) ?
    rp.color.w : ( rp.color.x + rp.color.y + rp.color.z );

  a += rparams.shader == DAFX_STD_SHADER_PREMULTALPHA ? rp.color.w : 0;
  return a > 0;
}

DAFX_INLINE
float4 flowps2_get_culling( RenData_cref rp, float3_cref wpos, float3_cref lengthening )
{
  if ( rp.radius <= 0 )
    return float4( 0, 0, 0, 0 );

  return float4( wpos.x, wpos.y, wpos.z, max( rp.radius, length( lengthening ) * 2.f ) );
}

DAFX_INLINE
float4 dafx_emission_shader_cb( ComputeCallDesc_cref cdesc, BufferData_ref ldata, GlobalData_cref gparams )
{
  G_UNREFERENCED(gparams);

  #ifdef __cplusplus
    const ParentRenData &rparams = *((ParentRenData*)( ldata + cdesc.parentRenOffset ));
    const ParentSimData &sparams = *((ParentSimData*)( ldata + cdesc.parentSimOffset ));

    RenData rp;
    SimData sp;
  #else
    ParentRenData rparams;
    ParentSimData sparams;
    unpack_parent_ren_data( rparams, ldata, cdesc.parentRenOffset );
    unpack_parent_sim_data( sparams, ldata, cdesc.parentSimOffset );

    RenData rp = (RenData)0;
    SimData sp = (SimData)0;
  #endif

  float3 pos;
  float3 norm;
  float3 vel;

  rnd_seed_t rnd_seed = dafx_calc_instance_rnd_seed( cdesc.gid, cdesc.rndSeed );
  sp.life = 0;
  rp.radius = 0;

  bool isTrail = sparams.emitter.isTrail;

  std_emitter_emit_point( sparams.emitter, dafx_frnd_vec3( rnd_seed ), pos, norm, vel );

  if ( isTrail )
  {
    uint serviceOfs = cdesc.serviceDataOffset;
    float3 curEmitterPos = float_xyz( float4x4_row( sparams.emitter.worldTm, 3 ) );

    bool isPrevPosValid = dafx_load_1ui( ldata, serviceOfs );

    float3 prevEmitterPos = isPrevPosValid ? dafx_load_3f( ldata, serviceOfs ) : float3( 0, 0, 0 );
    float3 prevSpawnPos = isPrevPosValid ? dafx_load_3f( ldata, serviceOfs ) : float3( 0, 0, 0 );
    float3 v = prevEmitterPos - curEmitterPos;
    uint spawnCounter = cdesc.count - cdesc.idx;

    if ( isPrevPosValid )
      pos += v * ( float( spawnCounter - 1 ) / float( cdesc.count ) );

    bool isMoving = true;
    if ( sparams.size.lengthDt > 0.f && isPrevPosValid )
    {
      vel = pos - prevSpawnPos;
      float velLenSq = dot( vel, vel );
      if (velLenSq >
        sparams.size.radius * sparams.size.radius * MAX_DIST_TO_PREV_RADII * MAX_DIST_TO_PREV_RADII)
      {
        vel *= sparams.size.radius * MAX_DIST_TO_PREV_RADII / length( vel );
      }
      else if (velLenSq < MIN_VEL_MUL * MIN_VEL_MUL)
      {
        isMoving = false;
      }
    }

    // only last part in job can write to service data, so we won't uses barriers
    if ( cdesc.idx == ( cdesc.count - 1 ) )
    {
      uint serviceOfs = cdesc.serviceDataOffset;
      dafx_store_1ui( 1, ldata, serviceOfs );
      dafx_store_3f( curEmitterPos, ldata, serviceOfs );
      dafx_store_3f( pos, ldata, serviceOfs );
    }

    if ( !isPrevPosValid || !isMoving )
      return flowps2_pack_null( rp, sp, cdesc, ldata );
  }

  if ( sparams.alphaHeightRange.x >= 0 && sparams.alphaHeightRange.y > sparams.alphaHeightRange.x )
  {
    float range = sparams.alphaHeightRange.y - sparams.alphaHeightRange.x;
    float a = (pos.y - sparams.alphaHeightRange.x) / range;
    if (a <= 1e-2f)
      return flowps2_pack_null( rp, sp, cdesc, ldata );
  }

  if ( sparams.onMoving && dot(sparams.emitter.emitterVel, sparams.emitter.emitterVel) < 10.f )
      return flowps2_pack_null( rp, sp, cdesc, ldata );

  sp.fxScale = sparams.fxScale;

  if ( isTrail )
  {
    sp.vel = vel;
    rp.pos = pos;
  }
  else
  {
    float fxScale = sparams.isUserScale ? sparams.fxScale : 1.f;

    sp.vel = vel +
      (normalize(dafx_srnd_vec3(rnd_seed))*sparams.randomVel +
      sparams.startVel + norm*sparams.normalVel) * fxScale;

    float radius = sparams.size.radius * std_cubic_curve_sample( sparams.size.sizeFunc, 0 );
    rp.pos = pos + sp.vel * sparams.size.lengthDt * radius;
  }

  float3 rvec;
  rvec.x = dafx_srnd( rnd_seed );
  rvec.y = dafx_srnd( rnd_seed );
  rvec.z = dafx_frnd( rnd_seed );
  rp.rnd = rvec.z;

  float maxLife = clamp(sparams.life + rp.rnd * sparams.randomLife, 0.f, sparams.life + sparams.randomLife);
  sp.life = maxLife;
  sp.angle = rvec.x * dafx_pi * sparams.randomRot;
  // rp.angle is only 2 bytes, not good for simulation, but enough for render
  rp.angle = sp.angle;

  sp.wvel = rvec.y * sparams.rotSpeed;
  float colorLerp_ = dafx_frnd(rnd_seed);
  float4 color1 = sparams.color.color;
  float4 color2 = sparams.color.color2;

  float4 finalColor = color1 * (1.f - colorLerp_) + color2 * colorLerp_;
  finalColor.x *= sparams.colorMult.x;
  finalColor.y *= sparams.colorMult.y;
  finalColor.z *= sparams.colorMult.z;
  sp.startColor = finalColor;
  sp.startColor.w = sp.startColor.w * sparams.colorMult.w;

  flowps2_calculate_color( rp, sp, sparams, 0 );
  flowps2_calculate_size( rp, sp, rparams, sparams, 0 );
  flowps2_calculate_lengthening( rp, sp, sparams );

  if ( sp.life <= 0 )
    return flowps2_pack_null( rp, sp, cdesc, ldata );

  if ( !flowps2_check_alpha( rp, rparams ) )
    rp.radius = 0;

  float4 p4 = float_xyz1( rp.pos );
  float3 wpos = float3(
    dot(p4, rparams.fxTm[0]),
    dot(p4, rparams.fxTm[1]),
    dot(p4, rparams.fxTm[2]));

  pack_ren_data( rp, ldata, cdesc.dataRenOffsetCurrent );
  pack_sim_data( sp, ldata, cdesc.dataSimOffsetCurrent );

  return flowps2_get_culling( rp, wpos, rp.lengthening );
}

DAFX_INLINE
float4 dafx_simulation_shader_cb( ComputeCallDesc_cref cdesc, BufferData_ref ldata, GlobalData_cref gparams )
{
  #ifdef __cplusplus
    const ParentRenData &rparams = *((ParentRenData*)( ldata + cdesc.parentRenOffset ));
    const ParentSimData &sparams = *((ParentSimData*)( ldata + cdesc.parentSimOffset ));
  #else
    ParentRenData rparams;
    ParentSimData sparams;
    unpack_parent_ren_data( rparams, ldata, cdesc.parentRenOffset );
    unpack_parent_sim_data( sparams, ldata, cdesc.parentSimOffset );
  #endif

  RenData rp;
  SimData sp;

  unpack_ren_data( rp, ldata, cdesc.dataRenOffsetCurrent );
  unpack_sim_data( sp, ldata, cdesc.dataSimOffsetCurrent );

  float dt = cdesc.lod * gparams.dt;

  sp.life -= dt;
  if ( sp.life <= 0 )
    return flowps2_pack_null( rp, sp, cdesc, ldata );

  float3 wind_offset = sparams.localWindSpeed * sparams.windScale * dt;
  float vk = max( 1.f - sparams.viscosity * dt, 0.f );
  float wk = max( 1.f - sparams.rotViscosity * dt, 0.f );

  rp.pos += sp.vel * dt;
  rp.pos += wind_offset;

  // there is no space left in particle data to store max_life_inv, so we recalc it every time with seed
  float maxLife = clamp(sparams.life + rp.rnd * sparams.randomLife, 0.f, sparams.life + sparams.randomLife);
  float maxLifeInv = maxLife > 0.0001f ? 1.f / maxLife : 0.f;

  bool isTrail = sparams.emitter.isTrail;
  if ( isTrail )
  {
    sp.vel -= sp.vel * dt * maxLifeInv;
  }
  else
  {
    sp.vel *= vk;
    sp.vel.y -= sparams.gravity * dt;
  }

  sp.angle += sp.wvel * dt;
  rp.angle = sp.angle;
  sp.wvel *= wk;

  float time = 1 - saturate( sp.life * maxLifeInv ); // cant be a little over 1, due to rp.rnd packing

  flowps2_calculate_color( rp, sp, sparams, time );
  flowps2_calculate_size( rp, sp, rparams, sparams, time );
  flowps2_calculate_lengthening( rp, sp, sparams );

  if ( !flowps2_check_alpha( rp, rparams ) )
    rp.radius = 0;

  float4 p4 = float_xyz1( rp.pos );
  float3 wpos = float3(
    dot(p4, rparams.fxTm[0]),
    dot(p4, rparams.fxTm[1]),
    dot(p4, rparams.fxTm[2]));

#if FLOWPS2_GPU_FEATURES
  if ( sp.life > 0 && ( sparams.collisionBounceFactor > 0 || sparams.collisionBounceDecay < 1 ) )
    flowps2_apply_depth_collision( wpos, rp.radius, sp.vel, sparams.collisionBounceFactor, sparams.collisionBounceDecay, gparams );
#endif

  pack_ren_data( rp, ldata, cdesc.dataRenOffsetCurrent );
  pack_sim_data( sp, ldata, cdesc.dataSimOffsetCurrent );

  return flowps2_get_culling( rp, wpos, rp.lengthening );
}
