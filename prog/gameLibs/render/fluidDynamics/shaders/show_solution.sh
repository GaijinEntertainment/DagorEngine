include "shader_global.sh"
include "postfx_inc.sh"

texture plot_tex;
int plot_type = 0;

shader show_cfd_solution
{
  cull_mode  = none;
  z_test = false;
  z_write = false;
  no_ablend;

  ENABLE_ASSERT(ps)
  POSTFX_VS_TEXCOORD(0, texcoord)

  (ps) {
    plot_tex@smp2d = plot_tex;
    plot_type@i1 = plot_type;
  }

  hlsl(ps) {
    float4 show_solution_ps(VsOutput input) : SV_Target
    {
      float4 val = tex2Dlod(plot_tex, float4(input.texcoord.xy,0,0));
      if (plot_type == 0) // DENSITY
      {
        float combined_density = dot(val, float4(1, 1, 1, 1));
        return lerp(float4(0, 0, 1, 1), float4(1, 0, 0, 1), smoothstep(0.8, 1.5, combined_density));
      }
      else if (plot_type == 1) // SPEED
      {
        float magnitude = length(float2(val.x, val.y));
        return magnitude > 0 ? lerp(float4(0, 0, 1, 1), float4(1, 0, 0, 1), smoothstep(5, 25.0, magnitude)) : float4(0, 0, 0, 1);
      }
      else if (plot_type == 2) // PRESSURE
        return lerp(float4(0, 0, 1, 1), float4(1, 0, 0, 1), smoothstep(99000, 103700, val.z)); // In pascals
      else if (plot_type == 3) // PLACEHOLDER
        return float4(0.5, 0.5, 0.5, 1);
      else
        return float4(0, 1, 1, 1);
    }
  }

  compile("target_ps", "show_solution_ps");
}
