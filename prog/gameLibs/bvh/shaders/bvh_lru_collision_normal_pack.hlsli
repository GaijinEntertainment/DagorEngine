#ifndef BVH_LRU_COLLISION_NORMAL_PACK_HLSLI
#define BVH_LRU_COLLISION_NORMAL_PACK_HLSLI

#include <octahedral.hlsl>

// the pool's storage protocol: one octahedral normal as 2x12 unorm in the low
// bits of one uint (max error ~0.03 deg, far under what the GI axis weights
// resolve); bits 24..31 are reserved for a per-face palette index, the fill
// writes them as 0. The fill shader packs and the consumer macro unpacks;
// change both sides only through this pair.

uint bvh_lru_collision_pack_normal(float3 unit_normal)
{
  uint2 quantized = uint2(round(saturate(octEncode(unit_normal) * 0.5 + 0.5) * 4095.0));
  return quantized.x | (quantized.y << 12);
}

float3 bvh_lru_collision_unpack_normal(uint packd) // note: packed is a keyword in PSSL
{
  return octDecode(float2(packd & 0xFFF, (packd >> 12) & 0xFFF) * (2.0 / 4095.0) - 1.0);
}

#endif
