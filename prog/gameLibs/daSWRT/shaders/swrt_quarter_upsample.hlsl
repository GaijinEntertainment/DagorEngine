#ifndef SWRT_QUARTER_UPSAMPLE_HLSL
#define SWRT_QUARTER_UPSAMPLE_HLSL 1

// the pixel of each 2x2 half-res block that the quarter rate traces on a frame
uint2 swrt_quarter_phase(uint frame) { return uint2(frame & 1, ((frame >> 1) ^ frame) & 1); }

// target: .x shadow, .y linear depth of the traced pixel; coord: the half-res pixel, w: the receiver's linear depth,
// src_max: the last valid half-res pixel, as dynamic resolution uses a part of the target; frame: swrt_checkerboard_frame
float swrt_quarter_upsample(Texture2D<float4> target, uint2 coord, float w, uint2 src_max, uint frame)
{
  int2 tile = int2(coord >> 1), tileMax = int2(src_max >> 1);
  int2 side = int2((coord.x & 1) ? 1 : -1, (coord.y & 1) ? 1 : -1);
  int2 next = clamp(tile + side, 0, tileMax);
  float2 s00 = texelFetch(target, tile, 0).xy, s10 = texelFetch(target, int2(next.x, tile.y), 0).xy;
  float2 s01 = texelFetch(target, int2(tile.x, next.y), 0).xy, s11 = texelFetch(target, next, 0).xy;
  float4 depthDiff = abs(float4(s00.y, s10.y, s01.y, s11.y) - w) * rcp(max(w, 1e-3));
  // per axis: the own tap where it traced the receiver's row or column, else the traced pixels on both sides
  float2 own = 0.5 + 0.5 * float2(min(2 * uint2(tile) + swrt_quarter_phase(frame), src_max) == coord);
  float4 weight = float4(own.x, 1 - own.x, own.x, 1 - own.x) * float4(own.yy, 1 - own.yy) * rcp(1 + depthDiff * depthDiff * 1024);
  float sum = dot(weight, 1);
  return sum > 1e-3 ? dot(weight, float4(s00.x, s10.x, s01.x, s11.x)) / sum : s00.x;
}

#endif
