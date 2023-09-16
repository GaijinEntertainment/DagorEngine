#ifndef DAFX_MODFX_RADIUS_HLSL
#define DAFX_MODFX_RADIUS_HLSL

DAFX_INLINE
ModfxDeclRadiusInitRnd ModfxDeclRadiusInitRnd_load( BufferData_cref buf, uint ofs )
{
#ifdef __cplusplus
  return *(ModfxDeclRadiusInitRnd*)( buf + ofs );
#else
  ModfxDeclRadiusInitRnd pp;
  pp.rad_min = dafx_load_1f( buf, ofs );
  pp.rad_max = dafx_load_1f( buf, ofs );
  return pp;
#endif
}

DAFX_INLINE
void modfx_radius_init_rnd( BufferData_cref buf, uint ofs, rnd_seed_ref rnd_seed, float_ref o_rad )
{
  ModfxDeclRadiusInitRnd pp = ModfxDeclRadiusInitRnd_load( buf, ofs );

  o_rad = lerp( pp.rad_min, pp.rad_max, dafx_frnd( rnd_seed ) );
}

DAFX_INLINE
void modfx_radius_init( ModfxParentSimData_cref parent_sdata, BufferData_cref buf, rnd_seed_ref rnd_seed, float_ref o_rad )
{
  if ( parent_sdata.mods_offsets[MODFX_SMOD_RADIUS_INIT] )
    o_rad = dafx_get_1f( buf, parent_sdata.mods_offsets[MODFX_SMOD_RADIUS_INIT] );
  else if ( parent_sdata.mods_offsets[MODFX_SMOD_RADIUS_INIT_RND] )
    modfx_radius_init_rnd( buf, parent_sdata.mods_offsets[MODFX_SMOD_RADIUS_INIT_RND], rnd_seed, o_rad );
}

DAFX_INLINE
void modfx_radius_sim( ModfxParentSimData_cref parent_sdata, BufferData_cref buf, rnd_seed_ref rnd_seed, float life_k, float_ref o_rad )
{
  modfx_radius_init( parent_sdata, buf, rnd_seed, o_rad );

  if ( parent_sdata.mods_offsets[MODFX_SMOD_RADIUS_OVER_PART_LIFE] )
    o_rad *= modfx_get_1f_curve( buf, parent_sdata.mods_offsets[MODFX_SMOD_RADIUS_OVER_PART_LIFE], life_k );
}

#endif