#ifndef WORLDF_SDF_DIST_INCLUDED
#define WORLDF_SDF_DIST_INCLUDED 1

uint encode_world_sdf(float sdf)
{
  uint v = (asuint(sdf)&0x7FFFFFFF)<<1;
  return sdf < 0 ? (v|1) : v;
}

float decode_world_sdf(uint v)
{
  float sdf = asfloat(v>>1);
  return (v&1) ? -sdf : sdf;
}

bool encoded_world_sdf_negative(uint v) {return v&1;}

#endif