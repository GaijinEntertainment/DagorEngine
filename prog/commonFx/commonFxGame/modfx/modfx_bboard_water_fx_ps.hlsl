#ifndef DAFX_MODFX_BBOARD_RENDER_HLSL
#define DAFX_MODFX_BBOARD_RENDER_HLSL

#include "modfx/modfx_curve.hlsli"
#include "modfx/modfx_misc.hlsli"
#include "modfx/modfx_part_data.hlsli"
#include "modfx/modfx_render_placement.hlsli"
#include "modfx/modfx_bboard_common.hlsl"
#include "pbr/BRDF.hlsl"

#if FX_PS

  struct PsOutput
  {
    float4 color : SV_Target0;
    #if MODFX_WATER_FX_OUTPUT_DETAILS
      float height : SV_Target1;
      float4 detail : SV_Target2;
    #endif
  };

  PsOutput encode_output(float4 color, float height, float4 detail)
  {
    PsOutput output;
    output.color = color;
    #if MODFX_WATER_FX_OUTPUT_DETAILS
      output.height = height;
      output.detail = detail;
    #endif
    return output;
  }

  #define fx_null encode_output(float4(0,0,0,0), 0, float4(0,0,0,0));

  PsOutput dafx_bboard_water_fx_ps(VsOutput input)
  {

#if MODFX_DEBUG_RENDER_ENABLED
    PsOutput output = fx_null;
#if !MODFX_RIBBON_SHAPE // TODO: add ribbon support for debug rendering
    output.color.xyz = modfx_pack_hdr(calc_debug_color(input.delta.xy));
#endif
    return output;
#else // MODFX_DEBUG_RENDER_ENABLED

    GlobalData gdata = global_data_load();
    DafxRenderData ren_info;
    dafx_get_render_info_patched( 0, input.draw_call_id, ren_info );

    ModfxParentRenData parent_data;
    dafx_preload_parent_ren_data( 0, ren_info.parent_ofs, parent_data );

    uint decls = parent_data.decls;
    uint flags = parent_data.flags;

  #if !MODFX_RIBBON_SHAPE
    if ( FLAG_ENABLED( flags, MODFX_RFLAG_USE_UV_ROTATION ) && dot( input.delta, input.delta ) > 1.f)
    {
      discard;
      return fx_null;
    }
  #endif

#if MODFX_TEX_DEBUG_ENABLED
    const bool tex_enabled = false;
  #if !MODFX_RIBBON_SHAPE
    input.color = modfx_pack_hdr( calc_debug_color(input.delta.xy, 1,1,1 ) );
  #endif
#else
    const bool tex_enabled = MOD_ENABLED( parent_data.mods, MODFX_RMOD_FRAME_INFO );
#endif

#if MODFX_RIBBON_SHAPE
    // TODO implement based on modfx_bboard_render implementation.
#else
    input.tc += modfx_render_get_motion_vecs(parent_data, input.tc, input.frame_blend);;

    ModfxDeclWaterFxParams water_params = ModfxDeclWaterFxParams_load( 0, parent_data.mods_offsets[MODFX_RMOD_WATER_FX_PARAMS] );

    float4 src_tex_0 = 1;
    if (tex_enabled)
      src_tex_0 = tex2D( g_tex_0, input.tc.xy );

  #if MODFX_USE_FRAMEBLEND
    if ( tex_enabled && FLAG_ENABLED( flags, MODFX_RFLAG_FRAME_ANIMATED_FLIPBOOK ) )
      src_tex_0 = lerp( src_tex_0, tex2D( g_tex_0, input.tc.zw ), input.frame_blend );
  #endif

    float height_raw = 2.f * (src_tex_0.w - 127.f/255.f);
    if (water_params.use_height_tex_1)
      height_raw = 2.f * (tex2D( g_tex_1, input.tc.xy ).x - 127.f/255.f);

  #if MODFX_USE_FRAMEBLEND
    if ( water_params.use_height_tex_1 && FLAG_ENABLED( flags, MODFX_RFLAG_FRAME_ANIMATED_FLIPBOOK ) )
      height_raw = lerp( height_raw, 2.f*(tex2D( g_tex_1, input.tc.zw ).x - 127.f/255.f), input.frame_blend );
  #endif
#endif

    float height = height_raw * water_params.height_scale;

    float depth_mask = 1;
    float cur_depth = 0;

    src_tex_0 = modfx_render_mul_color_matrix(parent_data, src_tex_0);

    float alpha = calc_alpha(src_tex_0, input, flags);
    float3 color = 0;

#if MODFX_USE_COLOR_REMAP
    float grad_offset = 0;
    float grad_scale_rcp = 1;
    if ( parent_data.mods_offsets[MODFX_RMOD_TEX_COLOR_REMAP_DYNAMIC] )
    {
      grad_scale_rcp = dafx_get_1f( 0, parent_data.mods_offsets[MODFX_RMOD_TEX_COLOR_REMAP_DYNAMIC] );
      grad_offset = input.life_radius.x;
    }

    if ( parent_data.mods_offsets[MODFX_RMOD_TEX_COLOR_REMAP] )
    {
      float3 first = modfx_get_e3dcolor_grad(
        0,
        parent_data.mods_offsets[MODFX_RMOD_TEX_COLOR_REMAP],
        saturate( src_tex_0.r * grad_scale_rcp + grad_offset ) ).rgb;

      if ( FLAG_ENABLED( flags, MODFX_RFLAG_TEX_COLOR_REMAP_APPLY_BASE_COLOR ) )
      {
        first *= input.color.xyz;
      }

      color = first.xyz;

      if ( FLAG_ENABLED( flags, MODFX_RFLAG_TEX_COLOR_REMAP_SECOND_MASK ) )
      {
        float3 second = src_tex_0.ggg;
        if ( FLAG_ENABLED( flags, MODFX_RFLAG_TEX_COLOR_REMAP_SECOND_MASK_APPLY_BASE_COLOR ) )
        {
          second *= input.color.xyz;
        }

        color = lerp(  second.rgb, color, src_tex_0.b );
      }
    }
    else
#endif
    {
      color = src_tex_0.xyz * input.color.xyz;
    }

    if (color_discard_test(float4(color, alpha), flags) && (!water_params.use_height_tex_1 || abs(height_raw) < 1.f/255.f))
    {
      discard;
      return fx_null;
    }

    float4 result_color = float4(color, alpha);

    if (is_blending_premult(flags))
    {
    }
    else if ( FLAG_ENABLED( flags, MODFX_RFLAG_BLEND_ADD ) )
    {
      result_color.xyz *= result_color.w;
      result_color.w = 0;
    }
    else if ( FLAG_ENABLED( flags, MODFX_RFLAG_BLEND_ABLEND ) )
    {
      result_color.xyz *= result_color.w;
    }
    else
    {
      result_color = float4(1,0,1,1);
    }

    float4 result_details = float4(water_params.detail_params, alpha);
    result_details.xyz *= result_details.w;

    float result_height = height * calc_alpha(float4(0,0,0,1), input, flags);

    return encode_output(result_color, result_height, result_details);

#endif // MODFX_DEBUG_RENDER_ENABLED
  }

#endif

#endif
