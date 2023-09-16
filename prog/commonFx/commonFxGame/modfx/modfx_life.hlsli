#ifndef DAFX_MODFX_LIFE_HLSL
#define DAFX_MODFX_LIFE_HLSL

void modfx_life_init( ModfxParentSimData_cref parent_sdata, BufferData_cref buf, rnd_seed_ref rnd_seed, float_ref o_life )
{
  if ( parent_sdata.mods_offsets[MODFX_SMOD_LIFE_OFFSET] )
    o_life = dafx_get_1f( buf, parent_sdata.mods_offsets[MODFX_SMOD_LIFE_OFFSET] ) * dafx_frnd( rnd_seed );
}

void modfx_life_sim( ModfxParentSimData_cref parent_sdata, BufferData_cref buf, rnd_seed_ref rnd_seed, float life_limit_rcp, float dt, float_ref o_life_norm )
{
  if ( parent_sdata.mods_offsets[MODFX_SMOD_LIFE_RND] )
  {
    float ratio = dafx_get_1f( buf, parent_sdata.mods_offsets[MODFX_SMOD_LIFE_RND] );
    life_limit_rcp *= lerp(1.f, ratio, dafx_frnd(rnd_seed));
  }

  o_life_norm += dt * life_limit_rcp;
}

#endif