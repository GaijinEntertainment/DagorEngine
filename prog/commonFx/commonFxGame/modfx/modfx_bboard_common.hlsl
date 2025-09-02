// #define MODFX_TEX_DEBUG_ENABLED 1


#define FX_MOTION_VECS_INVERT_Y 0 // VFX artists should bake them correctly instead (used for test assets)


// disabled for now, it doesn't make a big visual difference with gamma correction, but it makes it slower
#define MODFX_USE_PACK_HDR_ON_DISCARD 0


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

#define MODFX_PASS_FRAME_FLAGS (MODFX_USE_LIGHTING && !SIMPLIFIED_VS_LIGHTING && !MODFX_RIBBON_SHAPE && !MODFX_SHADER_VOLSHAPE && !MODFX_SHADER_VOLSHAPE_WBOIT)

#define MODFX_DISTORTION_DISCARD_THRESHOLD (1.0 / 255)

// VS and PS functions
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
  #elif !MODFX_SHADER_WATER_FX
    COLOR_INTERPOLATION float3 emission   : TEXCOORD4;
  #endif

  #if MODFX_USE_SHADOW || DAFX_USE_CLOUD_SHADOWS
    float4 lighting   : TEXCOORD7;
    float fake_brightness : TEXCOORD16;
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

  #if MODFX_PASS_FRAME_FLAGS
    nointerpolation uint frame_flags  : TEXCOORD15;
  #endif

  #if !MODFX_RIBBON_SHAPE || (MODFX_USE_FOG && MODFX_USE_FOG_PS_APPLY)
    PARAM_INTERPOLATION float3 view_dir   : TEXCOORD13;
  #endif

  #if MODFX_SHADER_XRAY
    PARAM_INTERPOLATION float view_dir_dist : TEXCOORD5;
  #endif

  #if MODFX_SHADER_DISTORTION
    PARAM_INTERPOLATION float zfar_strength : TEXCOORD5;
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

  bool is_blending_premult(uint flags)
  {
    #if (MODFX_SHADER_ATEST || MODFX_SHADER_FOM || MODFX_SHADER_VOLFOG_INJECTION)
      return false;
    #elif MODFX_SHADER_DISTORTION
      return true;
    #else
      return FLAG_ENABLED( flags, MODFX_RFLAG_BLEND_PREMULT );
    #endif
  }

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

// PS functions

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

  float2 decode_motion_vecs(float2 mv)
  {
    mv = mv * 2 - 1;
    #if FX_MOTION_VECS_INVERT_Y
      mv.y = -mv.y;
    #endif
    return mv;
  }

  float4 modfx_render_get_motion_vecs(
    ModfxParentRenData parent_data,
    float4 tc,
    float frame_blend)
  {
    float4 mvAB = 0;
  #if MODFX_USE_MOTION_VECS && MODFX_USE_FRAMEBLEND && !MODFX_SHADER_VOLSHAPE_WBOIT_APPLY
    BRANCH
    if (parent_data.mods_offsets[MODFX_RMOD_MOTION_VECTORS] )
    {
      ModfxDeclMotionVectors motionVecs = ModfxDeclMotionVectors_load( 0, parent_data.mods_offsets[MODFX_RMOD_MOTION_VECTORS] );
      mvAB = float4(decode_motion_vecs(tex2D(g_tex_1, tc.xy).xy), decode_motion_vecs(tex2D(g_tex_1, tc.zw).xy));
      mvAB *= float2(-frame_blend, 1.0 - frame_blend).xxyy * motionVecs.motion_vectors_strength;
    }
  #endif
    return mvAB;
  }

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

  bool color_discard_test(float4 src, uint flags)
  {
    #if (MODFX_SHADER_VOLSHAPE || MODFX_SHADER_VOLSHAPE_WBOIT || MODFX_SHADER_VOLFOG_INJECTION)
    {
      return false;
    }
    #elif MODFX_SHADER_FOM
    {
      // FOM result is alpha*something: we can discard by alpha
      // we need to use gamma corrected color threshold, becuase the gamma corrected rgb is scaled by alpha
      return src.a < fx_discard_color_threshold_gamma;
    }
    #elif MODFX_SHADER_DISTORTION
    {
      #if MODFX_SHADER_DISTORTION_IS_COLORED
        // special pass, can't skip anything
        return false;
      #else
        // no distortion if the offset is close to zero
        return all(abs(src.xy) < MODFX_DISTORTION_DISCARD_THRESHOLD);
      #endif
    }
    #elif MODFX_SHADER_ATEST
    {
      // we clip using result.w (final alpha), which is not bigger than source alpha, so this acts as a broad phase discard
      return src.a < atest_value_ref;
    }
    #else
    {
      float src_alpha = src.a;
      #if MODFX_USE_PACK_HDR_ON_DISCARD
        src.rgb = modfx_unpack_hdr(src.rgb);
      #endif
      float src_color_max = max3(src.rgb);

      FLATTEN if (is_blending_premult(flags))
      {
        // finalColor.rgb = (srcColor.rgb) + ((1-srcColor.a) * dstColor.rgb);
        // finalColor.a = ((1-srcColor.a) * dstColor.a);
        // -> this is a tricky one, because srcColor.rgb has a part that is multiplied by alpha and another by inverse (1-alpha*something)
        // -> on top of that, we can rely on no op in HW blending only if the alpha is close to 0
        // we need both rgb and alpha to be close to 0, and both need to be gamma corrected (alpha too, because we premultiply a part of rgb by it)
        return src_alpha < fx_discard_color_threshold_gamma && src_color_max < fx_discard_color_threshold_gamma;
      }
      else FLATTEN if ( FLAG_ENABLED( flags, MODFX_RFLAG_BLEND_ADD ) )
      {
        // finalColor.rgb = (srcColor.rgb*srcColor.a) + (dstColor.rgb);
        // finalColor.a = (dstColor.a);
        // -> we can rely on no op only if (srcColor.rgb*srcColor.a) is close to 0 (the srcColor.a multiplication is in PS)
        // the hw blending is additive, so only the (gamma corrected) rgb makes a difference, but it is multiplied by alpha in PS
        return src_alpha * src_color_max < fx_discard_color_threshold_gamma;
      }
      else // MODFX_RFLAG_BLEND_ABLEND
      {
        // finalColor.rgb = (srcColor.rgb*srcColor.a) + ((1-srcColor.a) * dstColor.rgb);
        // finalColor.a = ((1-srcColor.a) * dstColor.a);
        // -> we can rely on no op only if the srcColor.a is close to 0
        // alpha doesn't have gamma correction, and although we do have a blending on rgb with (1-alpha),
        // the small changes are around alpha=1, so they aren't visible, thus we can use the non-gamma corrected threshold
        return src_alpha < fx_discard_alpha_threshold;
      }
    }
    #endif
  }

  float calc_alpha(float4 src, VsOutput input, uint flags)
  {
    float alpha = src.w * input.color.w;
    if ( FLAG_ENABLED( flags, MODFX_RFLAG_COLOR_USE_ALPHA_THRESHOLD ) )
    {
      alpha = src.w - ( 1.f - input.color.w );
      alpha = saturate( alpha );
    }
    return alpha;
  }

  float4 modfx_render_mul_color_matrix(
    ModfxParentRenData parent_data,
    float4 color_in)
  {
    float4 color_out = color_in;
  #if MODFX_USE_COLOR_MATRIX
    if ( parent_data.mods_offsets[MODFX_RMOD_TEX_COLOR_MATRIX] )
    {
      uint4 mat = dafx_get_4ui( 0, parent_data.mods_offsets[MODFX_RMOD_TEX_COLOR_MATRIX] );
      float4 r = unpack_e3dcolor_to_n4f( mat.x );
      float4 g = unpack_e3dcolor_to_n4f( mat.y );
      float4 b = unpack_e3dcolor_to_n4f( mat.z );
      float4 a = unpack_e3dcolor_to_n4f( mat.w );

      float4 c;
      c.r = dot( color_in, r );
      c.g = dot( color_in, g );
      c.b = dot( color_in, b );
      c.a = dot( color_in, a );

      color_out = saturate( c );
    }
  #endif
    return color_out;
  }
#endif