#ifndef DAGDP_RIEX_HLSLI_INCLUDED
#define DAGDP_RIEX_HLSLI_INCLUDED

#define RIEX_PATCH_FLAG_BIT_EXTENDED_ARGS (1 << 0)
#define RIEX_PATCH_FLAG_BIT_PACKED        (1 << 1)

struct RiexPatch
{
  uint argsByteOffsetStatic;
  uint argsByteOffsetDynamic;
  uint localCounterIndex; // "Local" to its kind.
  uint indexCount;
  uint materialOffset;
  uint flags;
  uint _padding[2];
};

#endif // DAGDP_RIEX_HLSLI_INCLUDED