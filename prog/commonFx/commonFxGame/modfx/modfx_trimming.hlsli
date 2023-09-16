#ifndef DAFX_MODFX_TRIMMING_HLSL
#define DAFX_MODFX_TRIMMING_HLSL

DAFX_INLINE
ModfxDeclPartTrimming ModfxDeclPartTrimming_load( BufferData_cref buf, uint ofs )
{
#ifdef __cplusplus
  return *(ModfxDeclPartTrimming*)( buf + ofs );
#else
  ModfxDeclPartTrimming pp;
  pp.steps = dafx_load_1i( buf, ofs );
  pp.fade_mul = dafx_load_1f( buf, ofs );
  pp.fade_pow = dafx_load_1f( buf, ofs );
  return pp;
#endif
}

DAFX_INLINE
void modfx_part_trimming(
  ModfxParentRenData_cref parent_rdata, ModfxParentSimData_cref parent_sdata, BufferData_cref buf, uint gid, float life_k, DAFX_REF(ModfxRenData) rdata )
{
  uint ofs = parent_sdata.mods_offsets[MODFX_SMOD_PART_TRIMMING];

  ModfxDeclPartTrimming pp = ModfxDeclPartTrimming_load( buf, ofs );

  uint steps = pp.steps > 0 ? pp.steps : -pp.steps;
  float cur_step = steps * ( pp.steps > 0 ? life_k : 1.f - life_k );

  uint step_idx = min( (uint)cur_step, steps - 1 );
  float step_lerp = saturate( cur_step - step_idx );

  uint cur_phase = gid % steps;
  if ( cur_phase > step_idx )
    rdata.radius = 0;
  else if ( cur_phase > 0 && cur_phase == step_idx ) // skip fade for the first batch
  {
    step_lerp = saturate( pow( step_lerp * pp.fade_mul, pp.fade_pow ) );
    rdata.color.w *= step_lerp;
    if ( FLAG_ENABLED( parent_rdata.flags, MODFX_RFLAG_BLEND_PREMULT ) )
    {
      rdata.color.x *= step_lerp;
      rdata.color.y *= step_lerp;
      rdata.color.z *= step_lerp;
    }
  }
}

#endif