include "shader_global.sh"
include "gbuffer.sh"
include "taa_inc.sh"
include "static_shadow.sh"
include "vr_multiview.sh"

texture panel_texture;
float panel_brightness;
texture panel_pointer_texture1;
float4 panel_pointer_uv_tl_iwih1;
float4 panel_pointer_color1;
texture panel_pointer_texture2;
float4 panel_pointer_uv_tl_iwih2;
float4 panel_pointer_color2;

float panel_smoothness  = 1;
float panel_reflectance = 0;
float panel_metalness   = 0;

float4x4 panel_world;
float4x4 panel_world_normal;

define_macro_if_not_defined GLOBAL_FRAME_CONTENT()
  (vs) {
    world_view_pos@f3 = world_view_pos;
  }
  (ps) {
    world_view_pos@f3 = world_view_pos;
    from_sun_direction@f3 = from_sun_direction;
    sun_color_0@f3 = sun_color_0;
    toonshading@i1 = (toonshading, 0, 0, 0);
  }

  INIT_HDR_STCODE()
endmacro

define_macro_if_not_defined INIT_HDR(code)
endmacro

shader darg_panel_frame_buffer, darg_panel_frame_buffer_no_depth, darg_panel_gbuffer, darg_panel_depth
{
  GLOBAL_FRAME_CONTENT()

  cull_mode = none;

  channel float3 pos = pos;
  channel float3 norm = norm;
  channel float2 tc[0] = tc[0];

  if (shader == darg_panel_frame_buffer_no_depth)
  {
    z_test = false;
  }

  (vs) {
    globtm@f44 = globtm;
  }
  (ps) {
    panel_texture@smp2d = panel_texture;
    panel_brightness@f1 = (panel_brightness);
    panel_pointer_texture1@smp2d = panel_pointer_texture1;
    panel_pointer_uv_tl_iwih1@f4 = panel_pointer_uv_tl_iwih1;
    panel_pointer_color1@f4 = panel_pointer_color1;
    panel_pointer_texture2@smp2d = panel_pointer_texture2;
    panel_pointer_uv_tl_iwih2@f4 = panel_pointer_uv_tl_iwih2;
    panel_pointer_color2@f4 = panel_pointer_color2;
  }

  if (shader == darg_panel_gbuffer)
  {
    (vs) {
      panel_world@f44 = panel_world;
      panel_world_normal@f44 = panel_world_normal;
    }
    (ps) {
      panel_smoothness_reflectance_metalnessfactor@f3 = (panel_smoothness, panel_reflectance, 1-(1-panel_metalness)*(1-panel_metalness), 0);
    }

    VR_MULTIVIEW_RENDERING(true, true)
  }

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float2 texcoord : TEXCOORD1;
      ##if shader == darg_panel_gbuffer
        float3 worldNormal   : TEXCOORD2;
        float3 pointToEye    : TEXCOORD3;
      ##endif
    };
  }

  hlsl(vs) {
    struct VsInput
    {
      float3 pos      : POSITION;
      float2 texcoord : TEXCOORD;
      float3 normal   : NORMAL;
    };

    VsOutput darg_panel_shader_vs(VsInput input
      ##if shader == darg_panel_gbuffer
        USE_VIEW_ID
      ##endif
    )
    {
      VsOutput output;
      float4 pos4 = float4(input.pos, 1.0);

      output.pos      = mul(pos4, globtm);
      output.texcoord = input.texcoord;
      ##if shader == darg_panel_gbuffer
        output.worldNormal   = mul((float3x3)panel_world_normal, input.normal);
        output.pointToEye    = world_view_pos - mul(pos4, panel_world).xyz;
        VR_MULTIVIEW_REPROJECT(output.pos);
      ##endif
      return output;
    }
  }

  hlsl(ps) {
    void add_cursor(inout half4 color, float2 texcoord)
    {
      ##if panel_pointer_texture1 != NULL
      {
        float2 pointer_uv = (texcoord - panel_pointer_uv_tl_iwih1.xy) * panel_pointer_uv_tl_iwih1.zw;
        half4 pointer = tex2D(panel_pointer_texture1, pointer_uv) * panel_pointer_color1;
        color.rgb = lerp(color.rgb, pointer.rgb, pointer.a);
      }
      ##endif

      ##if panel_pointer_texture2 != NULL
      {
        float2 pointer_uv = (texcoord - panel_pointer_uv_tl_iwih2.xy) * panel_pointer_uv_tl_iwih2.zw;
        half4 pointer = tex2D(panel_pointer_texture2, pointer_uv) * panel_pointer_color2;
        color.rgb = lerp(color.rgb, pointer.rgb, pointer.a);
      }
      ##endif
    }

    half4 panel_color(float2 texcoord)
    {
      half4 color = tex2D(panel_texture, texcoord);
      color.rgb *= panel_brightness;
      return color;
    }
  }

  if (shader == darg_panel_frame_buffer || shader == darg_panel_frame_buffer_no_depth)
  {
    blend_src = sa;
    blend_dst = isa;
    z_write = false;

    INIT_HDR(ps)
    USE_HDR_PS()
    USE_ATEST_1()

    USE_MOTION_VEC_MASK_ONLY()

    hlsl(ps) {
      struct PsOutput
      {
        half4 color : SV_Target0;
        half4 motion : SV_Target1;
      };

      PsOutput darg_panel_shader_frame_buffer_ps(VsOutput input)
      {
        PsOutput output;
        half4 color = panel_color(input.texcoord);

        add_cursor(color, input.texcoord);

        clip_alpha(color.a);

        output.color = float4(pack_hdr(color.rgb), color.a);
        output.motion = float4(encode_motion_mask(), 0, step(0, color.a));

        return output;
      }
    }
    compile("target_ps", "darg_panel_shader_frame_buffer_ps");
  }

  if (shader == darg_panel_gbuffer)
  {
    hlsl
    {
    ##if compatibility_mode == compatibility_mode_off
      #ifndef MOTION_VECTORS_ENABLED
        #define MOTION_VECTORS_ENABLED 1
      #endif
    ##endif
    }

    if (mobile_render != off)
    {
      INIT_STATIC_SHADOW_PS()
    }

    USE_ATEST_HALF()
    WRITE_GBUFFER()
    PACK_UNPACK_GBUFFER()
    USE_MOTION_VEC_MASK_ONLY()

    hlsl(ps) {
      GBUFFER_OUTPUT darg_panel_gbuffer_ps(VsOutput input INPUT_VFACE HW_USE_SCREEN_POS)
      {
        half4 color = panel_color(input.texcoord);
        clip_alpha(color.a);

        add_cursor(color, input.texcoord);

        UnpackedGbuffer result;
        init_gbuffer(result);

        init_albedo(result, color.rgb);
        init_smoothness(result, panel_smoothness_reflectance_metalnessfactor.x);
        init_normal(result, MUL_VFACE(normalize(input.worldNormal)));
        init_reflectance(result, panel_smoothness_reflectance_metalnessfactor.y);
        init_metalness(result, panel_smoothness_reflectance_metalnessfactor.z);
        init_dynamic(result, true);
        init_motion_vector(result, encode_motion_mask());

        return encode_gbuffer3(result, input.pointToEye.xyz, GET_SCREEN_POS(input.pos));
      }
    }
    compile("target_ps", "darg_panel_gbuffer_ps");
  }

  if (shader == darg_panel_depth)
  {
    USE_ATEST_HALF()

    hlsl(ps) {
      void darg_panel_depth_ps(VsOutput input)
      {
        half4 color = panel_color(input.texcoord);
        clip_alpha(color.a);
      }
    }
    compile("target_ps", "darg_panel_depth_ps");
  }

  compile("target_vs", "darg_panel_shader_vs");
}
