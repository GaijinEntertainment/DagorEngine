#ifndef SCREENPROBES_INCLUDED
#define SCREENPROBES_INCLUDED 1

#include <screen_probes_encoding.hlsl>
#define SP_USE_HEMISPHERE_TRACE 1

#define UseByteAddressBuffer ByteAddressBuffer
#include "decodeProbes.hlsl"
#undef UseByteAddressBuffer
#define UseByteAddressBuffer RWByteAddressBuffer
#include "decodeProbes.hlsl"
#undef UseByteAddressBuffer

#endif
