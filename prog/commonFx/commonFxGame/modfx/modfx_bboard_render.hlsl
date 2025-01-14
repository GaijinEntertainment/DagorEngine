#ifndef DAFX_MODFX_BBOARD_RENDER_HLSL
#define DAFX_MODFX_BBOARD_RENDER_HLSL

#include "modfx/modfx_curve.hlsli"
#include "modfx/modfx_misc.hlsli"
#include "modfx/modfx_part_data.hlsli"
#include "modfx/modfx_render_placement.hlsli"

// #define MODFX_TEX_DEBUG_ENABLED 1


#if MODFX_DEBUG_RENDER_ENABLED
  #undef MODFX_USE_SHADOW
  #define MODFX_USE_SHADOW 0

  #undef DAFX_USE_CLOUD_SHADOWS
  #define DAFX_USE_CLOUD_SHADOWS 0

  #undef MODFX_USE_GI
  #define MODFX_USE_GI 0

  #undef MODFX_USE_FOG
  #define MODFX_USE_FOG 0

  #undef MODFX_USE_FOG_PS_APPLY
  #define MODFX_USE_FOG_PS_APPLY 0

  #undef FX_FOG_MULT_ALPHA
  #define FX_FOG_MULT_ALPHA 0
#endif


#if !APPLY_VOLUMETRIC_FOG
  #undef MODFX_USE_FOG_PS_APPLY
  #define MODFX_USE_FOG_PS_APPLY 0 // no per-pixel fog apply if we don't have volfog at all
#endif

#if MODFX_RIBBON_SHAPE || (MODFX_USE_FOG && !MODFX_USE_FOG_PS_APPLY && !FX_FOG_MULT_ALPHA)
  #define COLOR_INTERPOLATION linear
#else
  #define COLOR_INTERPOLATION nointerpolation
  #if MODFX_USE_FOG && !MODFX_USE_FOG_PS_APPLY && FX_FOG_MULT_ALPHA
    #define COLOR_PS_FOG_MULT 1
  #endif
#endif

#if MODFX_RIBBON_SHAPE
  #define PARAM_INTERPOLATION linear
#else
  #define PARAM_INTERPOLATION nointerpolation
#endif

#if FX_VS || FX_PS

  struct VsOutput
  {
    VS_OUT_POSITION(pos)
    COLOR_INTERPOLATION float4 color  : TEXCOORD0;

#if MODFX_SHADER_VOLSHAPE || MODFX_SHADER_VOLSHAPE_WBOIT
    PARAM_INTERPOLATION float3 life_radius: TEXCOORD2;
#else
    PARAM_INTERPOLATION float life_radius : TEXCOORD2;
#endif

#if MODFX_SHADER_THERMALS
    COLOR_INTERPOLATION float4 emission   : TEXCOORD4;
#else
    COLOR_INTERPOLATION float3 emission   : TEXCOORD4;
#endif

#if MODFX_USE_SHADOW || DAFX_USE_CLOUD_SHADOWS
    float4 lighting   : TEXCOORD7;
#elif MODFX_USE_LIGHTING
    float3 lighting   : TEXCOORD7;
#endif

#if MODFX_USE_GI
    float3 ambient    : TEXCOORD12;
#endif

#if MODFX_RIBBON_SHAPE
    float2 delta_tc   : TEXCOORD1;
    float frame_idx   : TEXCOORD3;
  #if !MODFX_RIBBON_SHAPE_IS_SIDE_ONLY
    uint ribbon_side  : TEXCOORD8;
  #endif
    float3 fwd_dir    : TEXCOORD9;
#else
    float4 tc                         : TEXCOORD1;
    nointerpolation float frame_blend : TEXCOORD3;
    float2 delta                      : TEXCOORD8;
    nointerpolation float3 right_dir  : TEXCOORD10;
    nointerpolation float3 up_dir     : TEXCOORD11;
#endif

#if !MODFX_RIBBON_SHAPE || (MODFX_USE_FOG && MODFX_USE_FOG_PS_APPLY)
    PARAM_INTERPOLATION float3 view_dir   : TEXCOORD13;
#endif

#if MODFX_USE_FOG
  #if MODFX_USE_FOG_PS_APPLY
    PARAM_INTERPOLATION float4 scattering_color__view_dir_dist : TEXCOORD5;
    float4 scattering_base                                     : TEXCOORD14;
  #else
    float3 fog_add    : TEXCOORD5;
    #if FX_FOG_MULT_ALPHA
      float3 fog_mul : TEXCOORD14;
    #endif
  #endif
#endif
    nointerpolation uint draw_call_id : TEXCOORD6;
  };

  float2 rotate_point(float2 pos, float angle_cos, float angle_sin, float inv_scale)
  {
    pos -= 0.5;
    float2 rot_delta;
    rot_delta.x =  angle_cos * pos.x + angle_sin * pos.y;
    rot_delta.y = -angle_sin * pos.x + angle_cos * pos.y;
    rot_delta *= inv_scale;
    rot_delta += 0.5;
    return rot_delta;
  }
  float4 rotate_points(float4 pos, float angle_cos, float angle_sin, float inv_scale)
  {
    pos -= 0.5;
    float4 rot_delta;
    rot_delta.xz =  angle_cos * pos.xz + angle_sin * pos.yw;
    rot_delta.yw = -angle_sin * pos.xz + angle_cos * pos.yw;
    rot_delta *= inv_scale;
    rot_delta += 0.5;
    return rot_delta;
  }

  float2 calc_delta_offset(float frame_idx, ModfxDeclFrameInfo finfo, float2 frames_inv)
  {
    // same as this, but faster // float2 delta_ofs = float2( (uint)frame_idx % finfo.frames_x, (uint)frame_idx / finfo.frames_x ) * frames_inv;
    float2 delta_ofs;
    static const float FRAME_EPS = 1.0/(DAFX_FLIPBOOK_MAX_KEYFRAME_DIM*DAFX_FLIPBOOK_MAX_KEYFRAME_DIM+1); // some GPUs have horrible float precision
    delta_ofs.y = floor((frame_idx + FRAME_EPS) * frames_inv.x);
    delta_ofs.x = frame_idx - delta_ofs.y * finfo.frames_x;
    delta_ofs *= frames_inv;
    return delta_ofs;
  }

  float4 modfx_render_get_frame_tc(
    ModfxParentRenData parent_data,
    uint frame_idx,
    uint frame_flags,
    bool use_uv_rotation,
    float angle_cos,
    float angle_sin,
#if MODFX_RIBBON_SHAPE
    float2 delta_tc,
#else
    inout float2 delta,
#endif
    out float2 scale_tc )
  {
    ModfxDeclFrameInfo finfo = ModfxDeclFrameInfo_load( 0, parent_data.mods_offsets[MODFX_RMOD_FRAME_INFO] );
    float4 tc = 0;

#if !MODFX_RIBBON_SHAPE
  // disabled for ribbons for now (it has a completely different pos/tc calc)
  #if MODFX_USE_FRAMEBOUNDS
    if (dafx_frame_boundary_buffer_enabled && finfo.boundary_id_offset != DAFX_INVALID_BOUNDARY_OFFSET)
    {
      float4 frame_boundary = structuredBufferAt(dafx_frame_boundary_buffer, finfo.boundary_id_offset + frame_idx);

    #if MODFX_USE_FRAMEBLEND
      if ( FLAG_ENABLED( parent_data.flags, MODFX_RFLAG_FRAME_ANIMATED_FLIPBOOK ) )
      {
        uint total_frames = finfo.frames_x * finfo.frames_y;
        uint next_frame_idx = (frame_idx + 1) % total_frames;
        float4 next_frame_boundary = structuredBufferAt(dafx_frame_boundary_buffer, finfo.boundary_id_offset + next_frame_idx);
        frame_boundary = float4(min(frame_boundary.xy, next_frame_boundary.xy), max(frame_boundary.zw, next_frame_boundary.zw));
      }
    #endif

      if ( use_uv_rotation )
      {
        float4 p00_p01 = rotate_points(frame_boundary.xyxw, angle_cos, angle_sin, finfo.inv_scale);
        float4 p10_p11 = rotate_points(frame_boundary.zyzw, angle_cos, angle_sin, finfo.inv_scale);
        float2 minPos = min(min(p00_p01.xy, p00_p01.zw), min(p10_p11.xy, p10_p11.zw));
        float2 maxPos = max(max(p00_p01.xy, p00_p01.zw), max(p10_p11.xy, p10_p11.zw));
        frame_boundary = float4(minPos, maxPos);
      }

      if ( frame_flags & MODFX_FRAME_FLAGS_FLIP_X )
        frame_boundary.xz = 1 - frame_boundary.zx;

      if (frame_flags & MODFX_FRAME_FLAGS_FLIP_Y )
        frame_boundary.yw = 1 - frame_boundary.wy;

      delta = lerp(frame_boundary.xy, frame_boundary.zw, delta);
    }
  #endif

    float2 delta_tc = float2( delta.x, 1.f - delta.y );
#endif

    if ( frame_flags & MODFX_FRAME_FLAGS_FLIP_X )
      delta_tc.x = 1.f - delta_tc.x;
    if ( frame_flags & MODFX_FRAME_FLAGS_FLIP_Y )
      delta_tc.y = 1.f - delta_tc.y;

    // delta_tc *= crop.x;
    // delta_tc += crop.y;

    if ( use_uv_rotation )
      delta_tc = rotate_point(delta_tc, angle_cos, angle_sin, finfo.inv_scale);

    float2 frames_inv = 1.0 / float2(finfo.frames_x, finfo.frames_y);
    delta_tc *= frames_inv;

    scale_tc = delta_tc;

    float2 delta_ofs = calc_delta_offset(frame_idx, finfo, frames_inv);
    tc.xy = delta_tc + delta_ofs;

#if MODFX_USE_FRAMEBLEND
    uint total_frames = finfo.frames_x * finfo.frames_y;
    if ( FLAG_ENABLED( parent_data.flags, MODFX_RFLAG_FRAME_ANIMATED_FLIPBOOK ) )
    {
      uint next_frame_idx = ( frame_idx + 1 ) % total_frames;
      delta_ofs = calc_delta_offset(next_frame_idx, finfo, frames_inv);
      tc.zw = delta_tc + delta_ofs;
    }
#endif
    return tc;
  }

  float4 modfx_render_get_frame_tc_opt(
    ModfxParentRenData parent_data,
    uint frame_idx,
    uint frame_flags,
    bool use_uv_rotation,
    float angle_cos,
    float angle_sin,
    inout float2 delta_tc )
  {
    float2 tmp;
    if ( parent_data.mods_offsets[MODFX_RMOD_FRAME_INFO] )
      return modfx_render_get_frame_tc( parent_data, frame_idx, frame_flags, use_uv_rotation, angle_cos, angle_sin, delta_tc, tmp );
    else
      return 0;
  }
#endif

#if FX_VS

  float3 get_omni_lighting(float3 pos, float3 omni_pos, float omni_radius, float3 omni_color)
  {
    float3 rvec = pos - omni_pos;
    float sqrDist = dot(rvec, rvec);
    float attenF = sqrDist * rcp(pow2(max(omni_radius, 0.01)));
    float atten = pow2(saturate(1. - pow2(attenF))) * rcp(max(sqrDist, 0.00001));

    return atten * omni_color;
  }

#if MODFX_BBOARD_LIGHTING_FROM_CLUSTERED_LIGHTS
  float3 get_omni_lighting_from_clustered_lights(float3 pos, float2 tc, float proj_dist)
  {
    float3 lighting = 0;

    // Find omni lights outside of current modfx
    uint sliceId = min(getSliceAtDepth(proj_dist, depthSliceScale, depthSliceBias), CLUSTERS_D);
    uint clusterIndex = getClusterIndex(tc, sliceId);

    uint omniAddress = clusterIndex*omniLightsWordCount;
    LOOP
    for ( uint omniWordIndex = 0; omniWordIndex < omniLightsWordCount; omniWordIndex++ )
    {
      // Can't use MERGE_MASK in the vertex shader, so just read mask for current vertex
      uint mask = flatBitArray[omniAddress + omniWordIndex];
      while ( mask != 0 )
      {
        uint bitIndex = firstbitlow( mask );
        mask ^= ( 1U << bitIndex );
        uint omni_light_index = ((omniWordIndex<<5) + bitIndex);
        RenderOmniLight light = omni_lights_cb[omni_light_index];
        lighting += get_omni_lighting(pos, light.posRadius.xyz, light.posRadius.a, light.colorFlags.rgb);
      }
    }

    return lighting;
  }
#endif

#if MODFX_USE_SHADOW
    half get_static_shadow_for_fx(float3 pos, float radius, float3 wpos, float3 view_dir_norm, float3 from_sun_direction)
    {
      static const int shadowBroadSampleCnt = 2;

      float3 shadowSampleDirFwd = (pos - wpos) * (1./ shadowBroadSampleCnt);

      float sunHackRadius = radius * (1./ shadowBroadSampleCnt);
      float3 sunHackUp = normalize( cross( from_sun_direction, float3(1,0,0) ) ) * sunHackRadius;
      float3 sunHackRight = normalize(cross( view_dir_norm, sunHackUp )) * sunHackRadius;

      float3 shadowPos = wpos + shadowSampleDirFwd * 0.5;
      half staticShadowSum = 0;
      UNROLL for (int i = 0; i < shadowBroadSampleCnt; ++i)
      {
        staticShadowSum += getStaticShadow(shadowPos - sunHackUp - sunHackRight);
        staticShadowSum += getStaticShadow(shadowPos + sunHackUp - sunHackRight);
        staticShadowSum += getStaticShadow(shadowPos + sunHackUp + sunHackRight);
        staticShadowSum += getStaticShadow(shadowPos - sunHackUp + sunHackRight);
        shadowPos += shadowSampleDirFwd;
      }
      return staticShadowSum * (0.25 / shadowBroadSampleCnt);
    }
#endif

  float get_placement_height(float3 pos, float placement_threshold, uint flags)
  {
    place_fx_above(pos, placement_threshold, flags);
    return pos.y;
  }


  VsOutput dafx_bboard_vs( uint vertex_id : SV_VertexID, uint draw_call_id : TEXCOORD0 HW_USE_INSTANCE_ID )
  {
    GlobalData gdata = global_data_load();
    DafxRenderData ren_info;
    dafx_get_render_info( instance_id, vertex_id, draw_call_id, ren_info );

    ModfxParentRenData parent_data;
    dafx_preload_parent_ren_data( 0, ren_info.parent_ofs, parent_data );

    uint decls = parent_data.decls;
    uint flags = parent_data.flags;

    bool reverse_order = FLAG_ENABLED( flags, MODFX_RFLAG_REVERSE_ORDER );
    uint data_ofs = dafx_get_render_data_offset( ren_info, !reverse_order );

    bool is_dead = false;

#if MODFX_RIBBON_SHAPE

    #if MODFX_RIBBON_SHAPE_IS_SIDE_ONLY
      bool ribbon_side = 1;
    #else
      bool ribbon_side = vertex_id < 4;
    #endif
    bool ribbon_start = vertex_id < 2;
    float3 older_pos = 0;
    float3 newer_pos = 0;

    if ( ren_info.instance_id == 0 )
      is_dead = true;

    if ( !ribbon_side )
      vertex_id -= 4;

    if ( ribbon_side )
    {
      ren_info.instance_id = max( (int)ren_info.instance_id - 1, 0 );
      uint prev_data_ofs = dafx_get_render_data_offset( ren_info, !reverse_order );
      uint other_side_ofs = prev_data_ofs;
      if (!ribbon_start) // tail should 100% mimic older part head
      {
        other_side_ofs = data_ofs;
        data_ofs = prev_data_ofs;
      }
      else
      {
        ren_info.instance_id = instance_id;
      }

      if ( MODFX_RDECL_POS_ENABLED( decls ) )
        other_side_ofs += 3;

      if ( MODFX_RDECL_POS_OFFSET_ENABLED( decls ) )
        other_side_ofs += 3;

      if (dafx_get_1f(0, other_side_ofs) <= 0.f) //other side radius is <0 (dead part)
        is_dead = true;
    }

    uint ren_instance_id = ren_info.instance_id;

    ren_info.instance_id = min( ren_instance_id + 1, ren_info.count - 1 );
    uint sample_data_ofs_newer = dafx_get_render_data_offset( ren_info, !reverse_order );

    ren_info.instance_id = max( (int)ren_instance_id - 1, 0 );
    uint sample_data_ofs_older = dafx_get_render_data_offset( ren_info, !reverse_order );

    if ( MODFX_RDECL_POS_ENABLED( decls ) )
    {
      newer_pos = dafx_load_3f( 0, sample_data_ofs_newer );
      older_pos = dafx_load_3f( 0, sample_data_ofs_older );
    }

    if ( MODFX_RDECL_POS_OFFSET_ENABLED( decls ) )
    {
      newer_pos += dafx_load_3f( 0, sample_data_ofs_newer );
      older_pos += dafx_load_3f( 0, sample_data_ofs_older );
    }

    ren_info.instance_id = ren_instance_id;

#endif

    // note: don't use construction like: v = MODFX_RDECL_ENALED( decls ) ? dafx_load_1f(cur_ofs) : 0
    // hlsl optimiizer is going haywire and exec both result, shifting cur_ofs even if condition is false

    ModfxRenData rdata;
    modfx_load_ren_data(0, data_ofs, decls, rdata);
    rdata.pos += rdata.pos_offset;
    if (rdata.radius <= 0.f)
      is_dead = true;

    float angle_sin = 0.f;
    float angle_cos = 1.f;
    sincos( -rdata.angle, angle_sin, angle_cos );

    float4 tc = float4( 0, 0, 0, 0 );
    float2 delta = float2( vertex_id % 3 != 0, (vertex_id / 2) );

    // hlsl error X4580 workaround
    float4x4 wtm =
      { 1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1 };
    if ( parent_data.mods_offsets[MODFX_RMOD_INIT_TM] )
      wtm = dafx_get_44mat( 0, parent_data.mods_offsets[MODFX_RMOD_INIT_TM] );

    bool apply_wtm = FLAG_ENABLED( flags, MODFX_RFLAG_USE_ETM_AS_WTM );
    if ( apply_wtm )
    {
      rdata.pos = mul( float4( rdata.pos, 1 ), wtm ).xyz;
#if MODFX_RIBBON_SHAPE
      older_pos = mul( float4( older_pos, 1 ), wtm ).xyz;
      newer_pos = mul( float4( newer_pos, 1 ), wtm ).xyz;
#endif
    }

    float placementNormalOffset = -1;
    BRANCH
    if ( parent_data.mods_offsets[MODFX_RMOD_RENDER_PLACEMENT_PARAMS] )
    {
      ModfxDeclRenderPlacementParams placementParams = ModfxDeclRenderPlacementParams_load(0, parent_data.mods_offsets[MODFX_RMOD_RENDER_PLACEMENT_PARAMS]);
      if (!place_fx_above(rdata.pos, placementParams.placement_threshold, placementParams.flags))
        is_dead = true;
      else
        placementNormalOffset = placementParams.align_normals_offset;
    }

    float3 view_dir = gdata.world_view_pos - rdata.pos;
    float view_dir_dist = length( view_dir );
    float3 view_dir_norm = view_dir * (view_dir_dist > 0.01 ? rcp( view_dir_dist ) : 0);
    float proj_dist = view_dir_dist * dot(-view_dir_norm, gdata.view_dir_z);

    float3 emission_mask =  0;
    if ( parent_data.mods_offsets[MODFX_RMOD_COLOR_EMISSION] )
    {
      ModfxColorEmission pp = ModfxColorEmission_load( 0, parent_data.mods_offsets[MODFX_RMOD_COLOR_EMISSION] );
      emission_mask = unpack_e3dcolor_to_n4f( pp.mask ).xyz * pp.value * rdata.emission_fade;
    }
#if MODFX_SHADER_THERMALS
    float thermal_value = 0;
    if ( parent_data.mods_offsets[MODFX_RMOD_THERMAL_EMISSION] )
    {
      thermal_value = dafx_load_1f( 0, parent_data.mods_offsets[MODFX_RMOD_THERMAL_EMISSION] );
      if ( parent_data.mods_offsets[MODFX_RMOD_THERMAL_EMISSION_FADE] )
        thermal_value *= modfx_get_1f_curve( 0, parent_data.mods_offsets[MODFX_RMOD_THERMAL_EMISSION_FADE], rdata.life_norm);
    }
#endif

#if !(MODFX_SHADER_VOLSHAPE || MODFX_SHADER_VOLSHAPE_WBOIT) && !MODFX_SHADER_VOLFOG_INJECTION
    if ( parent_data.mods_offsets[MODFX_RMOD_DEPTH_MASK] && !is_dead )
    {
      ModfxDepthMask pp = ModfxDepthMask_load( 0, parent_data.mods_offsets[MODFX_RMOD_DEPTH_MASK] );
      float znear_opacity = ( view_dir_dist - gdata.znear_offset - pp.znear_clip_offset ) * pp.znear_softness_rcp;
      if ( FLAG_ENABLED( flags, MODFX_RFLAG_DEPTH_MASK_USE_PART_RADIUS ) )
        znear_opacity *= rcp(rdata.radius);
      znear_opacity = saturate(znear_opacity);

      rdata.color.a *= znear_opacity;
      if ( FLAG_ENABLED( flags, MODFX_RFLAG_BLEND_PREMULT ) )
        rdata.color.rgb *= znear_opacity;

      if ( znear_opacity < 0.0001 )
        is_dead = true;
    }
#endif

#if MODFX_RIBBON_SHAPE
    float2 delta_tc = float2( delta.x, 1.f - delta.y );

    float3 wpos;
    float camera_offset = 0;

    float3 rotated_right_vec;
    float3 rotated_up_vec;

    if ( !FLAG_ENABLED( flags, MODFX_RFLAG_FRAME_ANIMATED_FLIPBOOK ) )
      rdata.frame_blend = 0;

    ModfxDeclRibbonParams pp = ModfxDeclRibbonParams_load( 0, parent_data.mods_offsets[MODFX_RMOD_RIBBON_PARAMS] );

    float2 ribbon_fade_params = ribbon_side ? pp.side_fade_params : pp.head_fade_params;

    float fade = abs( dot( normalize( older_pos - ( ren_info.instance_id ? rdata.pos : newer_pos ) ), view_dir_norm ) );
    if ( ribbon_side )
      fade = 1.f - fade;

    fade = saturate( pow( fade * ribbon_fade_params.x, ribbon_fade_params.y ) );

    if ( ribbon_side )
    {
      rotated_up_vec = gdata.view_dir_y;
      rotated_right_vec = gdata.view_dir_x;

      // float3 med_dir = normalize( normalize( rdata.pos - newer_pos ) + normalize( older_pos - rdata.pos ) );
      float3 med_dir = normalize( older_pos - newer_pos );
      float3 adj_dir = normalize( cross( med_dir, view_dir_norm ) );

      if ( vertex_id == 0 || vertex_id == 3 )
        adj_dir = -adj_dir;

      wpos = rdata.pos + adj_dir * rdata.radius;
    }
    else
    {
      float3 up_vec = gdata.view_dir_y;
      float3 right_vec = gdata.view_dir_x;

      rotated_right_vec = angle_cos * right_vec - angle_sin * up_vec;
      rotated_up_vec = angle_sin * right_vec + angle_cos * up_vec;

      float3 worldDelta = 2 * ((delta.x - 0.5) * rotated_right_vec + (delta.y - 0.5) * rotated_up_vec);
      wpos = rdata.pos + worldDelta * rdata.radius;

      if ( fade < 0.01f )
        is_dead = true;
    }

    // keep it, it is helpful for debugging
    // if ( vertex_id == 0 )
    //   rdata.color = float4( 0, 0, 0, 1 );
    // else if ( vertex_id == 1 )
    //   rdata.color = float4( 1, 0, 0, 1 );
    // else if ( vertex_id == 2 )
    //   rdata.color = float4( 0, 1, 0, 1 );
    // else
    //   rdata.color = float4( 0, 0, 1, 1 );

    if ( ren_info.instance_id < 1 && !ribbon_start )
      fade = 0;

    rdata.color.a *= fade;
    if ( FLAG_ENABLED( flags, MODFX_RFLAG_BLEND_PREMULT ) )
      rdata.color.rgb *= fade;

#else

    bool use_uv_rotation = FLAG_ENABLED( flags, MODFX_RFLAG_USE_UV_ROTATION );

    tc = modfx_render_get_frame_tc_opt(
      parent_data, rdata.frame_idx, rdata.frame_flags, use_uv_rotation, angle_cos, angle_sin, delta );
    float2 aspect = 1;
    if ( parent_data.mods_offsets[MODFX_RMOD_CUSTOM_ASPECT] )
      aspect = dafx_get_2f( 0, parent_data.mods_offsets[MODFX_RMOD_CUSTOM_ASPECT] );

    float camera_offset = 0;
    if ( parent_data.mods_offsets[MODFX_RMOD_CAMERA_OFFSET] )
      camera_offset = dafx_get_1f( 0, parent_data.mods_offsets[MODFX_RMOD_CAMERA_OFFSET] );

    float2 pivot_offset = 0;
    if ( parent_data.mods_offsets[MODFX_RMOD_PIVOT_OFFSET] )
    {
      uint4 pp = unpack_4b( dafx_get_1ui( 0, parent_data.mods_offsets[MODFX_RMOD_PIVOT_OFFSET] ) );
      pivot_offset.x = pp[0] * ( 1.f / 254 ) - 0.5f;
      pivot_offset.y = pp[1] * ( 1.f / 254 ) - 0.5f;
    }

    // MODFX_RMOD_SHAPE_SCREEN
    float3 up_vec = gdata.view_dir_y;
    float3 right_vec = gdata.view_dir_x;

#if (MODFX_SHADER_VOLSHAPE || MODFX_SHADER_VOLSHAPE_WBOIT)
    float stable_rnd_offset = (ren_info.instance_id + ren_info.data_ofs % 20 ) * 0.001;
    rdata.pos += gdata.view_dir_z * stable_rnd_offset;

    float proj_dist_abs = abs(proj_dist);
    float min_offset = gdata.zn_zfar.x * 2.f;
    float wboit_offset = 0;
    float z_offset = min(rdata.radius, max(proj_dist, 0));
    wboit_offset -= z_offset;
    if (proj_dist < min_offset && proj_dist_abs < rdata.radius )
    {
      wboit_offset += proj_dist_abs + min_offset + stable_rnd_offset;
      rdata.pos += gdata.view_dir_z * wboit_offset;
      view_dir_norm = -view_dir_norm;
    }

    float k = saturate(proj_dist / (rdata.radius * 2.f));
    float3 cross_up = normalize(cross(gdata.view_dir_x, view_dir_norm));
    up_vec = normalize(lerp(gdata.view_dir_y, cross_up, k));
    right_vec = normalize(lerp(gdata.view_dir_x, cross(view_dir_norm, cross_up), k));
#else
    bool apply_wtm_to_view = false;
    if ( MOD_ENABLED( parent_data.mods, MODFX_RMOD_SHAPE_VIEW_POS ) )
    {
      up_vec = normalize( cross( gdata.view_dir_x, view_dir_norm ) );
      right_vec = cross( view_dir_norm, up_vec );
    }
    else if ( parent_data.mods_offsets[MODFX_RMOD_SHAPE_STATIC_ALIGNED] )
    {
      apply_wtm_to_view = apply_wtm;

      up_vec = rdata.up_vec;
      right_vec = rdata.right_vec;
    }
    else if ( MOD_ENABLED( parent_data.mods, MODFX_RMOD_SHAPE_VELOCITY_MOTION ) )
    {
      apply_wtm_to_view = apply_wtm;
      up_vec = rdata.up_vec;
      right_vec = normalize( cross( view_dir_norm, up_vec ) );
      float vl = 1.f + rdata.velocity_length;
      aspect.x /= vl;
      rdata.radius *= vl;
    }
    else if ( MOD_ENABLED( parent_data.mods, MODFX_RMOD_SHAPE_ADAPTIVE_ALIGNED ) )
    {
      apply_wtm_to_view = apply_wtm;
      up_vec = rdata.up_vec;
      right_vec = normalize( cross( gdata.view_dir_y, up_vec ) );
    }

    if ( apply_wtm_to_view )
    {
      up_vec = mul( float4( up_vec, 0 ), wtm ).xyz;
      right_vec = mul( float4( right_vec, 0 ), wtm ).xyz;
    }

    if ( parent_data.mods_offsets[MODFX_RMOD_SHAPE_STATIC_ALIGNED] )
    {
      float wtm_scale = length( wtm[0] ); // TODO: move to CPP?

      if ( !apply_wtm_to_view ) // up/right vec are normalized
      {
        up_vec *= wtm_scale;
        right_vec *= wtm_scale;
      }

      if ( !any( up_vec ) )
        up_vec = normalize( cross( view_dir_norm, right_vec ) ) * wtm_scale;

      if ( !any( right_vec ) )
        right_vec = normalize( cross( view_dir_norm, up_vec ) ) * wtm_scale;

      ModfxDeclShapeStaticAligned pp = ModfxDeclShapeStaticAligned_load( 0, parent_data.mods_offsets[MODFX_RMOD_SHAPE_STATIC_ALIGNED] );

      float cross_k = abs( dot( normalize( cross( up_vec, right_vec ) ), view_dir_norm ) );
      cross_k = saturate( cross_k - pp.cross_fade_threshold ) * rcp( 1.f - pp.cross_fade_threshold );
      float a = 1.f - pow( abs( ( 1.f - cross_k ) * pp.cross_fade_mul ), pp.cross_fade_pow );

  #if !MODFX_SHADER_VOLFOG_INJECTION
      rdata.color.w *= a;

      if ( FLAG_ENABLED( flags, MODFX_RFLAG_BLEND_PREMULT ) )
        rdata.color.rgb *= a;
  #endif
    }
#endif

    float3 rotated_right_vec;
    float3 rotated_up_vec;
    if ( use_uv_rotation )
    {
      rotated_right_vec = right_vec;
      rotated_up_vec = up_vec;
    }
    else
    {
      rotated_right_vec = angle_cos * right_vec - angle_sin * up_vec;
      rotated_up_vec = angle_sin * right_vec + angle_cos * up_vec;
    }

    BRANCH
    if ( placementNormalOffset > 0 )
    {
      ModfxDeclRenderPlacementParams placementParams = ModfxDeclRenderPlacementParams_load(0, parent_data.mods_offsets[MODFX_RMOD_RENDER_PLACEMENT_PARAMS]);

      half W = get_placement_height(rdata.pos + float3(-placementNormalOffset, 0, 0), placementParams.placement_threshold, placementParams.flags);
      half E = get_placement_height(rdata.pos + float3(+placementNormalOffset, 0, 0), placementParams.placement_threshold, placementParams.flags);
      half N = get_placement_height(rdata.pos + float3(0, 0, -placementNormalOffset), placementParams.placement_threshold, placementParams.flags);
      half S = get_placement_height(rdata.pos + float3(0, 0, +placementNormalOffset), placementParams.placement_threshold, placementParams.flags);
      float3 placementWorldNormal = float3(W-E, 2*placementNormalOffset, N-S); // no need to normalize it on its own

      // particle "forward direction" (parallel to velocity dir) is up_vec, so we need that to align with the projected world normal
      rotated_up_vec = normalize( cross( view_dir_norm, placementWorldNormal ) );
      rotated_right_vec = normalize( cross( view_dir_norm, rotated_up_vec ) );
    }

#if !(MODFX_SHADER_VOLSHAPE || MODFX_SHADER_VOLSHAPE_WBOIT)
    if ( MOD_ENABLED( parent_data.mods, MODFX_RMOD_SHAPE_ADAPTIVE_ALIGNED ) )
    {
      float3 r = normalize( cross( view_dir_norm, rotated_up_vec ) );
      float3 u = cross( r, view_dir_norm );

      float k = abs( dot( rotated_up_vec, view_dir_norm ) );
      float2 a2 = aspect;
      if ( aspect.x < aspect.y ) // capsule
      {
        float rk = sqrt( 1.f - pow2( k ) ); // projection
        float rad_src = rdata.radius;
        rdata.radius *= lerp( a2.x, 1, rk );
        float ak = rad_src / rdata.radius; // aspect ratio
        a2.x *= ak;
      }

      if ( aspect.y < aspect.x ) // ellipsoid
        a2.y = lerp( aspect.y, 1, k );

      aspect = a2;
      rotated_up_vec = u;
      rotated_right_vec = r;
    }
#endif

    float3 worldDelta =
      2 * ((delta.x - 0.5 + pivot_offset.x) * rotated_right_vec * aspect.x
      + (delta.y - 0.5 + pivot_offset.y) * rotated_up_vec * aspect.y);

    float3 wpos = rdata.pos + worldDelta * rdata.radius;

#endif

    VsOutput vs_out;

    wpos += camera_offset * view_dir_norm;
    vs_out.pos = mul( float4( wpos, 1.f ), gdata.globtm );

    float3 lighting = 0;
    bool isModfxOmniLightEnabled = FLAG_ENABLED( flags, MODFX_RFLAG_OMNI_LIGHT_ENABLED );
    if ( isModfxOmniLightEnabled )
    {
      ModfxDeclExternalOmnilight pp = ModfxDeclExternalOmnilight_load( 0, parent_data.mods_offsets[MODFX_RMOD_OMNI_LIGHT_INIT] );
      lighting = get_omni_lighting(wpos.xyz, pp.pos, pp.radius, pp.color);
    }

#if MODFX_USE_GI
    if ( FLAG_ENABLED( flags, MODFX_RFLAG_LIGHTING_AMBIENT_ENABLED ) )
      vs_out.ambient.rgb = ambient_calculating(wpos, gdata.world_view_pos, gdata.sky_color, vs_out.pos);
    else
      vs_out.ambient.rgb = gdata.sky_color;
#endif
#if MODFX_RAIN
    BRANCH
    if (!is_dead)
      is_dead = cull_away_rain(rdata.pos - float3(0, rdata.radius, 0)); //Always test with bottom
#endif

#if MODFX_BBOARD_LIGHTING_FROM_CLUSTERED_LIGHTS
    BRANCH
    if (FLAG_ENABLED( flags, MODFX_RFLAG_EXTERNAL_LIGHTS_ENABLED ) && !isModfxOmniLightEnabled)
    {
      // Find omni lights outside of current modfx
      const float2 tc = (vs_out.pos.xy / vs_out.pos.w) * float2(0.5, -0.5) + 0.5;
      lighting += get_omni_lighting_from_clustered_lights(wpos.xyz, tc, proj_dist);
    }
#endif

 #if !MODFX_RIBBON_SHAPE

    float2 screen_clamp = 0;
    if ( parent_data.mods_offsets[MODFX_RMOD_SCREEN_CLAMP] )
    {
      screen_clamp = dafx_get_2f( 0, parent_data.mods_offsets[MODFX_RMOD_SCREEN_CLAMP] );
      float4 centeredPos = float4(rdata.pos + camera_offset * view_dir_norm, 1.0f);
      centeredPos =  mul( float4( centeredPos), gdata.globtm );
      float2 delta = centeredPos.xy / centeredPos.w - vs_out.pos.xy / vs_out.pos.w;
      float2 delta_abs = abs(delta.xy);
      float2 delta_mod = max(delta_abs, screen_clamp.xx);
      delta_mod = min(delta_mod, screen_clamp.y > 0 ? screen_clamp.yy : delta_mod);
      delta_mod.xy *= sign(delta.xy);
      vs_out.pos.xy = centeredPos.xy - delta_mod.xy * vs_out.pos.w;
    }
#endif
    vs_out.color = rdata.color;
    vs_out.emission.rgb = emission_mask;
#if MODFX_USE_SHADOW || MODFX_USE_LIGHTING || DAFX_USE_CLOUD_SHADOWS
    #if MODFX_USE_SHADOW || DAFX_USE_CLOUD_SHADOWS
      vs_out.lighting.a = 1.f;
    #endif
    #if MODFX_USE_SHADOW
    //this is a way to reduce flickering due to tesselation insufficiency
    //we sample shadow closer to center
    float3 shadowWorldPos = lerp(rdata.pos, wpos, rcp(2 * rdata.radius + 1.f));//the bigger radius is the closer we will be to center worldPos
    half shadow = get_static_shadow_for_fx(rdata.pos, rdata.radius, wpos, view_dir_norm, gdata.from_sun_direction.xyz) *
      getFOMShadow(wpos - gdata.from_sun_direction.xyz * rdata.radius) * clouds_shadow(shadowWorldPos);
    vs_out.lighting.a = shadow;
    #endif
    #if DAFX_USE_CLOUD_SHADOWS
      vs_out.lighting.a *= dafx_get_clouds_shadow(rdata.pos);
    #endif

    vs_out.lighting.rgb = lighting.rgb;
#endif
#if MODFX_RIBBON_SHAPE
    if ( ribbon_side )
    {
      int ribbon_id = instance_id;
      if (FLAG_ENABLED( flags, MODFX_RFLAG_RIBBON_UV_STATIC ))
      {
        uint start_unique_id = rdata.unique_id + (!ribbon_start ? ( !reverse_order ? +1 : -1 ) : 0);
        ribbon_id = !reverse_order ? start_unique_id : -start_unique_id;
      }
      float uv_tile_inv = rcp( (float)pp.uv_tile );
      delta_tc.y *= uv_tile_inv;
      delta_tc.y += ( ribbon_id % pp.uv_tile )* uv_tile_inv;
    }
    vs_out.delta_tc.xy = delta_tc;
    vs_out.frame_idx = (float)rdata.frame_idx + max( rdata.frame_blend, 0.0001f );
    #if !MODFX_RIBBON_SHAPE_IS_SIDE_ONLY
      vs_out.ribbon_side = ribbon_side;
    #endif
    vs_out.fwd_dir = cross( rotated_up_vec, rotated_right_vec );
#else
    vs_out.frame_blend = rdata.frame_blend;
    vs_out.delta = float2( delta.x - 0.5, -delta.y + 0.5 ) * 2.f;
    vs_out.right_dir = rotated_right_vec;
    vs_out.up_dir = rotated_up_vec;
    vs_out.view_dir = view_dir_norm;
    vs_out.tc = tc;
#endif

    vs_out.life_radius.x = rdata.life_norm;

#if MODFX_SHADER_VOLSHAPE || MODFX_SHADER_VOLSHAPE_WBOIT
    vs_out.life_radius.y = rdata.radius;
    vs_out.life_radius.z = wboit_offset;

    wpos += view_dir_norm * z_offset;
    float2 zw = mul(float4( wpos, 1.f), gdata.globtm).zw;
    vs_out.pos.z = zw.x * vs_out.pos.w / max(zw.y, 1);
#else
#endif

#if MODFX_USE_FOG
    float stc_raw_depth = vs_out.pos.w > 0 ? vs_out.pos.z / vs_out.pos.w : 0;
    float2 stc = vs_out.pos.w > 0 ? vs_out.pos.xy * RT_SCALE_HALF / vs_out.pos.w + float2(0.5, 0.5) : 0;
    stc = reproject_scattering(stc, stc_raw_depth);
    #if MODFX_USE_FOG_PS_APPLY
      float tcZ;
      vs_out.scattering_base = get_scattering_tc_scatter_loss(stc, view_dir_norm, view_dir_dist, tcZ);
      vs_out.scattering_color__view_dir_dist = float4(
        get_fog_prepared_tc(getPreparedScatteringTc(view_dir_norm.y, view_dir_dist)),
        view_dir_dist);
    #else
      half3 fog_mul, fog_add;
      get_volfog_with_scattering(stc, stc, view_dir_norm, view_dir_dist, vs_out.pos.w, fog_mul, fog_add);
      #if DAFX_USE_UNDERWATER_FOG
        modify_underwater_fog(view_dir_dist, fog_mul, fog_add);
      #endif
      #if DAFX_USE_UNDERWATER_DEPTH_LIGHTING_MOD
        modify_underwater_lighting(gdata.water_level, rdata.pos.y, vs_out.color.rgb);
      #endif
      #if MODFX_SHADER_THERMALS
        fog_modify_thermal(fog_mul, fog_add);
      #endif
      vs_out.fog_add = fog_add;
      #if FX_FOG_MULT_ALPHA // then COLOR_PS_FOG_MULT is defined
        vs_out.fog_mul = float3(fog_mul);
        #if MODFX_USE_SHADOW || MODFX_USE_LIGHTING || DAFX_USE_CLOUD_SHADOWS
        vs_out.lighting.rgb *= fog_mul;
        #endif
      #else
        vs_out.color.rgb *= fog_mul;
        vs_out.emission.rgb *= fog_mul;
      #endif
    #endif
#endif
#if MODFX_SHADER_THERMALS
    vs_out.emission.w = thermal_value;
#endif

    if ( is_dead )
      vs_out.pos = float4(-2,-2,1,1);

    if( FLAG_ENABLED( flags, MODFX_RFLAG_GAMMA_CORRECTION ) )
      vs_out.color.rgb = pow2(vs_out.color.rgb);

    vs_out.draw_call_id = draw_call_id;

    return vs_out;
  }

#endif

#if FX_PS

  float3 modfx_pack_hdr( float3 v )
  {
  #if MODFX_USE_PACK_HDR
    return pack_hdr( v.xyz );
  #else
    return v;
  #endif
  }

#if MODFX_WBOIT_ENABLED
    struct WboitData
    {
      float4 color : SV_Target0;
      float alpha : SV_Target1;
    };

  float wboit_weight(float z, float a)
  {
    z += modfx_wboit_depth_offset;
    // https://jcgt.org/published/0002/02/09/paper.pdf
    //return a * max(pow(10.f, -2.f), min(3*pow(10.f, 3.f), 10.f / (pow(10.f, -5.f) + pow(abs(z)/5.f, 2.f) + pow(abs(z)/200.f, 6.f))));
    return a * max(0.00001, min(1000.f, 10.f / (1e-05 + pow((z)/5.f, 2.f) + pow((z)/200.f, 6.f))));
  }
#endif


  float3 calc_debug_color(float2 delta_xy, float delta_scale, float border_scale, float center_scale)
  {
    float2 dd = delta_xy.xy * 0.5 + 0.5;

    float border = 0;
    float tt = 0.05;
    if ( dd.x < tt || dd.x > ( 1 - tt ) )
      border = 1;

    if ( dd.y < tt || dd.y > ( 1 - tt ) )
      border = 1;

    float center = 0;
    if ( dot( delta_xy, delta_xy ) < 0.3 )
      center = 1;

    dd *= delta_scale;
    border *= border_scale;
    center *= center_scale;

    float3 color;
    color.r = ( center > 0 ? 1 : 0 );
    color.gb = ( center > 0 ? 0 : 1 ) * dd;
    color.rgb += border;
    color = saturate( color );
    return color;
  }

  float3 calc_debug_color(float2 delta_xy)
  {
    // default params for debug rendering
    return calc_debug_color(delta_xy, 0.1, 0.2, 0.3);
  }

#if !MODFX_SHADER_ATEST && !_HARDWARE_METALIOS
  #define USE_EARLY_DEPTH_STENCIL [earlydepthstencil]
#else
  #define USE_EARLY_DEPTH_STENCIL
#endif

struct PsOutput
{
  float4 color : SV_Target0;
#if DAFXEX_USE_REACTIVE_MASK
  float reactive : SV_Target1;
#endif
#if MODFX_USE_DEPTH_OUTPUT
  float depth : SV_Target1;
#endif
};
PsOutput encode_output(float4 color, float depth)
{
  PsOutput output;
  output.color = color;
#if DAFXEX_USE_REACTIVE_MASK
  output.reactive = color.a;
#endif
#if MODFX_USE_DEPTH_OUTPUT
  output.depth = depth;
#endif
  return output;
}

#if MODFX_SHADER_VOLFOG_INJECTION
  #define fx_null
#elif MODFX_SHADER_FOM
  #define fx_null fx_fom_null()
#elif MODFX_SHADER_VOLSHAPE_WBOIT
  #define fx_null (WboitData)0
#else
  #define fx_null encode_output(0, 0)
#endif

bool color_discard_test(float4 src, uint flags)
{
  // default = premult
  float src_alpha = src.a;
  float src_color_sum = dot(src.rgb, 1.f);
#if MODFX_SHADER_DISTORTION
  src_color_sum = abs(src_color_sum);
#endif
  if ( FLAG_ENABLED( flags, MODFX_RFLAG_BLEND_ADD ) )
    src_alpha = 0;
  else if ( FLAG_ENABLED( flags, MODFX_RFLAG_BLEND_ABLEND ) )
    src_color_sum = 0;

  return (src_alpha + src_color_sum) < 1.f / 255.f;
}

#if MODFX_SHADER_VOLFOG_INJECTION
  USE_EARLY_DEPTH_STENCIL void dafx_bboard_ps(VsOutput input HW_USE_SCREEN_POS)
#elif MODFX_SHADER_FOM
  USE_EARLY_DEPTH_STENCIL FOM_DATA dafx_bboard_ps( VsOutput input HW_USE_SCREEN_POS)
#elif MODFX_WBOIT_ENABLED
  USE_EARLY_DEPTH_STENCIL WboitData dafx_bboard_ps( VsOutput input HW_USE_SCREEN_POS)
#else
  USE_EARLY_DEPTH_STENCIL PsOutput dafx_bboard_ps(VsOutput input HW_USE_SCREEN_POS)
#endif
  {

#if MODFX_DEBUG_RENDER_ENABLED
    PsOutput output = encode_output(0, 0);
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

#if COLOR_PS_FOG_MULT && !MODFX_SHADER_VOLFOG_INJECTION
    input.color.rgb *= input.fog_mul;
    input.color.a *= dot(input.fog_mul, 1.0/3);
    input.emission.rgb *= input.fog_mul;
#endif

#if !MODFX_RIBBON_SHAPE

#if MODFX_TEX_DEBUG_ENABLED
    input.color = modfx_pack_hdr( calc_debug_color(input.delta.xy, 1,1,1 ) );
#endif
#endif

#if MODFX_TEX_DEBUG_ENABLED
    const bool tex_enabled = false;
#else
    const bool tex_enabled = MOD_ENABLED( parent_data.mods, MODFX_RMOD_FRAME_INFO );
#endif

#if MODFX_RIBBON_SHAPE

    #if MODFX_RIBBON_SHAPE_IS_SIDE_ONLY
      bool ribbon_side = 1;
    #else
      bool ribbon_side = input.ribbon_side;
    #endif
    uint frame_idx = input.frame_idx;
    uint frame_flags = 0;

    float2 scale_tc;
    float4 tc = modfx_render_get_frame_tc( parent_data, frame_idx, frame_flags, false, 1, 0, input.delta_tc, scale_tc );

    float2 grad_x = ddx( scale_tc );
    float2 grad_y = ddy( scale_tc );

    float4 src_tex_00 = 1;
    if (tex_enabled)
    {
      if (ribbon_side)
        src_tex_00 = tex2Dgrad( g_tex_0, tc.xy, grad_x, grad_y );
      else
        src_tex_00 = tex2D( g_tex_1, tc.xy );
    }

    float4 src_tex_0 = src_tex_00;
  #if MODFX_USE_FRAMEBLEND
    if ( FLAG_ENABLED( flags, MODFX_RFLAG_FRAME_ANIMATED_FLIPBOOK ) )
    {
      float4 src_tex_01 = 1;
      if (tex_enabled)
      {
        if (ribbon_side)
          src_tex_01 = tex2Dgrad( g_tex_0, tc.zw, grad_x, grad_y );
        else
          src_tex_01 = tex2D( g_tex_1, tc.zw );
      }
      src_tex_0 = lerp( src_tex_00, src_tex_01, frac( input.frame_idx ) );
    }
  #endif
#else
    float4 src_tex_0 = 1;
    if (tex_enabled)
      src_tex_0 = tex2D( g_tex_0, input.tc.xy );

#if MODFX_USE_FRAMEBLEND && !MODFX_SHADER_VOLSHAPE_WBOIT_APPLY
    if ( tex_enabled && FLAG_ENABLED( flags, MODFX_RFLAG_FRAME_ANIMATED_FLIPBOOK ) )
      src_tex_0 = lerp( src_tex_0, tex2D( g_tex_0, input.tc.zw ), input.frame_blend );
#endif

  uint any_color_remap = 0;
  bool shouldDiscard = false;
#if !(MODFX_SHADER_VOLSHAPE || MODFX_SHADER_VOLSHAPE_WBOIT || MODFX_SHADER_VOLFOG_INJECTION)
#if MODFX_USE_COLOR_MATRIX
  any_color_remap |= (parent_data.mods_offsets[MODFX_RMOD_TEX_COLOR_MATRIX] != 0) ? 1 : 0;
#endif
#if MODFX_USE_COLOR_REMAP
  any_color_remap |= (parent_data.mods_offsets[MODFX_RMOD_TEX_COLOR_REMAP] != 0) ? 1 : 0;
#endif

  BRANCH
  if (!any_color_remap)
    shouldDiscard = color_discard_test(src_tex_0, flags);

#endif

#if !MODFX_SHADER_VOLSHAPE_WBOIT_APPLY
    if ( FLAG_ENABLED( flags, MODFX_RFLAG_USE_UV_ROTATION ) )
    {
      if ( dot( input.delta, input.delta ) > 1.f )
        shouldDiscard = true;
    }
#endif

  if (shouldDiscard)
  {
    discard;
    return fx_null;
  }

#endif

#if MODFX_SHADER_DISTORTION
    src_tex_0.xy -= float2( 127.f / 255.f, 127.f / 255.f );
#endif

    float depth_mask = 1;
    float cur_depth = 0;

    float4 pixel_pos = GET_SCREEN_POS(input.pos);
    float4 viewport_tc = float4(get_viewport_tc(pixel_pos.xy), pixel_pos.zw);

#if MODFX_SHADER_VOLFOG_INJECTION
    float tcZ_sq = viewport_tc.w*view_inscatter_inv_range;
    if (tcZ_sq > 1)
      return;
#endif

#if MODFX_USE_DEPTH_MASK && !(MODFX_SHADER_VOLSHAPE || MODFX_SHADER_VOLSHAPE_WBOIT)
    if ( parent_data.mods_offsets[MODFX_RMOD_DEPTH_MASK] )
    {
      ModfxDepthMask pp = ModfxDepthMask_load( 0, parent_data.mods_offsets[MODFX_RMOD_DEPTH_MASK] );
      float4 cloud_tc = viewport_tc;
      cloud_tc.xy = reproject_scattering(cloud_tc.xy, cloud_tc.z / cloud_tc.w);
      depth_mask = dafx_get_soft_depth_mask(viewport_tc, cloud_tc, pp.depth_softness_rcp, gdata);

  #if !MODFX_SHADER_VOLFOG_INJECTION
      if (depth_mask < 1e-03)
      {
        discard;
        return fx_null;
      }
  #endif
    }
#endif

#if MODFX_USE_COLOR_MATRIX
    if ( parent_data.mods_offsets[MODFX_RMOD_TEX_COLOR_MATRIX] )
    {
      uint4 mat = dafx_get_4ui( 0, parent_data.mods_offsets[MODFX_RMOD_TEX_COLOR_MATRIX] );
      float4 r = unpack_e3dcolor_to_n4f( mat.x );
      float4 g = unpack_e3dcolor_to_n4f( mat.y );
      float4 b = unpack_e3dcolor_to_n4f( mat.z );
      float4 a = unpack_e3dcolor_to_n4f( mat.w );

      float4 c;
      c.r = dot( src_tex_0, r );
      c.g = dot( src_tex_0, g );
      c.b = dot( src_tex_0, b );
      c.a = dot( src_tex_0, a );

      src_tex_0 = saturate( c );
    }
#endif

    float alpha = src_tex_0.w * input.color.w;

    if ( FLAG_ENABLED( flags, MODFX_RFLAG_COLOR_USE_ALPHA_THRESHOLD ) )
    {
      alpha = src_tex_0.w - ( 1.f - input.color.w );
      alpha = saturate( alpha );
    }

    float3 emissive_part = 0;
    float3 lighting_part = 0;

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
      #if MODFX_USE_FOG && !MODFX_USE_FOG_PS_APPLY && FX_FOG_MULT_ALPHA
      else if (FLAG_ENABLED( flags, MODFX_RFLAG_BLEND_PREMULT))
      {
        first *= input.fog_mul.xyz;
      }
      #endif


      emissive_part = first.xyz * input.emission.r;
      lighting_part = saturate( 1.f - input.emission.r ) * first.rgb;

      if ( FLAG_ENABLED( flags, MODFX_RFLAG_TEX_COLOR_REMAP_SECOND_MASK ) )
      {
        float3 second = src_tex_0.ggg;
        if ( FLAG_ENABLED( flags, MODFX_RFLAG_TEX_COLOR_REMAP_SECOND_MASK_APPLY_BASE_COLOR ) )
        {
          second *= input.color.xyz;
        }
        #if  MODFX_USE_FOG && !MODFX_USE_FOG_PS_APPLY && FX_FOG_MULT_ALPHA
        else if (FLAG_ENABLED( flags, MODFX_RFLAG_BLEND_PREMULT))
        {
          second *= input.fog_mul.xyz;
        }
        #endif

        float3 second_emissive = second.rgb * input.emission.g;
        float3 second_lighting = saturate( 1.f - input.emission.g ) * second.rgb;

        emissive_part = lerp( second_emissive, emissive_part, src_tex_0.b );
        lighting_part = lerp( second_lighting, lighting_part, src_tex_0.b );
      }
    }
    else
#endif
    {
      float3 c = src_tex_0.xyz * input.color.xyz;
      emissive_part = c * input.emission.rgb;
      lighting_part = c;
    }

#if !(MODFX_SHADER_VOLSHAPE || MODFX_SHADER_VOLSHAPE_WBOIT || MODFX_SHADER_VOLFOG_INJECTION || MODFX_SHADER_DISTORTION)
    if (color_discard_test(float4(lighting_part.xyz + emissive_part.xyz, alpha), flags))
    {
      discard;
      return fx_null;
    }
#endif

#if MODFX_SHADER_VOLFOG_INJECTION
    float4 volfogInjection = float4(lighting_part, alpha);

    if ( parent_data.mods_offsets[MODFX_RMOD_VOLFOG_INJECTION] )
    {
      ModfxDeclVolfogInjectionParams pp = ModfxDeclVolfogInjectionParams_load( 0, parent_data.mods_offsets[MODFX_RMOD_VOLFOG_INJECTION] );
      volfogInjection.rgb *= pp.weight_rgb;
      volfogInjection.a *= pp.weight_alpha;
    }

    float3 posF = float3(viewport_tc.xy, sqrt(tcZ_sq))*view_inscatter_volume_resolution.xyz;
    posF.z += 0.5;
    float zPart = frac(posF.z);
    uint3 volfogTargetId0 = clamp(uint3(posF), 0u, uint3(view_inscatter_volume_resolution.xyz - 1));
    uint3 volfogTargetId1 = clamp(uint3(posF + float3(0,0,1)), 0u, uint3(view_inscatter_volume_resolution.xyz - 1));
    volfog_ff_initial_media_rw[volfogTargetId0] = (1-zPart)*volfogInjection;
    volfog_ff_initial_media_rw[volfogTargetId1] = zPart * volfogInjection;
    return;
#endif

#if MODFX_SHADER_VOLSHAPE || MODFX_SHADER_VOLSHAPE_WBOIT
    float thickness_mul = dafx_get_1f( 0, parent_data.mods_offsets[MODFX_RMOD_VOLSHAPE_PARAMS] );
    float radius_pow = dafx_get_1f( 0, parent_data.mods_offsets[MODFX_RMOD_VOLSHAPE_PARAMS] + 1u );

#ifdef DAFX_DEPTH_TEX
    float dst_depth = dafx_get_depth_base(viewport_tc.xy, gdata);
#else
    float dst_depth = gdata.zn_zfar.y;
#endif
    float znear_offset = 0.f;
    if (parent_data.mods_offsets[MODFX_RMOD_DEPTH_MASK])
    {
      ModfxDepthMask pp = ModfxDepthMask_load(0, parent_data.mods_offsets[MODFX_RMOD_DEPTH_MASK]);
      znear_offset = pp.znear_clip_offset;
    }

    float src_depth = linearize_z(viewport_tc.z, gdata.zn_zfar.zw) - input.life_radius.z;
    float offset = saturate(1.f - dot(input.delta, input.delta)) * input.life_radius.y;
    float far_depth = min(src_depth + offset, dst_depth);
    float near_depth = max(min(src_depth - offset, dst_depth), gdata.zn_zfar.x + znear_offset);

    float thickness = pow( max(far_depth - near_depth, 0) / (input.life_radius.y*2.f), radius_pow ) * (input.life_radius.y*2.f) * thickness_mul;
    float vol_alpha = 1.f - saturate( exp2( -2.f * thickness) );

    cur_depth = max( src_depth, gdata.zn_zfar.x);
    lighting_part *= vol_alpha;
    emissive_part *= vol_alpha;
    alpha *= vol_alpha;

    if (alpha < 1e-03)
    {
      discard;
      return fx_null;
    }

#if MODFX_SHADER_VOLSHAPE_WBOIT_APPLY

#ifdef DAFXEX_CLOUD_MASK_ENABLED
    alpha *= dafx_get_screen_cloud_volume_mask(viewport_tc.xy, viewport_tc.w);
#endif

    float4 wboit_accum = tex2D(wboit_color, viewport_tc.xy);
    float wboit_r = wboit_accum.w;
    wboit_accum.w = tex2D(wboit_alpha, viewport_tc.xy).r;

    float3 col = wboit_accum.xyz / clamp(wboit_accum.w, 0.0000001f, 1000.f);
    float a = 1.f - wboit_r;
    return encode_output(float4(col * a, a) * alpha * a, 0);
#endif

#endif

#if MODFX_USE_LIGHTING
    // TODO: optimize!
    BRANCH
    if ( parent_data.mods_offsets[MODFX_RMOD_LIGHTING_INIT] )
    {
      ModfxDeclLighting pp = ModfxDeclLighting_load( 0, parent_data.mods_offsets[MODFX_RMOD_LIGHTING_INIT] );

      float translucency = pp.translucency / 255.f;
      float sphere_rad = pp.sphere_normal_radius;
      float sphere_power = pp.sphere_normal_power / 255.f;
      float normal_softness = pp.normal_softness / 255.f;

#if MODFX_RIBBON_SHAPE
      float3 fwd_dir = input.fwd_dir;
      float ndl, nda;
      float3 wnorm = 0;
      wnorm = float3( 0, 1, 0 );
      nda = 1.f;
#else
      float3 fwd_dir = cross( input.up_dir, input.right_dir );
      float3 sphere_normal;
      sphere_normal.xy = input.delta / sphere_rad;
      sphere_normal.z = sqrt( sphere_rad * sphere_rad - dot( sphere_normal.xy, sphere_normal.xy ) );
      sphere_normal.y = -sphere_normal.y;
      sphere_normal = lerp( float3( 0, 0, 1 ), sphere_normal, sphere_power );
      sphere_normal = normalize( sphere_normal );
      sphere_normal = fwd_dir * sphere_normal.z + input.right_dir * sphere_normal.x + input.up_dir * sphere_normal.y;

      float ndl, nda;
      float3 wnorm = 0;
      if ( pp.type == MODFX_LIGHTING_TYPE_UNIFORM )
      {
        wnorm = float3( 0, 1, 0 );
        nda = 1.f;
      }
      else if ( pp.type == MODFX_LIGHTING_TYPE_DISC )
      {
        wnorm = fwd_dir;
        nda = 1.f;
      }
      else if ( pp.type == MODFX_LIGHTING_TYPE_SPHERE )
      {
        wnorm = sphere_normal;
        nda = 1.f;
      }
      else if ( pp.type == MODFX_LIGHTING_TYPE_NORMALMAP )
      {
  #if !MODFX_SHADER_VOLSHAPE && !MODFX_SHADER_VOLSHAPE_WBOIT
        float2 src_tex_1 = tex2D( g_tex_1, input.tc.xy ).rg;
        if ( FLAG_ENABLED( flags, MODFX_RFLAG_FRAME_ANIMATED_FLIPBOOK ) )
          src_tex_1 = lerp( src_tex_1, tex2D( g_tex_1, input.tc.zw ).rg, input.frame_blend );

        float3 lnorm;
        lnorm.xy = src_tex_1 * 2.f - 1.f;
        lnorm.z = sqrt( saturate( 1 - dot( lnorm.xy, lnorm.xy ) ) );

        wnorm = fwd_dir * lnorm.z + input.right_dir * lnorm.x - input.up_dir * lnorm.y;

        nda = saturate( dot( sphere_normal, wnorm ) );
  #endif
      }
#endif

      ndl = saturate( dot( -gdata.from_sun_direction, wnorm ) );

      float specular = 0;

#if !MODFX_RIBBON_SHAPE
      if ( FLAG_ENABLED( flags, MODFX_RFLAG_LIGHTING_SPECULART_ENABLED ) )
      {
        float3 h = normalize( input.view_dir - gdata.from_sun_direction );
        float ndh = saturate( dot( wnorm, h ) );
        specular = pow( ndh, pp.specular_power ) * ( pp.specular_strength / 255.f );
      }
#endif

      ndl = ndl * ( 1.f - normal_softness ) + normal_softness;
      ndl = lerp( ndl, 1.f, saturate( dot( gdata.from_sun_direction, fwd_dir ) ) * translucency );
#if MODFX_USE_SHADOW || MODFX_USE_LIGHTING || DAFX_USE_CLOUD_SHADOWS
      #if MODFX_USE_SHADOW || DAFX_USE_CLOUD_SHADOWS
      half shadow = input.lighting.a;
      #else
      half shadow = 1.f;
      #endif
      #if MODFX_USE_GI
      float3 ambient = input.ambient.rgb;
      #else
      float3 ambient = gdata.sky_color;
      #endif

      float3 lighting = ndl * gdata.sun_color * shadow + nda * ambient + input.lighting.rgb;

#if MODFX_WATER_PROJ_IGNORES_LIGHTING
      if (!dafx_is_water_proj)
#endif
        lighting_part *= lighting;
#endif
      lighting_part += specular * gdata.sun_color * alpha * shadow;
    }
#endif

    if ( FLAG_ENABLED( flags, MODFX_RFLAG_BLEND_PREMULT ) )
      emissive_part *= alpha;

    float4 result = float4( emissive_part + lighting_part, alpha );
    float depth = 0;
#if MODFX_SHADER_DISTORTION
    float depthScene = tex2Dlod(haze_scene_depth_tex, float4(viewport_tc.xy,0, haze_scene_depth_tex_lod)).x;
    float depthHaze  = GET_SCREEN_POS(input.pos).z;
    depth = depthHaze;

    BRANCH
    if (depthHaze <= depthScene)
    {
      discard;
      return encode_output(0, 0);
    }

    float distortionMod = dafx_get_1f(0, parent_data.mods_offsets[MODFX_RMOD_DISTORTION_STRENGTH]);

    if ( ( abs( result.x ) + abs( result.y ) ) < ( 1.f / 127.f ))
      result.w = 0;

    result.xy *= distortionMod;

    ModfxDeclDistortionFadeColorColorStrength finfo = ModfxDeclDistortionFadeColorColorStrength_load(0, parent_data.mods_offsets[MODFX_RMOD_DISTORTION_FADE_COLOR_COLORSTRENGTH]);

    if ((finfo.fadeRange * finfo.fadePower) > 0)
    {
      float dist = GET_SCREEN_POS(input.pos).w;
      float rate = saturate(dist / finfo.fadeRange);
      float t    = 1.0 - pow(rate, (100 - pow(finfo.fadePower, 0.015) * 100));
      result    *= t;
    }

    #if MODFX_SHADER_DISTORTION_IS_COLORED
      float3 distortionColor         = unpack_e3dcolor_to_n4f(finfo.color).rgb;
      float  distortionColorStrength = finfo.colorStrength;
      distortionColor = lerp(1, distortionColor, saturate(max(abs(result.x), abs(result.y)) * distortionColorStrength));

      result.xyz = distortionColor;
    #endif

    // no need for custom blending and hdr/fog
#endif

    half3 fog_add = 0;
#if MODFX_USE_FOG
  #if MODFX_USE_FOG_PS_APPLY
    half3 fog_mul = 1;
    get_volfog_with_precalculated_scattering_fast(
      viewport_tc.xy, viewport_tc.xy, input.view_dir, input.scattering_color__view_dir_dist.a, viewport_tc.w,
      input.scattering_color__view_dir_dist.rgb, input.scattering_base,
      fog_mul, fog_add);
    #if DAFX_USE_UNDERWATER_FOG
      modify_underwater_fog(input.scattering_color__view_dir_dist.a, fog_mul, fog_add);
    #endif
    #if MODFX_SHADER_THERMALS
      fog_modify_thermal(fog_mul, fog_add);
    #endif
    result.rgb *= fog_mul;
  #else
    fog_add = input.fog_add.xyz;
  #endif
#endif

    result.w *= depth_mask;

#if MODFX_SHADER_ATEST

    result.xyz += fog_add;
    result.xyz = modfx_pack_hdr( result.xyz );
    clip_alpha( result.w );
    result.w = 1.f;
    return encode_output(result, depth);

#elif MODFX_SHADER_FOM

    float fom_opacity = saturate(result.a) * 0.75; // can't be 1! otherwise log(0) disaster
    float fom_distance = viewport_tc.z * viewport_tc.w;

    return fx_fom_calc( fom_distance, fom_opacity );

#elif !MODFX_SHADER_VOLFOG_INJECTION

  #if MODFX_SHADER_THERMALS
    float3 thermal_normal = 1;
    if (parent_data.mods_offsets[MODFX_RMOD_LIGHTING_INIT])
    {
      ModfxDeclLighting pp = ModfxDeclLighting_load(0, parent_data.mods_offsets[MODFX_RMOD_LIGHTING_INIT]);
      if (pp.type == MODFX_LIGHTING_TYPE_NORMALMAP)
      {
        float2 src_tex_1 = tex2D(g_tex_1, input.tc.xy).gr;
        if (FLAG_ENABLED(flags, MODFX_RFLAG_FRAME_ANIMATED_FLIPBOOK))
          src_tex_1 = lerp(src_tex_1, tex2D(g_tex_1, input.tc.zw).rg, input.frame_blend);
        thermal_normal = src_tex_1.xyx * 2.f - 1.f;
      }
    }

    float3 thermal_color = src_tex_0.rgb;
    float thermal_alpha = result.a;
    if (FLAG_ENABLED(flags, MODFX_RFLAG_BLEND_ADD))
      thermal_alpha = max(thermal_color.r, max(thermal_color.g, thermal_color.b)); // alpha used in color calculations and absent for blend_add particles
    else
      thermal_color += thermal_alpha;
    if ( parent_data.mods_offsets[MODFX_RMOD_THERMAL_EMISSION] )
      result.rgb = fx_thermals_apply( max(min(1.0, input.emission.w), 0.0), thermal_color, emissive_part, 0.5 + 0.5*thermal_normal, thermal_alpha).rgb;
    else
      result.rgb = fx_thermals_apply( 0.032, thermal_color, emissive_part, 0.5 + 0.5*thermal_normal, thermal_alpha).rgb;
  #endif

    if ( FLAG_ENABLED( flags, MODFX_RFLAG_BLEND_ADD ) )
    {
      result.xyz *= result.w;
      result.w = 0;
      result.xyz = modfx_pack_hdr( result.xyz );
    }
    else if ( FLAG_ENABLED( flags, MODFX_RFLAG_BLEND_ABLEND ) )
    {
      result.xyz += fog_add;
      result.xyz *= result.w;
      result.xyz = modfx_pack_hdr( result.xyz );
    }
    else // premult
    {
      float3 ablend_part = result.xyz * alpha + fog_add;
      ablend_part = modfx_pack_hdr( ablend_part ) * result.w;

      float3 add_part = result.xyz * ( 1.f - alpha ) * depth_mask;
      add_part = modfx_pack_hdr( add_part );

      result.xyz = ablend_part + add_part;
    }

  #if MODFX_WBOIT_ENABLED
      WboitData wboit_res;
      wboit_res.color.xyz = result.xyz * wboit_weight(cur_depth, result.w);
      wboit_res.color.w = result.w;
      wboit_res.alpha = result.w * wboit_weight(cur_depth, result.w);
      return wboit_res;
  #else
    return encode_output(result, depth);
  #endif
#endif

#endif // MODFX_DEBUG_RENDER_ENABLED
  }

#endif

#endif
