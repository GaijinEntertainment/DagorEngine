#define TEMPORAL_PROBES_FRAMES 8
#define SP_SPATIAL_FILTER 1
#define SP_TEMPORAL_BILINEAR_EXPAND 0
#define SP_SCREENSPACE_TRACE 1 // -20% performance of HQ, -30% of LQ, but much better quality
#define SP_PROBE_SIZE sqrt(3.) // total size of any voxel
#define SP_LQ_PROBE_NORMAL_BIAS 0.25 // offsets in probe normal direction on SCREENPROBES_HQ == false, to avoid self occlusion
#define SP_HQ_PROBE_NORMAL_BIAS_SDF 0.25 // offsets in probe normal direction on SCREENPROBES_HQ == true, to avoid self occlusion

#define SP_MATCH_PROBES_OFFSETED 0 // we don't use probes ofsetting when marking, due to lower stability (probe Normal). However, it could increase quality

// about 'bright points'
// if DEPTH_ONLY_WEIGHTENING 0, we use plane weightening. I.e. we measure not probes rel depth from point, but probes distance to plane at point + worldNormal.
// now consider a point (.) on a wall corner with camera (*) looking from glazing angle and two probes (x).
//                   __ _x_
//                        |
//                      <-.
//                        |
//                        |
//                        |
//                        |x___
//
//                       * (camera)
// distance from plane for both probes is same, so point will be lit as bilinear interpolation from both (producing incorrect lighting)
// if we use DEPTH_ONLY_WEIGHTENING, that won't be an issue, as depth difference between two probes is significant, and only upper probe will be used
// however if we use DEPTH_ONLY_WEIGHTENING, we will see different issue.
// consider camera moving a bit to the left
//                      _x_
//                        |
//                        x
//                      <-.
//                        |
//                        x
//                        |__
//
//                   * (camera)
// now, there are two more probes on a same wall as point.
// and although both probes are facing same direction and point is almost right between them, bilinear filtering won't happen!
// as depth difference is still signficant. with DEPTH_ONLY_WEIGHTENING 0, both would be used, as plane distance is almost zero for both of them
// todo: use some kind of both weights. depth weight probably should be low

#define DEPTH_ONLY_WEIGHTENING 0

#ifndef SP_ENCODING
#define SP_ENCODING 1

#include <pixelPacking/PixelPacking_R11G11B10.hlsl>

float3 sp_decode_texture_radiance(float3 encoded)
{
  return encoded;//we increase precision with per-probe average (pre-exposure)
  //we store data in r11g11b10f, it has awful precision. So, we increase quality by introducing gamma 2
  //https://bartwronski.com/2017/04/02/small-float-formats-r11g11b10f-precision/
  //return sqrt(encoded);
}

float3 sp_encode_texture_radiance(float3 decoded)
{
  return decoded;//we increase precision with per-probe averagedecoded;
  //return decoded*decoded;
}

#endif