texture clouds_depth_downsampled_panorama_tex;

float4 lightning_point_light_pos = (0, 0, 0, 0);
float4 lightning_point_light_color = (0.8, 0.8, 0.8, 0);
float lightning_additional_point_light_radius = 100000;
float lightning_additional_point_light_strength = 1.0;
int lightning_render_additional_point_light = 0;

interval lightning_render_additional_point_light: no<1, yes;

macro INIT_PANORAMA_LIGHTNING_FLASH()
  if (clouds_depth_downsampled_panorama_tex != NULL && lightning_render_additional_point_light == yes)
  {
    (ps) {
      lightning_flash_params@f2 = (lightning_additional_point_light_strength, lightning_additional_point_light_radius, 0, 0);
      lightning_pos@f3 = lightning_point_light_pos;
      lightning_color@f3 = lightning_point_light_color;
      clouds_depth_panorama_tex@smp2d = clouds_depth_downsampled_panorama_tex;
    }
  }
endmacro

macro USE_PANORAMA_LIGHTNING_FLASH()
  hlsl(ps)
  {
    void apply_lightning_flash(float2 sky_uv, float3 view, inout half3 color)
    {
      ##if clouds_depth_downsampled_panorama_tex != NULL && lightning_render_additional_point_light == yes
        //TODO: move some of the calculations to preshader
        float3 viewDirectionFlash = lightning_pos.xyz - skies_world_view_pos;
        float viewDistFlash = length(viewDirectionFlash);
        viewDirectionFlash = normalize(viewDirectionFlash);
        float2 fragPanoramaUV = sky_uv;
        float fragdDepth = tex2Dlod(clouds_depth_panorama_tex, float4(fragPanoramaUV, 0, 0)).x;
        float2 flashPanoramaUV = get_panorama_uv(skies_world_view_pos, viewDirectionFlash);
        float flashDepth = tex2Dlod(clouds_depth_panorama_tex, float4(flashPanoramaUV, 0, 0)).x;
        float maxDist = viewDistFlash / flashDepth;
        float3 fragPos = view * fragdDepth * maxDist;
        float3 flashPos = lightning_pos.xyz;
        float flashDist = length(fragPos - flashPos);
        float x = flashDist / lightning_flash_params.y;
        float distanceFade = lightning_flash_params.x * exp(-x * x );
        color.rgb += lightning_color * saturate(distanceFade);
      ##endif
    }
  }
endmacro
