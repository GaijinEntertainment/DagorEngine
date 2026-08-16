#ifndef OMM_TEXCOORD_DECODE_HLSLI
#define OMM_TEXCOORD_DECODE_HLSLI

#include "omm_texcoord_formats.hlsli"

float2 omm_decode_texcoord(ByteAddressBuffer texcoords, uint byte_offset, uint format)
{
  if (format == OMM_TC_UV32_FLOAT)
    return asfloat(texcoords.Load2(byte_offset));

  uint packd = texcoords.Load(byte_offset);
  if (format == OMM_TC_UV16_FLOAT)
    return f16tof32(uint2(packd & 0xFFFFu, packd >> 16));
  if (format == OMM_TC_UV16_UNORM)
    return float2(packd & 0xFFFFu, packd >> 16) / float(0xFFFF);

  // OMM_TC_SHORT2_FIXED4096. Unreachable for any other value: the C++ side asserts on an unmapped one.
  int2 raw = int2(asint(packd << 16), asint(packd & 0xFFFF0000u)) >> 16; // sign-extend both halves
  return float2(raw) / 4096.0;
}

#endif // OMM_TEXCOORD_DECODE_HLSLI
