#ifndef DAFX_MODFX_PART_DATA_HLSL
#define DAFX_MODFX_PART_DATA_HLSL

DAFX_INLINE
void modfx_load_sim_data(BufferData_cref buf, uint ofs, uint decls, DAFX_OREF(ModfxSimData) o)
{
#ifdef __cplusplus
  memset(&o, 0, sizeof(ModfxSimData));
#else
  o = (ModfxSimData)0;
#endif
  o.em_color = float4( 1.f, 1.f, 1.f, 1.f );

  // life_norm is always loaded, but can be reset to 0 if not used (extreme edge case)
  o.life_norm = dafx_load_1f(buf, ofs);
  if (!MODFX_SDECL_LIFE_ENABLED(decls))
    o.life_norm = 0;

  if (MODFX_SDECL_COLLISION_TIME_ENABLED(decls))
    o.collision_time_norm = dafx_load_1f(buf, ofs);

  if (MODFX_SDECL_VELOCITY_ENABLED(decls))
    o.velocity = dafx_load_3f(buf, ofs);

  if (MODFX_SDECL_SIM_FLAGS_ENABLED(decls))
    o.flags = dafx_load_1ui(buf, ofs);

  if (MODFX_SDECL_RND_SEED_ENABLED(decls))
    o.rnd_seed = dafx_load_1ui(buf, ofs);

  if (MODFX_SDECL_EMISSION_COLOR_ENABLED(decls))
    o.em_color = unpack_uint_to_n4f(dafx_load_1ui(buf, ofs));
}

DAFX_INLINE
void modfx_save_sim_data(BufferData_ref buf, uint ofs, uint decls, bool is_emission, DAFX_CREF(ModfxSimData) v)
{
  // life_norm is always stored
  dafx_store_1f(v.life_norm, buf, ofs);

  if (MODFX_SDECL_COLLISION_TIME_ENABLED(decls))
    dafx_store_1f(v.collision_time_norm, buf, ofs);

  if (MODFX_SDECL_VELOCITY_ENABLED(decls))
    dafx_store_3f(v.velocity, buf, ofs );

  if (MODFX_SDECL_SIM_FLAGS_ENABLED(decls))
    dafx_store_1ui(v.flags, buf, ofs);

  // emission only after this line
  if (is_emission && MODFX_SDECL_RND_SEED_ENABLED(decls))
    dafx_store_1ui(v.rnd_seed, buf, ofs );

  if (is_emission && MODFX_SDECL_EMISSION_COLOR_ENABLED(decls))
    dafx_store_1ui(pack_n4f_to_uint(v.em_color), buf, ofs);
}

DAFX_INLINE
void modfx_load_ren_data(BufferData_cref buf, uint ofs, uint decls, DAFX_OREF(ModfxRenData) o)
{
#ifdef __cplusplus
  memset(&o, 0, sizeof(ModfxRenData));
#else
  o = (ModfxRenData)0;
#endif
  o.radius = 1.f;
  o.color = float4(1, 1, 1, 1);
  o.emission_fade = 1.f;

  if (MODFX_RDECL_POS_ENABLED(decls))
    o.pos = dafx_load_3f(buf, ofs);

  if (MODFX_RDECL_POS_OFFSET_ENABLED(decls))
  {
    o.pos_offset = dafx_load_3f(buf, ofs);
    // This "useless" operation fixes problems on Nvidia 10 series due to broken codegen.
    o.pos_offset *= 1.0000001;
  }

  if (MODFX_RDECL_RADIUS_ENABLED(decls))
    o.radius = dafx_load_1f(buf, ofs);

  if (MODFX_RDECL_COLOR_ENABLED(decls))
    o.color = pow2(unpack_uint_to_n4f(dafx_load_1ui(buf, ofs))); // gamma correction applied

  if (MODFX_RDECL_ANGLE_ENABLED(decls))
    o.angle = dafx_load_1f(buf, ofs);

  if (MODFX_RDECL_UP_VEC_ENABLED(decls))
    o.up_vec = normalize_safe(unpack_uint_to_n3s(dafx_load_1ui(buf, ofs)), float3(0, 1, 0));

  if (MODFX_RDECL_RIGHT_VEC_ENABLED(decls))
    o.right_vec = normalize_safe(unpack_uint_to_n3s(dafx_load_1ui(buf, ofs)), float3(1, 0, 0));

  if (MODFX_RDECL_ORIENTATION_VEC_ENABLED(decls))
    o.orientation_vec = normalize_safe(unpack_uint_to_n3s(dafx_load_1ui(buf, ofs)), float3(0, 1, 0));

  if (MODFX_RDECL_VELOCITY_LENGTH_ENABLED(decls))
    o.velocity_length = dafx_load_1f(buf, ofs);

  if (MODFX_RDECL_UNIQUE_ID_ENABLED(decls))
    o.unique_id = dafx_load_1ui(buf, ofs);

  // 1 byte packing after this line
  uint packed_bits = countbits(decls & MODFX_RDECL_PACKED_MASK);
  uint4 packed_data[2];

  if (packed_bits > 0)
    packed_data[0] = unpack_4b(dafx_load_1ui(buf, ofs));
  if (packed_bits > 4)
    packed_data[1] = unpack_4b(dafx_load_1ui(buf, ofs));

  uint pofs = 0;
  if (MODFX_RDECL_FRAME_IDX_ENABLED(decls))
    o.frame_idx = packed_data[0][pofs++];

  if (MODFX_RDECL_FRAME_FLAGS_ENABLED(decls))
    o.frame_flags = packed_data[0][pofs++];

  if (MODFX_RDECL_FRAME_BLEND_ENABLED(decls))
    o.frame_blend = packed_data[0][pofs++] * (1.f / 255.f);

  if (MODFX_RDECL_LIFE_NORM_ENABLED(decls))
    o.life_norm = packed_data[0][pofs++] * (1.f / 255.f);

  // can dip to second packed_data
  if (MODFX_RDECL_EMISSION_FADE_ENABLED(decls) )
    o.emission_fade = packed_data[pofs / 4][pofs % 4] * (1.f / 255.f);
}

DAFX_INLINE
void modfx_save_ren_data(BufferData_ref buf, uint ofs, uint decls, uint sflags, bool is_emission, DAFX_CREF(ModfxRenData) v)
{
  if (MODFX_RDECL_POS_ENABLED(decls))
    dafx_store_3f(v.pos, buf, ofs);

  if (MODFX_RDECL_POS_OFFSET_ENABLED(decls))
    dafx_store_3f(v.pos_offset, buf, ofs);

  if (MODFX_RDECL_RADIUS_ENABLED(decls))
    dafx_store_1f(v.radius, buf, ofs);

  if (MODFX_RDECL_COLOR_ENABLED(decls))
    dafx_store_1ui(pack_n4f_to_uint(sqrt(saturate(v.color))), buf, ofs); // gamma correction applied

  if (MODFX_RDECL_ANGLE_ENABLED(decls))
    dafx_store_1f(v.angle, buf, ofs);

  if (MODFX_RDECL_UP_VEC_ENABLED(decls))
  {
    if (is_emission || !FLAG_ENABLED(sflags, MODFX_SFLAG_PASS_UP_VEC_TO_RDECL_ONCE))
      dafx_set_1ui(pack_n3s_to_uint(v.up_vec), buf, ofs);
    ofs++;
  }

  if (MODFX_RDECL_RIGHT_VEC_ENABLED(decls))
  {
    if (is_emission)
      dafx_set_1ui(pack_n3s_to_uint(v.right_vec ), buf, ofs);
    ofs++;
  }

  if (MODFX_RDECL_ORIENTATION_VEC_ENABLED(decls))
  {
    if (is_emission)
      dafx_set_1ui(pack_n3s_to_uint(v.orientation_vec), buf, ofs);
    ofs++;
  }

  if (MODFX_RDECL_VELOCITY_LENGTH_ENABLED(decls))
    dafx_store_1f(v.velocity_length, buf, ofs);

  if (MODFX_RDECL_UNIQUE_ID_ENABLED(decls))
  {
    if (is_emission)
      dafx_set_1ui(v.unique_id, buf, ofs);
    ofs++;
  }
  // 1 byte packing after this line
  // (cant write to uint4 as array directly, see: HLSL error: X3500)
  // (can use arrays to storing temp data, due to Spirv error)
  uint packed_bits = countbits(decls & MODFX_RDECL_PACKED_MASK);
  uint packed_data[2];

  packed_data[0] = 0;
  packed_data[1] = 0;

  uint pofs = 0;
  if (MODFX_RDECL_FRAME_IDX_ENABLED(decls))
    packed_data[0] |= (v.frame_idx & 0xff) << (8 * pofs++);

  if (MODFX_RDECL_FRAME_FLAGS_ENABLED(decls))
    packed_data[0] |= (v.frame_flags & 0xff) << (8 * pofs++);

  if (MODFX_RDECL_FRAME_BLEND_ENABLED(decls))
    packed_data[0] |= (uint(v.frame_blend * 255.f) & 0xff) << (8 * pofs++);

  if (MODFX_RDECL_LIFE_NORM_ENABLED(decls))
    packed_data[0] |= (uint(v.life_norm * 255.f) & 0xff) << (8 * pofs++);

  // can dip to second packed_data
  if ( MODFX_RDECL_EMISSION_FADE_ENABLED(decls))
    packed_data[pofs / 4] |= (uint(v.emission_fade * 255.f) & 0xff) << (8 * (pofs % 4));

  if (packed_bits > 0)
    dafx_store_1ui(packed_data[0], buf, ofs);

  if (packed_bits > 4)
    dafx_store_1ui(packed_data[1], buf, ofs);
}

#endif