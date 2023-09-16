include "postfx_inc.sh"
include "sky_shader_global.sh"
include "skies_special_vision.sh"
include "vr_reprojection.sh"

texture lowres_sky;
shader applySkies
{
  cull_mode = none;

  z_write = false;
  z_test = true;

  POSTFX_VS_TEXCOORD(0, texcoord)

  INIT_BOUNDING_VIEW_REPROJECTION(ps)
  USE_BOUNDING_VIEW_REPROJECTION(ps)

  (ps) { lowres_sky@smp2d = lowres_sky;}
  hlsl(ps) {
    half4 apply_skies_ps(VsOutput input):SV_Target
    {
      float2 tc = input.texcoord;
##if use_bounding_vr_reprojection == on
      tc = vr_bounding_view_reproject_tc(tc,0);
##endif
      float4 sky = tex2Dlod(lowres_sky, float4(tc,0,0));
      sky.a = 0;
      return sky;
    }
  }
  compile("target_ps", "apply_skies_ps");
}
