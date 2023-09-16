include "shader_global.sh"
include "depth_above.sh"
include "postfx_inc.sh"

float4 depth_above_copy_region;

shader depth_above_copy_regions
{
  supports global_frame;
  z_test=true;
  z_write=true;
  cull_mode=none;
  no_ablend;

  (ps) {
    depth_above@smp2d = depth_around;
  }

  POSTFX_VS(0)
  ENABLE_ASSERT(ps)

  hlsl(ps) {
    float main_ps(VsOutput input) : SV_Depth
    {
      return texelFetch(depth_above, input.pos.xy, 0).x;
    }
  }
  compile("target_ps", "main_ps");
}
