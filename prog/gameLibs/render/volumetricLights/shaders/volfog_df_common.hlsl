#ifndef VOLFOG_DF_COMMON_INCLUDED
#define VOLFOG_DF_COMMON_INCLUDED 1


#define DEBUG_DISTANT_FOG_BOTH_SIDES_RAYMARCH 1
#define DEBUG_DISTANT_FOG_SHOW_RAYMARCH_END 0

#define DISTANT_FOG_DEPTH_SAMPLE_LOD 0



uint calc_raymarch_noise_index(uint2 dtId, uint frame_id)
{
  uint hashId = (dtId.x+dtId.y*1920*2)*8;
  uint z_sample = pcg_hash(hashId);
  return z_sample + frame_id;
}

float calc_raymarch_noise_offset_8(uint noise_index)
{
  return POISSON_Z_SAMPLES[noise_index % SAMPLE_NUM];
}

 // TODO: refactor and optimize it (in NBS too!!)
float calc_raymarch_noise_offset_32_impl(uint noise_index, Texture2D<float4> poisson_samples)
{
  return texelFetch(poisson_samples, uint2(noise_index % 32, 0), 0).x;
}
#define calc_raymarch_noise_offset_32(noise_index) calc_raymarch_noise_offset_32_impl(noise_index, volfog_poisson_samples)


uint2 calc_raymarch_offset(uint2 raymarch_id, uint frame_id)
{
  uint checkerOffset = (raymarch_id.x^raymarch_id.y)&1;
  uint id = (frame_id + checkerOffset) & 3;
  return (id == 2) || (id == uint2(1, 3));
  // same as below, but faster:
  // if (id == 0)
  //   return uint2(0,0);
  // if (id == 1)
  //   return uint2(1,0);
  // if (id == 2)
  //   return uint2(1,1);
  // return uint2(0,1);
}

uint2 calc_reconstruction_id(uint2 raymarch_id, uint frame_id)
{
#if DEBUG_DISTANT_FOG_DISABLE_4_WAY_RECONSTRUCTION
  return raymarch_id;
#else
  return raymarch_id * 2 + calc_raymarch_offset(raymarch_id, frame_id);
#endif
}

float encodeRaymarchingOffset(uint2 raymarch_index_offset)
{
  return (raymarch_index_offset.y * 2 + raymarch_index_offset.x) / 3.0;
}
uint2 decodeRaymarchingOffset(float raymarch_index_offset_enc)
{
  return ((uint2)(raymarch_index_offset_enc * 3.0) & uint2(1, 2)) != 0;
}

#endif