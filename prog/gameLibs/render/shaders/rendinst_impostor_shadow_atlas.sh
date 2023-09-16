include "shader_global.sh"
include "viewVecVS.sh"

include "rendinst_opaque_inc.sh"

include "rendinst_impostor_inc.sh"
include "rendinst_vegetation_inc.sh"
include "rendinst_inc.sh"

float4 impostor_slice = float4(0, 0, 0, 0);
texture impostor_atlas_mask;

texture impostor_shadow_texture;
int impostor_data_offset = 0;

shader impostor_shadow_atlas
{
  supports global_frame;

  cull_mode=none;
  z_write=false;
  z_test=false;

  USE_POSTFX_VERTEX_POSITIONS()
  hlsl {
    #define DYNAMIC_TEX 1
  }

  INIT_BAKED_IMPOSTOR_VARS(dynamic)
  ROTATION_PALETTE_STUB()
  USE_BAKED_IMPOSTORS()


  OCTAHEDRAL_UTILS()

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float2 sliceTc : TEXCOORD0;
      float4 tc_clipping : TEXCOORD1;
      nointerpolation float4 viewDirX_texOffsetX : TEXCOORD2;
      nointerpolation float4 viewDirY_texOffsetY : TEXCOORD3;
      nointerpolation float3 viewDirZ : TEXCOORD4;
    };
  }
  (vs) {
    impostor_slice@f2 = impostor_slice;
    gbuffer_sun_light_dir@f3 = from_sun_direction;
    impostor_data_offset@i1 = (impostor_data_offset);

    impostor_options@f4 = (impostor_options.x, impostor_options.y, 1/impostor_options.x, 1/impostor_options.y);
    impostor_scale@f4 = (impostor_scale.x, impostor_scale.y, 1/impostor_scale.x, 1/impostor_scale.y);
    impostor_bounding_sphere@f4 = impostor_bounding_sphere;

    impostor_data_buffer@buf = impostor_data_buffer hlsl {
      Buffer<float4> impostor_data_buffer@buf;
    };
  }
  hlsl(vs) {
    #include <rendinst_impostor_dirs.hlsli>

    VsOutput shadow_vs(uint vertexId : SV_VertexID)
    {
      VsOutput output;
      float2 inpos = getPostfxVertexPositionById(vertexId);
      output.pos = float4(inpos.x,inpos.y,0,1);

      uint slice_n = uint(impostor_slice.y * impostor_options.x + impostor_slice.x + 0.1);

      float2 tc = screen_to_texcoords(inpos);
      float4 tcTm = get_impostor_slice_tc_tm(impostor_data_offset, slice_n);
      float4 tcTmInv = float4(1.f/tcTm.x, 1.f/tcTm.y, -tcTm.z/tcTm.x, -tcTm.w/tcTm.y);

      output.sliceTc = tcTmInv.xy * tc + tcTmInv.zw;
      output.tc_clipping = apply_slice_data(impostor_data_offset, slice_n, output.sliceTc);

      float2 tex = impostor_slice * impostor_options.zw;
      float2 textureOffset = get_texture_offset(tex);
      output.viewDirZ = IMPOSTOR_SLICE_DIRS[slice_n];

      output.viewDirY_texOffsetY.xyz = abs(output.viewDirZ.y) == 1 ? float3(1, 0, 0) : float3(0, 1, 0);
      output.viewDirX_texOffsetX.xyz = normalize(cross(output.viewDirY_texOffsetY.xyz, output.viewDirZ));
      // This is flipped intentionally
      // otherwise the tc.y should be flipped in ps
      output.viewDirY_texOffsetY.xyz = cross(output.viewDirX_texOffsetX.xyz, output.viewDirZ);

      output.viewDirX_texOffsetX.xyz *= impostor_scale.x;
      output.viewDirY_texOffsetY.xyz *= impostor_scale.y;
      output.viewDirZ *= impostor_bounding_sphere.w;

      output.viewDirX_texOffsetX.w = slice_n;
      output.viewDirY_texOffsetY.w = textureOffset.y;

      return output;
    }
  }

  INIT_RENDINST_IMPOSTOR_SHADOW()
  USE_RENDINST_IMPOSTOR_SHADOW()

  (ps)
  {
    impostor_options@f4 = (impostor_options.x, impostor_options.y, 1/impostor_options.x, 1/impostor_options.y);
    impostor_shadow_texture@smp2d = impostor_shadow_texture;
    impostor_bounding_sphere@f4 = impostor_bounding_sphere;
    impostor_atlas_mask@smp2d = impostor_atlas_mask;
  }

  hlsl(ps) {
    float shadow_ps(VsOutput input) : SV_Target
    {
      if (need_clip_impostor(input.tc_clipping.zw))
        discard;
      float2 atlasTc = input.tc_clipping.xy;
      ##if impostor_atlas_mask != NULL
        float alpha = tex2D(impostor_atlas_mask, atlasTc).a;
        clip(alpha-0.5);

        float depth = 1-tex2D(impostor_shadow_texture, atlasTc).x;
        float3 viewPos = float3(input.sliceTc, depth)*2-1;
        float3 modelPos = mul(
          float4(viewPos, 1),
          float4x3(input.viewDirX_texOffsetX.xyz, input.viewDirY_texOffsetY.xyz, input.viewDirZ, impostor_bounding_sphere.xyz));

        return getShadow(world_view_pos.xyz - modelPos);
      ##else
        return 1;
      ##endif
    }
  }
  compile("target_vs", "shadow_vs");
  compile("target_ps", "shadow_ps");
}
