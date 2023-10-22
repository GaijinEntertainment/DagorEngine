include "shader_global.sh"
include "invGlobTm.sh"
include "waveWorks.sh"
include "gaussian_blur.sh"

texture foam_mask;
texture foam_mask_depth;
texture foam_generator_tile;
texture foam_generator_gradient;
texture foam_overfoam_tex;
texture foam_underfoam_tex;
texture foam_final_tex;

float foam_tile_uv_scale = 0.05;
float foam_distortion_scale = 0.75;
float foam_normal_scale = 0.1;
float foam_pattern_gamma = 2.2;
float foam_mask_gamma = 2.2;
float foam_gradient_gamma = 2.2;
float foam_underfoam_threshold = 0.1;
float foam_overfoam_threshold = 0.25;
float foam_underfoam_weight = 0.2;
float foam_overfoam_weight = 1.0;
float4 foam_underfoam_color = (1,1,1,1);
float4 foam_overfoam_color = (1,1,1,1);

int foam_prepare_debug_step = 4;
interval foam_prepare_debug_step: prepare_step_pattern<1, prepare_step_masked_pattern<2, prepare_step_threshold<3, prepare_step_thresholded_pattern<4, all_prepare_steps;

texture water_flowmap_tex;
texture water_flowmap_tex_add;
float4 world_to_flowmap = (1/32,1/32,0.5,0.5);
float4 world_to_flowmap_add = (1,1,0,0);
float water_wind_strength = 0.2;
float water_flowmap_fading = 3;
float4 water_flowmap_strength = (1,5,0.5,0); // speed, foam, curvature, mask
float4 water_flowmap_strength_add = (0.5,1,1,0.3); // min distance,max distance,speed,foam
float4 water_flowmap_foam = (5,10,0.5,0.1); // power, scale, threshold, reflectivity
float water_flowmap_foam_tiling = 1;
float flowmap_texture_size_meters = 200;

shader foam_prepare
{
  cull_mode = none;
  z_test = false;
  z_write = false;

  INIT_INV_GLOBTM(ps)

  hlsl
  {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      noperspective float2 uv : TEXCOORD0;
      float2 screen_pos : TEXCOORD1;
    };
  }

  USE_POSTFX_VERTEX_POSITIONS()

  INIT_WATER_GRADIENTS()
  USE_WATER_GRADIENTS(1)

  hlsl(vs) {
    VsOutput prepare_vs(uint vertexId : SV_VertexID)
    {
      VsOutput output;
      float2 inpos = getPostfxVertexPositionById(vertexId);

      output.pos = float4(inpos, 1, 1);
      output.uv = screen_to_texcoords(inpos);
      output.screen_pos = inpos;
      return output;
    }
  }

  (ps)
  {
    foam_mask@smp2d = foam_mask;
    foam_mask_depth@smp2d = foam_mask_depth;
    foam_generator_tile@smp2d = foam_generator_tile;
    foam_generator_gradient@smp2d = foam_generator_gradient;
    foam_scale_threshold@f4 = (foam_tile_uv_scale, foam_distortion_scale, foam_overfoam_threshold, foam_underfoam_threshold);
    foam_gamma@f4 = (foam_pattern_gamma, 1 / foam_pattern_gamma, foam_mask_gamma, 1 / foam_mask_gamma);

    wind_dir@f2 = (wind_dir_x, wind_dir_y, 0, 0);
    water_flowmap_tex@smp2d = water_flowmap_tex;
    water_flowmap_tex_add@smp2d = water_flowmap_tex_add;
    world_to_flowmap@f4 = world_to_flowmap;
    world_to_flowmap_add@f4 = world_to_flowmap_add;
    cascadesTexelScale@f1 = (cascadesTexelScale0123.x);
    current_time_water_wind_strength_flowmap_fading@f4 = (time_phase(0, 0), water_wind_strength, water_flowmap_fading, 1 / water_flowmap_fading);
    water_flowmap_strength@f4 = (water_flowmap_strength);
    water_flowmap_strength_add@f4 = (-2/((water_flowmap_strength_add.y-water_flowmap_strength_add.x)*flowmap_texture_size_meters),
      water_flowmap_strength_add.y/(water_flowmap_strength_add.y-water_flowmap_strength_add.x), water_flowmap_strength_add.z, water_flowmap_strength_add.w);
    water_flowmap_foam@f4 = (water_flowmap_foam);
  }

  hlsl(ps) {
    #define foam_tile_uv_scale (foam_scale_threshold.x)
    #define foam_distortion_scale (foam_scale_threshold.y)
    #define foam_overfoam_threshold (foam_scale_threshold.z)
    #define foam_underfoam_threshold (foam_scale_threshold.w)
    #define foam_pattern_gamma (foam_gamma.x)
    #define inv_foam_pattern_gamma (foam_gamma.y)
    #define foam_mask_gamma (foam_gamma.z)
    #define inv_foam_mask_gamma (foam_gamma.w)

    #define current_time (current_time_water_wind_strength_flowmap_fading.x)
    #define water_wind_strength (current_time_water_wind_strength_flowmap_fading.y)
    #define water_flowmap_fading (current_time_water_wind_strength_flowmap_fading.z)
    #define inv_water_flowmap_fading (current_time_water_wind_strength_flowmap_fading.w)
    #define water_flowmap_foam_power (water_flowmap_foam.x)
    #define water_flowmap_foam_scale (water_flowmap_foam.y)
    #define water_flowmap_foam_threshold (water_flowmap_foam.z)
    #define water_flowmap_foam_reflectivity (water_flowmap_foam.w)
  }

  hlsl(ps)
  {
    struct PrepareOutput
    {
      float4 overfoam: SV_Target0;
      float4 underfoam: SV_Target1;
    };

    PrepareOutput prepare_ps(VsOutput input)
    {
      // Depth projection
      float mask_depth = tex2D(foam_mask_depth, input.uv).r;
      float4 screen_position = float4(input.screen_pos.xy, mask_depth, 1);
      float4 world_position = mul(screen_position, globtm_inv);
      world_position /= world_position.w;
      // Waveworks
      float worldPosDistance = 1.0;
      float4 nvsf_tex_coord_cascade01, nvsf_tex_coord_cascade23, nvsf_tex_coord_cascade45, nvsf_blendfactors0123, nvsf_blendfactors4567;
      get_blend_factor(world_position.xzy, worldPosDistance, nvsf_tex_coord_cascade01, nvsf_tex_coord_cascade23,
                      nvsf_tex_coord_cascade45, nvsf_blendfactors0123, nvsf_blendfactors4567);

      float nvsf_foam_turbulent_energy, nvsf_foam_surface_folding, nvsf_foam_wave_hats;
      float3 nvsf_normal;
      float fadeNormal = 1;
      get_gradients(nvsf_tex_coord_cascade01, nvsf_tex_coord_cascade23, nvsf_tex_coord_cascade45, nvsf_blendfactors0123,
            nvsf_blendfactors4567, fadeNormal, nvsf_foam_turbulent_energy, nvsf_foam_surface_folding, nvsf_normal, nvsf_foam_wave_hats);

      // Masking
      float mask = pow(tex2D(foam_mask, input.uv).r, inv_foam_mask_gamma);

      // Pattern
      ##if water_flowmap_tex == NULL && water_flowmap_tex_add == NULL
        float pattern = pow(tex2D(foam_generator_tile, foam_tile_uv_scale*(world_position.xz-foam_distortion_scale*nvsf_normal.xy)).a, inv_foam_pattern_gamma);
      ##else
        float3 worldPos = world_position.xyz;
        float2 windVec = wind_dir.xy * cascadesTexelScale * current_time * water_wind_strength;

        float3 flowmapBias = float3(0.5, 0.5, 0);
        float3 flowmapSample = flowmapBias;
        ##if water_flowmap_tex != NULL
          float2 flowmapUV1 = worldPos.xz * world_to_flowmap.xy + world_to_flowmap.zw;
          if (all(flowmapUV1 >= 0) && all(flowmapUV1 <= 1))
          {
            float3 flowmapSample1 = tex2Dlod(water_flowmap_tex, float4(flowmapUV1, 0, 0)).xyz - flowmapBias;
            float2 flowmapUV2 = flowmapUV1 + flowmapSample1.xy * world_to_flowmap.xy;
            float3 flowmapSample2 = tex2Dlod(water_flowmap_tex, float4(flowmapUV2, 0, 0)).xyz - flowmapBias;
            float flowmapFoam = abs(dot(float2(flowmapSample1.x, flowmapSample1.y), float2(flowmapSample2.y, -flowmapSample2.x)));
            flowmapFoam = saturate(pow(flowmapFoam, water_flowmap_strength.z) * water_flowmap_strength.y);
            flowmapSample += flowmapSample1 * water_flowmap_strength.x;
            flowmapSample.z += flowmapFoam;
          }
        ##endif
        ##if water_flowmap_tex_add != NULL
          float2 flowmapUVAdd = worldPos.xz * world_to_flowmap_add.xy + world_to_flowmap_add.zw;
          if (all(flowmapUVAdd >= 0) && all(flowmapUVAdd <= 1))
          {
            float3 flowmapStrengthAdd = saturate(worldPosDistance * water_flowmap_strength_add.x + water_flowmap_strength_add.y) * water_flowmap_strength_add.zzw;
            flowmapSample += tex2Dlod(water_flowmap_tex_add, float4(flowmapUVAdd, 0, 0)).xyz * flowmapStrengthAdd;
          }
        ##endif
        float2 flowmapVec = flowmapSample.xy - flowmapBias.xy;
        flowmapVec *= water_flowmap_fading;

        float flowmapTime = current_time * inv_water_flowmap_fading;
        float2 flowmapVec_a = flowmapVec * frac(flowmapTime + 0.0) - windVec;
        float2 flowmapVec_b = flowmapVec * frac(flowmapTime + 0.5) - windVec;

        float3 worldPos_a = worldPos;
        float3 worldPos_b = worldPos;

        worldPos_a.xz += flowmapVec_a;
        worldPos_b.xz += flowmapVec_b;

        float2 foamTileDistortion = nvsf_normal.xy * foam_distortion_scale;
        float flowFoamTileA = pow(tex2D(foam_generator_tile, (worldPos_a.xz - foamTileDistortion) * foam_tile_uv_scale).a, inv_foam_pattern_gamma);
        float flowFoamTileB = pow(tex2D(foam_generator_tile, (worldPos_b.xz - foamTileDistortion) * foam_tile_uv_scale).a, inv_foam_pattern_gamma);
        float crossFade = abs(frac(flowmapTime) * 2 - 1);
        crossFade = smoothstep(0, 1, crossFade);
        half flowFoamTile = lerp(flowFoamTileA, flowFoamTileB, crossFade);
        float flowFoamStrength = min(pow(flowmapSample.z, water_flowmap_foam_power) * water_flowmap_foam_scale, 1 + water_flowmap_foam_threshold);
        flowFoamStrength *= saturate(water_flowmap_strength.w);

        float pattern = flowFoamTile;
        mask += flowFoamStrength;
      ##endif

      ##if foam_prepare_debug_step == prepare_step_pattern
      {
        PrepareOutput result;
        result.overfoam = float4(pattern, pattern, pattern, 1.0);
        result.underfoam = float4(pattern, pattern, pattern, 1.0);
        return result;
      }
      ##endif

      float masked_pattern = pattern-(1.0-mask);
      ##if foam_prepare_debug_step == prepare_step_masked_pattern
      {
        PrepareOutput result;
        result.overfoam = float4(masked_pattern, masked_pattern, masked_pattern, 1.0);
        result.underfoam = float4(masked_pattern, masked_pattern, masked_pattern, 1.0);
        return result;
      }
      ##endif
      // Threshold
      float overfoam_mask = step(foam_overfoam_threshold, masked_pattern);
      float underfoam_mask = step(foam_underfoam_threshold, masked_pattern);
      ##if foam_prepare_debug_step == prepare_step_threshold
      {
        PrepareOutput result;
        result.overfoam = float4(overfoam_mask, overfoam_mask, overfoam_mask, 1.0);
        result.underfoam = float4(underfoam_mask, underfoam_mask, underfoam_mask, 1.0);
        return result;
      }
      ##endif
      // Patterning
      float overfoam_pattern = overfoam_mask*pattern;
      float underfoam_pattern = underfoam_mask*pattern;
      ##if foam_prepare_debug_step == prepare_step_thresholded_pattern
      {
        PrepareOutput result;
        result.overfoam = float4(overfoam_pattern, overfoam_pattern, overfoam_pattern, 1.0);
        result.underfoam = float4(underfoam_pattern, underfoam_pattern, underfoam_pattern, 1.0);
        return result;
      }
      ##endif
      // Result
      PrepareOutput result;
      result.overfoam = float4(overfoam_pattern, overfoam_pattern, overfoam_pattern, 1.0);
      result.underfoam = float4(underfoam_pattern, underfoam_pattern, underfoam_pattern, 1.0);
      return result;
    }
  }

  compile("target_vs", "prepare_vs");
  compile("target_ps", "prepare_ps");
}

shader foam_combine
{
  cull_mode = none;
  z_test = false;
  z_write = false;

  POSTFX_VS_TEXCOORD(1, uv)

  (ps)
  {
    foam_overfoam_tex@smp2d = foam_overfoam_tex;
    foam_underfoam_tex@smp2d = foam_underfoam_tex;
    foam_generator_gradient@smp2d = foam_generator_gradient;
    foam_gamma_weight@f3 = (foam_gradient_gamma, foam_underfoam_weight, foam_overfoam_weight);
    foam_underfoam_color@f3 = (foam_underfoam_color);
    foam_overfoam_color@f3 = (foam_overfoam_color);
  }

  hlsl(ps)
  {
    #define foam_gradient_gamma (foam_gamma_weight.x)
    #define foam_underfoam_weight (foam_gamma_weight.y)
    #define foam_overfoam_weight (foam_gamma_weight.z)

    float4 combine_ps(VsOutput input): SV_Target
    {
      float overfoam_gray = tex2D(foam_overfoam_tex, input.uv).r;
      float underfoam_gray = tex2D(foam_underfoam_tex, input.uv).r;

      float3 overfoam_color = pow(tex2D(foam_generator_gradient, float2(overfoam_gray, 0.0)).rgb, 1.0/foam_gradient_gamma);
      float3 underfoam_color = pow(tex2D(foam_generator_gradient, float2(underfoam_gray, 0.0)).rgb, 1.0/foam_gradient_gamma);

      overfoam_color *= foam_overfoam_color;
      underfoam_color *= foam_underfoam_color;

      float3 final_color = foam_underfoam_weight*underfoam_color+foam_overfoam_weight*overfoam_color;
      float final_alpha = foam_underfoam_weight*underfoam_gray+foam_overfoam_weight*overfoam_gray;

      return float4(final_color, saturate(1.0-final_alpha));
    }
  }

  compile("target_ps", "combine_ps");
}

shader foam_height
{
  cull_mode = none;
  z_test = false;
  z_write = false;

  blend_src = one;
  blend_dst = one;

  POSTFX_VS_TEXCOORD(1, uv)

  (ps)
  {
    foam_overfoam_tex@smp2d = foam_overfoam_tex;
    foam_normal_scale@f1 = (foam_normal_scale);
  }

  hlsl(ps)
  {
    float4 height_ps(VsOutput input): SV_Target
    {
      float overfoam = tex2D(foam_overfoam_tex, input.uv).r;
      return float4(overfoam*foam_normal_scale, 0, 0, 1);
    }
  }

  compile("target_ps", "height_ps");
}

texture tex;
float4 texsz = (0.5, -0.5, 0.5, 0.5);

shader foam_blur_x, foam_blur_y
{
  cull_mode=none;
  z_test=false;
  z_write=false;
  no_ablend;

  (ps) {
    tex@smp2d = tex;
    texsz@f4 = texsz;
  }

  USE_POSTFX_VERTEX_POSITIONS()

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float2 tc : TEXCOORD0;
    };
  }

  hlsl(vs) {
    VsOutput blur_vs(uint vertex_id : SV_VertexID)
    {
      VsOutput output;
      float2 pos = getPostfxVertexPositionById(vertex_id);
      float2 tc = screen_to_texcoords(pos);
      output.pos = float4(pos.x, pos.y, 1, 1);
      output.tc.xy = tc;
      return output;
    }
  }

  hlsl(ps) {
    #define GAUSSIAN_BLUR_STEPS_COUNT 4
    #define GAUSSIAN_BLUR_COLOR_TYPE float

    void gaussianBlurSampleColor(float2 centre_uv, float2 tex_coord_offset, out GAUSSIAN_BLUR_COLOR_TYPE out_color)
    {
      out_color = tex2Dlod(tex, float4(centre_uv + tex_coord_offset, 0, 0)).r +
                  tex2Dlod(tex, float4(centre_uv - tex_coord_offset, 0, 0)).r;
    }
  }

  USE_GAUSSIAN_BLUR()

  hlsl(ps) {
    float4 blur_ps(VsOutput input) : SV_Target
    {
      ##if shader == blur_x
        return GaussianBlur(input.tc, float2(texsz.z, 0));
      ##else
        return GaussianBlur(input.tc, float2(0, texsz.w));
      ##endif
    }
  }

  compile("target_vs", "blur_vs");
  compile("target_ps", "blur_ps");
}
