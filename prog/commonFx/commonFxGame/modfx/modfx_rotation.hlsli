#ifndef DAFX_MODFX_ROTATION_HLSL
#define DAFX_MODFX_ROTATION_HLSL

DAFX_INLINE
ModfxDeclRotationInitRnd ModfxDeclRotationInitRnd_load( BufferData_cref buf, uint ofs )
{
#ifdef __cplusplus
  return *(ModfxDeclRotationInitRnd*)( buf + ofs );
#else
  ModfxDeclRotationInitRnd pp;
  pp.angle_min = dafx_load_1f( buf, ofs );
  pp.angle_max = dafx_load_1f( buf, ofs );
  return pp;
#endif
}

DAFX_INLINE
ModfxDeclRotationDynamic ModfxDeclRotationDynamic_load( BufferData_cref buf, uint ofs )
{
#ifdef __cplusplus
  return *(ModfxDeclRotationDynamic*)( buf + ofs );
#else
  ModfxDeclRotationDynamic pp;
  pp.vel_min = dafx_load_1f( buf, ofs );
  pp.vel_max = dafx_load_1f( buf, ofs );
  return pp;
#endif
}

DAFX_INLINE
void modfx_rotation_init_rnd( BufferData_cref buf, uint ofs, rnd_seed_ref rnd_seed, float_ref o_angle )
{
  ModfxDeclRotationInitRnd pp = ModfxDeclRotationInitRnd_load( buf, ofs );

  o_angle = lerp( pp.angle_min, pp.angle_max, dafx_frnd( rnd_seed ) );
}

DAFX_INLINE
void modfx_rotation_init( ModfxParentSimData_cref parent_sdata, BufferData_cref buf, rnd_seed_ref rnd_seed, float_ref o_angle )
{
  if ( parent_sdata.mods_offsets[MODFX_SMOD_ROTATION_INIT] )
    o_angle = dafx_get_1f( buf, parent_sdata.mods_offsets[MODFX_SMOD_ROTATION_INIT] );
  else if ( parent_sdata.mods_offsets[MODFX_SMOD_ROTATION_INIT_RND] )
    modfx_rotation_init_rnd( buf, parent_sdata.mods_offsets[MODFX_SMOD_ROTATION_INIT_RND], rnd_seed, o_angle );
}

DAFX_INLINE
void modfx_rotation_dynamic( BufferData_cref buf, uint ofs, rnd_seed_ref rnd_seed, float_ref o_speed )
{
  ModfxDeclRotationDynamic pp = ModfxDeclRotationDynamic_load( buf, ofs );

  o_speed = lerp( pp.vel_min, pp.vel_max, dafx_frnd( rnd_seed ) );
}

DAFX_INLINE
void modfx_rotation_sim( ModfxParentSimData_cref parent_sdata, BufferData_cref buf, rnd_seed_ref rnd_seed, float life_k, float_ref o_angle )
{
  modfx_rotation_init( parent_sdata, buf, rnd_seed, o_angle );

  float speed = 0.f;
  float speed_k = life_k;

  if ( parent_sdata.mods_offsets[MODFX_SMOD_ROTATION_DYNAMIC] )
    modfx_rotation_dynamic( buf, parent_sdata.mods_offsets[MODFX_SMOD_ROTATION_DYNAMIC], rnd_seed, speed );

  if ( parent_sdata.mods_offsets[MODFX_SMOD_ROTATION_OVER_PART_LIFE] )
    speed_k = modfx_get_1f_curve( buf, parent_sdata.mods_offsets[MODFX_SMOD_ROTATION_OVER_PART_LIFE], life_k );

  o_angle += speed * speed_k;
}


#endif
