#ifndef SWRT_SHADOW_SAMPLE_HLSL
#define SWRT_SHADOW_SAMPLE_HLSL 1

#include <pcg_hash.hlsl>
#include <swrt_quarter_upsample.hlsl>

// the shadow of a half-res pixel; w: its linear depth; src_max: the last valid half-res pixel, 0 with no target;
// frame: swrt_checkerboard_frame, 0 for the full half-res target; quarter: swrt_shadow_quarter
float swrt_shadow_sample(Texture2D<float4> target, uint2 coord, float w, int2 src_max, uint frame, bool quarter)
{
  if (!src_max.x)
    return 1;
  if (quarter)
    return swrt_quarter_upsample(target, coord, w, uint2(src_max), frame);
  if (frame)
  {
    uint rand = pcg3d_hash_16bit(uint3(coord, frame)).x;
    int randOfs = ((int)(rand&2)) - 1;
    int2 ofs = select(bool2(rand&1, 1-(rand&1)), int2(randOfs, randOfs), int2(0,0));
    ofs = select(clamp(int2(coord) + ofs, 0, src_max) == int2(coord) + ofs, ofs, -ofs);
    FLATTEN
    if ((frame & 1) != (coord.y & 1))// this is pixel obtained on current frame
      ofs = 0;
    coord = coord + ofs;
    coord.x = coord.x>>1;
  }
  return texelFetch(target, coord, 0).x;
}

#endif
