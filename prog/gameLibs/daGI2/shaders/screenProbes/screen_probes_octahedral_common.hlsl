#ifndef SCREENPROBES_INCLUDED
#define SCREENPROBES_INCLUDED 1

#include <octahedral_common.hlsl>
float2 screenspace_probe_dir_encode(float3 view_dir)
{
  return dagi_dir_oct_encode(view_dir);
}
///tc -1,1
float3 screenspace_probe_dir_decode(float2 tc)
{
  return dagi_dir_oct_decode(tc);
}

#define SP_TAG_HAS_ADDITIONAL 1
#define SP_TAG_MOVING_PROBE   2
#define SP_TAG_MOVING_SHIFT   1
#define SP_TAG_AGE_SHIFT      (SP_TAG_MOVING_SHIFT + 1)
#define SP_TAG_MAX_AGE        3
#define SP_TAG_BORN_HISTORY_AGE    (SP_TAG_MAX_AGE+1)
#define SP_TAG_AGE_MASK     (SP_TAG_MAX_AGE<<SP_TAG_AGE_SHIFT)
#define SP_TAG_AGE_BORN     SP_TAG_AGE_MASK

#define SP_DEPTH_BITS 28
#define SP_DEPTH_BITS_MASK ((1<<SP_DEPTH_BITS)-1)
#define SP_TAG_BITS_SHIFT (SP_DEPTH_BITS)

bool sp_is_relatively_new(uint tag) {return tag>>SP_TAG_AGE_SHIFT;} // age is below SP_TAG_MAX_AGE
bool sp_is_moving_or_new(uint tag) {return (tag>>SP_TAG_MOVING_SHIFT) != 0;} // age is below SP_TAG_MAX_AGE or moving
bool sp_is_newborn_tag(uint tag) {return (tag&SP_TAG_AGE_MASK) == SP_TAG_AGE_BORN;}
bool sp_is_moving_tag(uint tag) {return tag&SP_TAG_MOVING_PROBE;}
bool sp_has_additinal_probes(uint tag) {return tag&SP_TAG_HAS_ADDITIONAL;}

uint sp_encodeTagMoving(uint is_moving) {return is_moving<<SP_TAG_MOVING_SHIFT;}
uint sp_decodeTagRevAge(uint tag) {return tag>>SP_TAG_AGE_SHIFT;} // reversed age
uint sp_encodeTagRevAge(uint age) {return age<<SP_TAG_AGE_SHIFT;} // reversed age

uint sp_decodeEncodedTagRevAge(uint tag) {return tag>>(SP_TAG_BITS_SHIFT + SP_TAG_AGE_SHIFT);} // reversed age

uint sp_encodeAdditionalFinal(uint additional) {return additional<<SP_TAG_BITS_SHIFT;}
uint sp_decodeTag(uint encoded_tag) {return encoded_tag>>SP_TAG_BITS_SHIFT;}
uint sp_encodeTag(uint allTag) {return allTag<<SP_TAG_BITS_SHIFT;}
uint sp_encodeNormalizedW(float w) {return max(1, saturate(w)*SP_DEPTH_BITS_MASK);}
float sp_decodeNormalizedW(uint encoded)  {return (encoded&SP_DEPTH_BITS_MASK)*(1./SP_DEPTH_BITS_MASK);}

struct DecodedProbe
{
  float normalizedW;
  uint allTag;
};
uint sp_encodeProbeInfo(DecodedProbe probe)
{
  return sp_encodeNormalizedW(probe.normalizedW) | sp_encodeTag(probe.allTag);
}
DecodedProbe sp_decodeProbeInfo(uint encoded)
{
  DecodedProbe probe;
  probe.normalizedW = sp_decodeNormalizedW(encoded);
  probe.allTag = sp_decodeTag(encoded);
  return probe;
}

#define SP_NORMAL_BITS 11
#define SP_NORMAL_MASK ((1<<SP_NORMAL_BITS)-1)
#define SP_COORD_OFS_SHIFT (SP_NORMAL_BITS + SP_NORMAL_BITS)
#define SP_COORD_OFS_BITS (32 - SP_COORD_OFS_SHIFT)
#define SP_COORD_OFS_BITS_Y (SP_COORD_OFS_BITS/2)
#define SP_COORD_OFS_BITS_X (SP_COORD_OFS_BITS - SP_COORD_OFS_BITS_Y)
#define SP_COORD_OFS_MASK_X ((1<<SP_COORD_OFS_BITS_X)-1)
#define SP_COORD_OFS_MASK_Y ((1<<SP_COORD_OFS_BITS_Y)-1)

//todo: for screen probes we can use whole 32 bits for normal only
uint encodeProbeNormal(float3 scene_normal)
{
  uint2 encodedOct = uint2(screenspace_probe_dir_encode(scene_normal)*0.5*SP_NORMAL_MASK + 0.5*SP_NORMAL_MASK + 0.5/SP_NORMAL_MASK);
  return encodedOct.x | (encodedOct.y<<SP_NORMAL_BITS);
}
float3 decodeProbeNormal(uint encoded)
{
  return screenspace_probe_dir_decode(float2(uint2(encoded, encoded>>SP_NORMAL_BITS)&SP_NORMAL_MASK)*(2./SP_NORMAL_MASK) - 1);
}
uint encodeCoordOfs(uint2 coord_ofs)
{
  return (coord_ofs.x | (coord_ofs.y<<SP_COORD_OFS_BITS_X))<<SP_COORD_OFS_SHIFT;
}
uint2 decodeCoordOfs(uint encoded)
{
  return uint2((encoded>>SP_COORD_OFS_SHIFT)&SP_COORD_OFS_MASK_X, encoded>>(SP_COORD_OFS_SHIFT + SP_COORD_OFS_BITS_X));
}
uint encodeProbeNormalAndOfs(float3 scene_normal, uint2 coord_ofs)
{
  return encodeProbeNormal(scene_normal) | encodeCoordOfs(coord_ofs);
}

float3 getViewVecFromTc(float2 tc, float3 lt, float3 hor, float3 ver) {return lt + hor*tc.x + ver*tc.y;}

#define UseByteAddressBuffer ByteAddressBuffer
#include "decodeProbes.hlsl"
#undef UseByteAddressBuffer
#define UseByteAddressBuffer RWByteAddressBuffer
#include "decodeProbes.hlsl"
#undef UseByteAddressBuffer

#endif
