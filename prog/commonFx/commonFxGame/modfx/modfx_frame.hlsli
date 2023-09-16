#ifndef DAFX_MODFX_FRAME_HLSL
#define DAFX_MODFX_FRAME_HLSL

DAFX_INLINE
ModfxDeclFrameInit ModfxDeclFrameInit_load( BufferData_cref buf, uint ofs )
{
#ifdef __cplusplus
  return *(ModfxDeclFrameInit*)( buf + ofs );
#else
  ModfxDeclFrameInit finit;
  finit.start_frame_min = dafx_load_1ui( buf, ofs );
  finit.start_frame_max = dafx_load_1ui( buf, ofs );
  finit.flags = dafx_load_1ui( buf, ofs );
  return finit;
#endif
}

DAFX_INLINE
void modfx_frame_init( ModfxParentSimData_cref parent_sdata, BufferData_cref buf, rnd_seed_ref rnd_seed, float_ref o_frame, uint_ref o_flags, bool_ref o_disable_loop )
{
  uint ofs = parent_sdata.mods_offsets[MODFX_SMOD_FRAME_INIT];
  ModfxDeclFrameInit finit = ModfxDeclFrameInit_load( buf, ofs );

  o_frame = lerp( (float)finit.start_frame_min, (float)finit.start_frame_max, dafx_frnd( rnd_seed ) );
  o_frame = round( o_frame );
  o_flags = 0;
  o_disable_loop = finit.flags & MODFX_FRAME_FLAGS_DISABLE_LOOP;

  if ( finit.flags & MODFX_FRAME_FLAGS_RANDOM_FLIP_X )
    o_flags |= dafx_frnd( rnd_seed ) > 0.5 ? MODFX_FRAME_FLAGS_RANDOM_FLIP_X : 0;

  if ( finit.flags & MODFX_FRAME_FLAGS_RANDOM_FLIP_Y )
    o_flags |= dafx_frnd( rnd_seed ) > 0.5 ? MODFX_FRAME_FLAGS_RANDOM_FLIP_Y : 0;
}

DAFX_INLINE
ModfxDeclFrameAnimInit ModfxDeclFrameAnimInit_load( BufferData_cref buf, uint ofs )
{
#ifdef __cplusplus
  return *(ModfxDeclFrameAnimInit*)( buf + ofs );
#else
  ModfxDeclFrameAnimInit anim;
  anim.speed_min = dafx_load_1f( buf, ofs );
  anim.speed_max = dafx_load_1f( buf, ofs );
  return anim;
#endif
}

DAFX_INLINE
void modfx_frame_sim(
  ModfxParentRenData_cref parent_rdata, ModfxParentSimData_cref parent_sdata, BufferData_cref buf,
  rnd_seed_ref rnd_seed, float life_k,
  uint_ref o_frame_idx, uint_ref o_frame_flags, float_ref o_frame_blend )
{
  if ( !parent_sdata.mods_offsets[MODFX_SMOD_FRAME_INIT] )
    return;

  float frame;
  uint flags;
  bool disable_loop;

  modfx_frame_init( parent_sdata, buf, rnd_seed, frame, flags, disable_loop );

  uint ofs = parent_rdata.mods_offsets[MODFX_RMOD_FRAME_INFO];
  ModfxDeclFrameInfo finfo = ModfxDeclFrameInfo_load( buf, ofs );

  uint total_frames = finfo.frames_x * finfo.frames_y;

  ofs = parent_sdata.mods_offsets[MODFX_SMOD_FRAME_ANIM_INIT];
  if ( ofs )
  {
    ModfxDeclFrameAnimInit anim = ModfxDeclFrameAnimInit_load( buf, ofs );

    float speed = lerp( anim.speed_min, anim.speed_max, dafx_frnd( rnd_seed ) );
    float t = life_k;
    ofs = parent_sdata.mods_offsets[MODFX_SMOD_FRAME_ANIM_OVER_PART_LIFE];
    if ( ofs )
      t = modfx_get_1f_curve( buf, ofs, life_k );

    float f = t * speed * total_frames;
    frame = frame + f;
    if ( disable_loop )
      frame = min( frame, (float)total_frames - 1 );
  }

  o_frame_idx = ((uint)frame) % total_frames;
  o_frame_flags = flags;
  o_frame_blend = frame - (uint)frame;
}

#endif
