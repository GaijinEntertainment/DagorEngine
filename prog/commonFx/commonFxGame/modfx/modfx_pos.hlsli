#ifndef DAFX_MODFX_POS_HLSL
#define DAFX_MODFX_POS_HLSL

DAFX_INLINE
float modfx_pos_radius_rnd( float rad, float volume, float rnd )
{
  float v = 1.f - pow2( rnd ); // better distribution
  return lerp( 1.f, v, volume ) * rad;
}

DAFX_INLINE
ModfxDeclPosInitSphere ModfxDeclPosInitSphere_load( BufferData_cref buf, uint ofs )
{
#ifdef __cplusplus
  return *(ModfxDeclPosInitSphere*)( buf + ofs );
#else
  ModfxDeclPosInitSphere pp;
  pp.volume = dafx_load_1f( buf, ofs );
  pp.offset = dafx_load_3f( buf, ofs );
  pp.radius = dafx_load_1f( buf, ofs );
  return pp;
#endif
}

DAFX_INLINE
ModfxDeclParticleHeightLimit ModfxDeclParticleHeightLimit_load( BufferData_cref buf, uint ofs )
{
#ifdef __cplusplus
  return *(ModfxDeclParticleHeightLimit*)( buf + ofs );
#else
  ModfxDeclParticleHeightLimit pp;
  pp.height_limit = dafx_load_1f( buf, ofs );
  return pp;
#endif
}

DAFX_INLINE
bool modfx_pos_under_height_limit(ModfxParentSimData_cref parent_sdata, BufferData_cref buf, float3_ref o_pos)
{
  uint ofs = parent_sdata.mods_offsets[MODFX_SMOD_HEIGHT_LIMIT];
  if (!ofs)
    return true;

  ModfxDeclParticleHeightLimit pp = ModfxDeclParticleHeightLimit_load(buf, ofs);

  float height_offset = 0;
#if DAFX_USE_HMAP
  height_offset = getWorldHeight(float2(o_pos.x, o_pos.z));
#endif

  return o_pos.y < pp.height_limit + height_offset;
}

DAFX_INLINE
void modfx_pos_init_sphere( BufferData_cref buf, uint ofs, rnd_seed_ref rnd_seed, float3_ref o_p, float3_ref o_v )
{
  ModfxDeclPosInitSphere pp = ModfxDeclPosInitSphere_load( buf, ofs );

  o_v = normalize( dafx_srnd_vec3( rnd_seed ) );
  o_p = o_v * modfx_pos_radius_rnd( pp.radius, pp.volume, dafx_frnd( rnd_seed ) ) + pp.offset;
}

DAFX_INLINE
ModfxDeclPosInitBox ModfxDeclPosInitBox_load( BufferData_cref buf, uint ofs )
{
#ifdef __cplusplus
  return *(ModfxDeclPosInitBox*)( buf + ofs );
#else
  ModfxDeclPosInitBox pp;
  pp.volume = dafx_load_1f( buf, ofs );
  pp.offset = dafx_load_3f( buf, ofs );
  pp.dims = dafx_load_3f( buf, ofs );
  return pp;
#endif
}

DAFX_INLINE
void modfx_pos_init_box( BufferData_cref buf, uint ofs, rnd_seed_ref rnd_seed, float3_ref o_p, float3_ref o_v )
{
  ModfxDeclPosInitBox pp = ModfxDeclPosInitBox_load( buf, ofs );

  o_v = dafx_srnd_vec3( rnd_seed );
  o_p = o_v * pp.dims + pp.offset;
}

DAFX_INLINE
ModfxDeclPosInitCone ModfxDeclPosInitCone_load( BufferData_cref buf, uint ofs )
{
#ifdef __cplusplus
  return *(ModfxDeclPosInitCone*)( buf + ofs );
#else
  ModfxDeclPosInitCone pp;
  pp.volume = dafx_load_1f( buf, ofs );
  pp.offset = dafx_load_3f( buf, ofs );
  pp.vec = dafx_load_3f( buf, ofs );
  pp.h1 = dafx_load_1f( buf, ofs );
  pp.h2 = dafx_load_1f( buf, ofs );
  pp.rad = dafx_load_1f( buf, ofs );
  pp.random_burst = dafx_load_1f( buf, ofs );
  return pp;
#endif
}

DAFX_INLINE
void modfx_pos_init_cone( BufferData_cref buf, uint ofs, rnd_seed_ref rnd_seed, uint dispatch_seed, float3_ref o_p, float3_ref o_v )
{
  ModfxDeclPosInitCone pp = ModfxDeclPosInitCone_load( buf, ofs );

  rnd_seed_t burst_seed = dafx_fastrnd( dispatch_seed );
  float3 rnd_sector = dafx_srnd_vec3( burst_seed );
  float3 yaxis = normalize( lerp( pp.vec, rnd_sector, pp.random_burst ) );

  float3 origin = -yaxis * pp.h2;

  float3 xaxis = fabsf( yaxis.y ) > 0.9 ? float3( 1, 0, 0 ) : float3( 0, 1, 0 );
  float3 zaxis = normalize( cross( xaxis, yaxis ) );
  xaxis = cross( zaxis, yaxis );

  float2 rnd = dafx_srnd_vec2( rnd_seed );
  float3 xo = xaxis * rnd.x;
  float3 zo = zaxis * rnd.y;

  float3 p = normalize( xo + zo ) * modfx_pos_radius_rnd( pp.rad, pp.volume, dafx_frnd( rnd_seed ) );
  o_v = normalize( p - origin );
  o_p = origin + o_v * ( pp.h2 + dafx_frnd( rnd_seed ) * pp.h1 ) / dot( o_v, yaxis ) + pp.offset;

  if ( pp.h1 < 0 )
    o_v = -o_v;
}

DAFX_INLINE
ModfxDeclPosInitCylinder ModfxDeclPosInitCylinder_load( BufferData_cref buf, uint ofs )
{
#ifdef __cplusplus
  return *(ModfxDeclPosInitCylinder*)( buf + ofs );
#else
  ModfxDeclPosInitCylinder pp;
  pp.volume = dafx_load_1f( buf, ofs );
  pp.offset = dafx_load_3f( buf, ofs );
  pp.vec = dafx_load_3f( buf, ofs );
  pp.radius = dafx_load_1f( buf, ofs );
  pp.height = dafx_load_1f( buf, ofs );
  pp.random_burst = dafx_load_1f( buf, ofs );
  return pp;
#endif
}

DAFX_INLINE
void modfx_pos_init_cylinder( BufferData_cref buf, uint ofs, rnd_seed_ref rnd_seed, uint dispatch_seed, float3_ref o_p, float3_ref o_v )
{
  ModfxDeclPosInitCylinder pp = ModfxDeclPosInitCylinder_load( buf, ofs );

  rnd_seed_t burst_seed = dafx_fastrnd( dispatch_seed );
  float3 rnd_sector = dafx_srnd_vec3( burst_seed );
  float3 v = normalize( lerp( pp.vec, rnd_sector, pp.random_burst ) );

  float3 yaxis = fabsf( v.y ) < 0.9 ? float3( 0, 1, 0 ) : float3( 1, 0, 0 );
  float3 zaxis = normalize( cross( yaxis, v ) );
  float3 xaxis = normalize( cross( v, zaxis ) );

  o_v = normalize( dafx_srnd( rnd_seed ) * xaxis + dafx_srnd( rnd_seed ) * zaxis );
  o_p = o_v * modfx_pos_radius_rnd( pp.radius, pp.volume, dafx_frnd( rnd_seed ) ) + v * dafx_frnd( rnd_seed ) * pp.height + pp.offset;
}

DAFX_INLINE
ModfxDeclPosInitSphereSector ModfxDeclPosInitSphereSector_load( BufferData_cref buf, uint ofs )
{
#ifdef __cplusplus
  return *(ModfxDeclPosInitSphereSector*)( buf + ofs );
#else
  ModfxDeclPosInitSphereSector pp;
  pp.vec = dafx_load_3f( buf, ofs );
  pp.radius = dafx_load_1f( buf, ofs );
  pp.sector = dafx_load_1f( buf, ofs );
  pp.volume = dafx_load_1f( buf, ofs );
  pp.random_burst = dafx_load_1f( buf, ofs );
  return pp;
#endif
}

DAFX_INLINE
ModfxDeclPosInitGpuPlacement ModfxDeclPosInitGpuPlacement_load( BufferData_cref buf, uint ofs )
{
#ifdef __cplusplus
  return *(ModfxDeclPosInitGpuPlacement*)( buf + ofs );
#else
  ModfxDeclPosInitGpuPlacement pp;
  pp.flags = dafx_load_1ui(buf, ofs);
  pp.height_threshold = dafx_load_1f(buf, ofs);
  return pp;
#endif
}

DAFX_INLINE
void modfx_pos_init_sphere_sector( BufferData_cref buf, uint ofs, rnd_seed_ref rnd_seed, uint dispatch_seed,
                                   float3_ref o_p, float3_ref o_v )
{
  ModfxDeclPosInitSphereSector pp = ModfxDeclPosInitSphereSector_load( buf, ofs );

  // TODO: better distribution
  float xz_angle = dafx_frnd( rnd_seed ) * dafx_2pi;
  float y_angle_rnd = dafx_frnd( rnd_seed );
  float y_angle = lerp( pp.sector - 0.5f, 0.5f, y_angle_rnd ) * dafx_pi;

  float s0, c0;
  sincos( xz_angle, s0, c0 );

  float s1, c1;
  sincos( y_angle, s1, c1 );

  float3 tv = float3( c0 * c1, s1, s0 * c1 );

  rnd_seed_t burst_seed = dafx_fastrnd( dispatch_seed );
  float3 rnd_sector = dafx_srnd_vec3( burst_seed );
  float3 yaxis = normalize( lerp( pp.vec, rnd_sector, pp.random_burst ) );

  float3 up_vec = fabsf(yaxis.y) > 0.8 ? float3( 1, 0, 0 ) : float3( 0, 1, 0 );
  float3 zaxis = normalize( cross( yaxis, up_vec ) );
  float3 xaxis = normalize( cross( yaxis, zaxis ) );

  o_v = tv.x * xaxis + tv.y * yaxis + tv.z * zaxis;


  float r = lerp( 1.f, dafx_frnd( rnd_seed ), pp.volume ) * pp.radius;
  o_p = o_v * r;
}

#if MODFX_GPU_FEATURES_ENABLED
bool modfx_pos_gpu_placement(ModfxParentSimData_cref parent_sdata, BufferData_cref buf, float emitter_pos_y, float3_ref o_pos, GlobalData_cref gdata)
{
  uint ofs = parent_sdata.mods_offsets[MODFX_SMOD_POS_INIT_GPU_PLACEMENT];
  if (!ofs)
    return true;

  ModfxDeclPosInitGpuPlacement pp = ModfxDeclPosInitGpuPlacement_load(buf, ofs);

  float vignette = 1.f;
  float r = gdata.water_level;
#if DAFX_USE_HMAP
  if (pp.flags & MODFX_GPU_PLACEMENT_HMAP)
    r = getWorldHeight(float2(o_pos.x, o_pos.z));
#endif

#if DAFX_USE_DEPTH_ABOVE
  if (pp.flags & MODFX_GPU_PLACEMENT_DEPTH_ABOVE)
  {
    float h = get_depth_above_fast(o_pos, vignette);
    r = lerp(r, h, 1.f - vignette);
  }
#endif

  if (pp.flags & MODFX_GPU_PLACEMENT_WATER)
    r = max(r, gdata.water_level);

  float y_move = r - emitter_pos_y; // move emitter exact to the ground level
  o_pos.y += y_move;
  return (pp.height_threshold > 0) ? (fabsf(y_move) < pp.height_threshold) : true;
}

void modfx_pos_add_water_flowmap(float3_ref o_pos, float3 pos_offset, float dt)
{
#if DAFX_USE_WATER_FLOWMAP
  o_pos += getWaterFlowmapVec(o_pos + pos_offset) * dt;
#endif
}
#endif

DAFX_INLINE
void modfx_pos_init( ModfxParentSimData_cref parent_sdata, BufferData_cref buf, rnd_seed_ref rnd_seed, uint dispatch_seed, float3_ref o_p, float3_ref o_v )
{
  if ( parent_sdata.mods_offsets[MODFX_SMOD_POS_INIT_SPHERE] )
    modfx_pos_init_sphere( buf, parent_sdata.mods_offsets[MODFX_SMOD_POS_INIT_SPHERE], rnd_seed, o_p, o_v );
  else if ( parent_sdata.mods_offsets[MODFX_SMOD_POS_INIT_BOX] )
    modfx_pos_init_box( buf, parent_sdata.mods_offsets[MODFX_SMOD_POS_INIT_BOX], rnd_seed, o_p, o_v );
  else if ( parent_sdata.mods_offsets[MODFX_SMOD_POS_INIT_CONE] )
    modfx_pos_init_cone( buf, parent_sdata.mods_offsets[MODFX_SMOD_POS_INIT_CONE], rnd_seed, dispatch_seed, o_p, o_v );
  else if ( parent_sdata.mods_offsets[MODFX_SMOD_POS_INIT_CYLINDER] )
    modfx_pos_init_cylinder( buf, parent_sdata.mods_offsets[MODFX_SMOD_POS_INIT_CYLINDER], rnd_seed, dispatch_seed, o_p, o_v );
  else if ( parent_sdata.mods_offsets[MODFX_SMOD_POS_INIT_SPHERE_SECTOR] )
    modfx_pos_init_sphere_sector( buf, parent_sdata.mods_offsets[MODFX_SMOD_POS_INIT_SPHERE_SECTOR], rnd_seed, dispatch_seed, o_p, o_v );
}

#endif
