include "hardware_defines.sh"
include "shader_global.sh"
include "land_block_inc.sh"
include "normaldetail.sh"
include "terraform_inc.sh"

texture terraform_tex_data;
float4 terraform_tex_data_pivot_size = (0, 0, 0, 0);
float4 terraform_tex_data_size = (1, 1, 1, 1);

// in a linear space
float4 tform_above_ground_color = (0.42, 0.1764, 0.04, 0.5);
float4 tform_under_ground_color = (0.14, 0.06, 0.0, 0.5);

float tform_dig_ground_alpha = 1.0;
float tform_dig_roughness = 0.2;
float tform_texture_blend = 1.0;
float tform_texture_blend_above = 1.0;

int terraform_render_mode = 0;
interval terraform_render_mode: terraform_render_color < 1, terraform_render_height < 2, terraform_render_height_mask < 3, terraform_render_mask;

texture tform_diffuse_tex;
texture tform_diffuse_above_tex;
texture tform_detail_rtex;
texture tform_normals_tex;

float4 tform_texture_tile = (1, 1, 0, 0);

shader terraform_patch
{
  z_write = false;
  z_test = false;
  cull_mode = none;

  USE_NORMAL_DETAIL()

  if (terraform_render_mode == terraform_render_color && render_with_normalmap != render_displacement)
  {
    hlsl(ps) {
      #define RENDER_TO_COLOR 1
    }
    blend_src = sa; blend_dst = isa;
    blend_asrc = one; blend_adst = zero;
  }
  else if (terraform_render_mode == terraform_render_height)
  {
    blend_src = sa; blend_dst = isa;
    blend_asrc = one; blend_adst = zero;
  }
  else
  {
    no_ablend;
  }

  supports global_frame;

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float4 pos_tc: TEXCOORD0;
    };
  }

  (vs) {
    globtm@f44=globtm;
  }

  (vs) {
    tex_data_pivot_size@f4 = terraform_tex_data_pivot_size;
  }

  hlsl(vs) {
    VsOutput terraform_patch_vs(uint vertexId: SV_VertexID)
    {
      uint instanceNo = vertexId / 4;
      uint vertexNo = vertexId % 4;

      float2 vpos = float2(vertexNo % 2, vertexNo / 2);
      FLATTEN
      if ((vertexNo / 2) % 2 == 0)
        vpos.x = 1 - vpos.x;
      float2 pos = tex_data_pivot_size.xy + vpos * tex_data_pivot_size.zw;

      VsOutput output;
      output.pos = mul(float4(pos.x, 0, pos.y, 1.0), globtm);
      output.pos.z = 0.5;
      output.pos_tc = float4(pos.xy, vpos);
      return output;
    }
  }
  compile("target_vs", "terraform_patch_vs");

  (ps) {
    tex_data@smp2d = terraform_tex_data;
    diffuse_tex@smp2d = tform_diffuse_tex;
    diffuse_above_tex@smp2d = tform_diffuse_above_tex;
    detail_rtex@smp2d = tform_detail_rtex;
    normals_tex@smp2d = tform_normals_tex;
    texture_tile@f2 = tform_texture_tile;
    min_max_level@f4 = (terraform_min_max_level.x, terraform_min_max_level.y - terraform_min_max_level.x,
      (terraform_min_max_level.y - terraform_min_max_level.x) / 255, 255 / (terraform_min_max_level.y - terraform_min_max_level.x));
    tex_data_size@f4 = (terraform_tex_data_size);
    tex_data_pivot_size@f4 = terraform_tex_data_pivot_size;

    above_ground_color@f4 = tform_above_ground_color;
    under_ground_color@f4 = tform_under_ground_color;
    dig_ground_alpha@f1 = (tform_dig_ground_alpha);
    dig_roughness@f1 = (tform_dig_roughness);
    texture_blend@f1 = (tform_texture_blend);
    texture_blend_above@f1 = (tform_texture_blend_above);
  }

  USE_CLIPMAP_MRT_OUTPUT()

  hlsl(ps) {
    #include <pixelPacking/ColorSpaceUtility.hlsl>

    // bias and account for linear filtration
    #define DISTURBED_GROUND_BIAS (min_max_level.z * 0.5)

    #define CLIP_ALPHA_BORDER 0.001
    #define DISTURBED_WEIGHT_TOLERANCE 25

  #if RENDER_TO_COLOR
    VTEX_MRT terraform_patch_ps(VsOutput input)
  #else
    float4 terraform_patch_ps(VsOutput input): SV_Target
  #endif
    {
      float2 tc = input.pos_tc.zw;
      float2 pos = input.pos_tc.xy;
      float2 posTiled = pos * texture_tile.xy;

      #define GET(a, b) tex2Dlod(tex_data, float4(tc + float2(a, b) * tex_data_size.zw, 0, 0)).r
      #define RADIUS 5
      #define DIAM (2 * RADIUS - 1)
      const float SQUARE = DIAM * DIAM;
      const float SQUARE_INV = 1.0 / SQUARE;

      float zeroLevel = -min_max_level.x / min_max_level.y;
      float cols[DIAM][DIAM];
      float texHeight = 0;
      bool disturbed = false;
      float heapWeight = 0;
      float holeWeight = 0;
      for (int x = -RADIUS + 1; x < RADIUS; ++x)
        for (int y = -RADIUS + 1; y < RADIUS; ++y)
        {
          float col = GET(x, y);
          cols[x + RADIUS - 1][y + RADIUS - 1] = col;
          texHeight += col;
          disturbed = disturbed || abs(col - zeroLevel) - DISTURBED_GROUND_BIAS > 0;
          heapWeight += (col - zeroLevel) > DISTURBED_GROUND_BIAS ? 1.0 : 0.0;
          holeWeight += (col - zeroLevel) < -DISTURBED_GROUND_BIAS ? 1.0 : 0.0;
        }
      heapWeight *= SQUARE_INV;
      holeWeight *= SQUARE_INV;
      if (!disturbed)
        discard;
      texHeight = texHeight * SQUARE_INV;
      float dig = saturate(abs(texHeight - zeroLevel) * (min_max_level.w * dig_roughness));
      dig = lerp(dig, 1.0, saturate(heapWeight * holeWeight * DISTURBED_WEIGHT_TOLERANCE));

    #if RENDER_TO_COLOR
      float3 texNormal = 0;
      float sizeInMeters = 2 * tex_data_size.z * tex_data_pivot_size.z;
      for (int nx = -RADIUS + 2; nx < RADIUS - 1; ++nx)
        for (int ny = -RADIUS + 2; ny < RADIUS - 1; ++ny)
        {
          half W = cols[(RADIUS - 1) + nx - 1][(RADIUS - 1) + ny + 0];
          half E = cols[(RADIUS - 1) + nx + 1][(RADIUS - 1) + ny + 0];
          half N = cols[(RADIUS - 1) + nx + 0][(RADIUS - 1) + ny - 1];
          half S = cols[(RADIUS - 1) + nx + 0][(RADIUS - 1) + ny + 1];
          texNormal += normalize(half3(W - E, sizeInMeters, N - S));
        }
      texNormal = RADIUS > 1 ? normalize(texNormal) : float3(0, 1, 0);

      float under = saturate(1.0 - texHeight / zeroLevel);
      float above = saturate((texHeight - zeroLevel) / (1.0 - zeroLevel));

      float4 underColor = under_ground_color;
      float4 aboveColor = above_ground_color;
      ##if tform_diffuse_tex != NULL
      float4 colorMap = tex2D(diffuse_tex, posTiled);
      underColor = lerp(underColor, colorMap, texture_blend);
      ##endif
      ##if tform_diffuse_above_tex != NULL
      float4 colorMapAbove = tex2D(diffuse_above_tex, posTiled);
      aboveColor = lerp(aboveColor, colorMapAbove, texture_blend_above);
      ##endif
      ##if tform_normals_tex != NULL
      float4 normalMap = tex2D(normals_tex, posTiled);
      float3 baseNormal = restore_normal(normalMap.rg);
      baseNormal = lerp(float3(0, 0, 1), baseNormal, texture_blend);
      texNormal = RNM_ndetail_normalized(texNormal.xzy, baseNormal.xyz).xzy;
      ##endif

      float4 diffuse = float4(underColor.rgb, underColor.a * dig_ground_alpha * dig);
      diffuse.a = lerp(diffuse.a, underColor.a, under);
      diffuse.rgb = lerp(diffuse.rgb, aboveColor.rgb, above * aboveColor.a);

      VTEX_MRT ret;
      Vtex vtex = create_vtex();

      set_diffuse(vtex, diffuse.rgb);
      set_normal(vtex, texNormal.xzy);

      ##if tform_detail_rtex != NULL
      float4 rMap = tex2D(detail_rtex, posTiled);
      set_micro_detail(vtex, rMap.a);
      set_smoothness(vtex, rMap.r);
      set_reflectance(vtex, rMap.g);
      set_ao(vtex, rMap.b);
      ##endif

      ret = create_render_vtex(vtex);
      set_alpha(ret, diffuse.a);
      set_alpha_diffuse(ret, diffuse.a);
      set_alpha_normal_microdetail(ret, diffuse.a);
      set_alpha_smoothness_ao(ret, diffuse.a);
      return ret;
    #else
      ##if terraform_render_mode == terraform_render_mask
      return float4(0, 0, 0, 0);
      ##elif terraform_render_mode == terraform_render_height_mask
      return float4(texHeight.xxx, 1);
      ##else
      float colorHeight = 0.5; // just a default value, does not correspond to the zero level
      ##if tform_diffuse_tex != NULL
      float4 colorMap = tex2D(diffuse_tex, posTiled);
      colorHeight = colorMap.a;
      ##endif
      return float4(colorHeight, texHeight.xx, dig);
      ##endif
    #endif
    }
  }

  compile("target_ps", "terraform_patch_ps");
}