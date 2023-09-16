include "shader_global.sh"
include "cloud_mask.sh"
include "gui_aces_helpers.sh"
include "gbuffer.sh"

float4 blue_to_alpha_mul = (0, 0, 0, 1);
texture tex;
texture mask_tex;
float4 mask_matrix_line_0 = (1, 0, 0, 0);
float4 mask_matrix_line_1 = (0, 1, 0, 0);
float4 gui_to_scene_point_to_eye = (0, 0, 0, 0);

block(object) gui_aces_object
{
  (vs) {
    mask_matrix_line_0@f3 = mask_matrix_line_0;
    mask_matrix_line_1@f3 = mask_matrix_line_1;
  }
  (ps) {
    tex@smp2d = tex;
    mask_tex@smp2d = mask_tex;
    blue_to_alpha_mul@f4 = blue_to_alpha_mul;
  }
}



//controlled by stdgui
int gui_write_to_z = 0;
interval gui_write_to_z : write_to_z_off < 1, write_to_z_on;
int gui_test_depth = 0;
interval gui_test_depth : test_depth_off < 1, test_depth_on<2, test_depth_cockpit;

int night_vision_gui_mode = 0;
interval night_vision_gui_mode : night_vision_off<1, night_vision_on;

float gui_cockpit_depth = 10;

shader gui_aces
{
  supports gui_aces_object;

  cull_mode = none;

  dynamic int render_mode = 0;
  interval render_mode: to_hud < 1, to_scene;

  if (render_mode == to_scene)
  {
    blend_src = sa; blend_dst = isa;
  }
  else
  {
    blend_src = one; blend_dst = isa;
  }

  DECL_POSTFX_TC_VS_DEP()
  USE_ATEST_1()
  USE_GUI_ACES_HELPERS()

  channel float4 pos = pos;

  if (gui_write_to_z == write_to_z_on)
  {
    z_write = true;
  }
  else
  {
    z_write = false;
  }

  if (render_mode == to_scene) //act like scene object
  {
    hlsl {
      #define OGL_SCREEN_POSITION float4 screenPosition : TEXCOORD4;
    }
    hlsl(vs) {
      #define OGL_SET_SCREEN_POSITION(OUT, OUTpos) OUT.screenPosition = float4(   \
        OUTpos.xy * float2(0.5, HALF_RT_TC_YSCALE) + 0.5 * OUTpos.w,              \
        OUTpos.z,                                                                 \
        OUTpos.w);
    }
    hlsl(ps) {
      #define OGL_DEPTH_TEST(VSOUT)
    }

    if (gui_test_depth == test_depth_on)
    {
      z_test = true;
    }
    else
    {
      z_test = false;
    }

    GET_CLOUD_VOLUME_MASK()
    INIT_BRUNETON_FOG(ps)
    USE_BRUNETON_FOG()

    (ps) { gui_to_scene_point_to_eye@f4 = gui_to_scene_point_to_eye; }
  }
  else if (gui_test_depth != test_depth_off) //render to hud
  {
    if (depth_gbuf != NULL)// && shader==invalid_shader
    {
      z_test = false;

      (ps) { gui_cockpit_depth__screensize@f3 = (gui_cockpit_depth, 1./screen_pos_to_texcoord.x, 1./screen_pos_to_texcoord.y, 0); }

      hlsl(ps)
      {
        #define gui_cockpit_depth gui_cockpit_depth__screensize.x
        #define screen_size gui_cockpit_depth__screensize.yz
      }

      hlsl(ps) {
        #define HALF_PLUS_HALF_PIXEL_OFS float2(0.5,0.5)
      }
      if (gui_test_depth == test_depth_cockpit)
      {
        hlsl {
          #define OGL_SCREEN_POSITION float3 screenPosition:TEXCOORD4;

        }
        hlsl(vs) {
          #define OGL_SET_SCREEN_POSITION(OUT, OUTpos) OUT.screenPosition = OUTpos.xyw;
        }
        hlsl(ps) {
          #define OGL_DEPTH_TEST(VSOUT) {float2 ftc=(VSOUT.screenPosition.xy * rcp(VSOUT.screenPosition.z))*RT_SCALE_HALF+HALF_PLUS_HALF_PIXEL_OFS;\
          clip(linearize_z(readGbufferDepth(ftc), zn_zfar.zw)-gui_cockpit_depth);}
        }
      } else
      {
        hlsl {
          #define OGL_SCREEN_POSITION float3 screenPosition:TEXCOORD4;
          #define OGL_SET_SCREEN_POSITION(OUT, OUTpos) OUT.screenPosition = OUTpos.xyw;
        }
        hlsl(ps) {
          #define OGL_DEPTH_TEST(VSOUT) {float2 ftc=(VSOUT.screenPosition.xy *rcp(VSOUT.screenPosition.z))*RT_SCALE_HALF+HALF_PLUS_HALF_PIXEL_OFS;\
          clip(linearize_z(readGbufferDepth(ftc), zn_zfar.zw)-VSOUT.screenPosition.z);}
        }
      }
    } else
    {
      if (gui_test_depth == test_depth_on)
      {
        z_test = true;
      }
      hlsl {
        #define OGL_SCREEN_POSITION 
        #define OGL_SET_SCREEN_POSITION(OUT, OUTpos) 
        #define OGL_DEPTH_TEST(VSOUT)
      }
    }
  }
  else
  {
    z_test = false;
    hlsl {
      #define OGL_SCREEN_POSITION 
      #define OGL_SET_SCREEN_POSITION(OUT, OUTpos) 
      #define OGL_DEPTH_TEST(VSOUT)
    }
  }



  channel color8 vcol = vcol;
  channel float2 tc = tc;


  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float2 texcoord     : TEXCOORD0;
##if mask_tex != NULL
      float2 maskTexcoord : TEXCOORD1;
##endif
      float4 color        : COLOR0;
      OGL_SCREEN_POSITION
    };
  }
  
  hlsl(vs) {
    struct VsInput
    {
      float4 pos: POSITION;
      float2 texcoord : TEXCOORD0;
      float4 color    : COLOR0;
    };

    VsOutput main_vs_gui_aces(VsInput input)
    {
      VsOutput output;

      output.pos = input.pos;
      output.texcoord = input.texcoord;
##if mask_tex != NULL
      output.maskTexcoord.x = dot(input.pos.xy / input.pos.w, mask_matrix_line_0.xy) + mask_matrix_line_0.z;
      output.maskTexcoord.y = dot(input.pos.xy / input.pos.w, mask_matrix_line_1.xy) + mask_matrix_line_1.z;
##endif
      output.color = BGRA_SWIZZLE(input.color);
      OGL_SET_SCREEN_POSITION(output, output.pos);

      return output;
    }
  }

  hlsl(ps) {
    fixed4 main_ps_gui_aces(VsOutput input) : SV_Target
    {
      OGL_DEPTH_TEST_FOR_INPUT(input);

      fixed4 result = h4tex2D(tex, input.texcoord);
      result.a = dot(result, blue_to_alpha_mul);
      result.rgb = result.rgb * blue_to_alpha_mul.a + blue_to_alpha_mul.rrr * result.a;

##if mask_tex != NULL
      result *= h4tex2D(mask_tex, input.maskTexcoord).a;
##endif
      clip_alpha(result.a);

      result *= input.color;

##if render_mode == to_scene
      result.a *= get_cloud_volume_mask(input.screenPosition);
      BRANCH
      if (gui_to_scene_point_to_eye.w > 0.f)
        result.rgb = apply_fog(result.rgb, gui_to_scene_point_to_eye.xyz);
      result.rgb = pack_hdr(result.rgb).rgb;
##else
      result.rgb *= input.color.a;
      result.a = gui_hdr_tonemap(result).a;   // Do not modify color to avoid double tonemaping when appling dagui RT.
##endif
##if night_vision_gui_mode == night_vision_on
      result.rgb *= 0.07;
##endif
      return result;
    }
  }
  compile("target_vs", "main_vs_gui_aces");
  compile("target_ps", "main_ps_gui_aces");
}



shader depth_fill_gui_aces
{
  supports gui_aces_object;

  cull_mode = none;
  z_write = true;
  color_write = false;
  no_ablend;
  USE_ATEST_255()

  channel float4 pos = pos;
  channel color8 vcol = vcol;
  channel float2 tc = tc;

  DECL_POSTFX_TC_VS_DEP()

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float2 texcoord     : TEXCOORD0;
    };
  }
  
  hlsl(vs) {
    struct VsInput
    {
      float4 pos: POSITION;
      float2 texcoord : TEXCOORD0;
      float4 color    : COLOR0;
    };

    VsOutput depth_fill_gui_aces_vs(VsInput input)
    {
      VsOutput output;

      output.pos = input.pos;
      output.texcoord = input.texcoord;

      return output;
    }
  }

  hlsl(ps) {
    fixed4 depth_fill_gui_aces_ps(VsOutput input) : SV_Target
    {
      fixed4 result = h4tex2D(tex, input.texcoord);
      clip_alpha(result.a);
      return result;//float4(1, 1, 1, 1);
    }
  }
  compile("target_vs", "depth_fill_gui_aces_vs");
  compile("target_ps", "depth_fill_gui_aces_ps");
}



shader antialised_2d_line
{
  dont_render;//not used any more
  cull_mode  = none;
  z_test = false;
  z_write = false;

  channel float2 pos = pos;
  channel color8 vcol = vcol;
  channel float4 tc = tc;
  channel float4 tc[1] = tc[1];
  channel float4 tc[2] = tc[2];
  blend_src = one;
  blend_dst = isa;
  USE_ATEST_1()

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float4 col: COLOR0;
      //float3 wpos: TEXCOORD0;
      float4 e0t: TEXCOORD1;
      float4 e1t: TEXCOORD2;
      float4 e2t: TEXCOORD3;
    };
  }

  hlsl(vs) {
    struct VsInput
    {
      float2 pos: POSITION;
      float4 col: COLOR0;
      float4 e0t: TEXCOORD0;
      float4 e1t: TEXCOORD1;
      float4 e2t: TEXCOORD2;
    };

    VsOutput antialised_2d_line_vs(VsInput IN)
    {
      VsOutput OUT;
      OUT.pos = float4(IN.pos.x, IN.pos.y, 1, 1);
      OUT.col = BGRA_SWIZZLE(IN.col);
      //OUT.wpos = (OUT.pos.xyz*float3(0.5,-0.5,1)+float3(0.5,0.5,0))*float3(1280,720,1);
      OUT.e0t = IN.e0t;
      OUT.e1t = IN.e1t;
      OUT.e2t = IN.e2t;
      return OUT;
    }
  }
  compile("target_vs", "antialised_2d_line_vs");


  hlsl(ps) {
    half aa_line(float2 p, float4 E0t, float4 E1t, float4 E2t)
    { 

      // evaluate edge functions f0, f1 

      // discard fragments that lie outside the line 

      // choose the relevant distance (edge) to use 
      half4 scaledDistance = saturate(E0t*p.x+E1t*p.y+E2t);
      //half dist = max (max(scaledDistance.x, scaledDistance.y), max(scaledDistance.z, scaledDistance.w));
      //return dist<0 ? 1 : 1-length(saturate(scaledDistance));
      //scaledDistance)
      return all(bool4(scaledDistance > 0)) ? 1 : 1 - length(scaledDistance);

      //half4 scaledDistance = E0t*p.x+E1t*p.y+E2t; 
      //return saturate(1-dist);
      //half index = saturate(min (min(scaledDistance.x, scaledDistance.y), min(scaledDistance.z, scaledDistance.w)));
      //return index;

      // map to alpha using precomputed filter table 
      //half alpha = tex1D(intensityMap, index); 

      // 
      // do other shading here ... 
      // 

      // color.xyz = ... 
      //color.w = alpha; 
    }
    half4 antialised_2d_line_ps(VsOutput IN HW_USE_SCREEN_POS) : SV_Target
    {
      float4 screenpos = GET_SCREEN_POS(IN.pos);
      half line_aa = aa_line(screenpos, IN.e0t, IN.e1t, IN.e2t);
      
      half4 result =  IN.col*line_aa;
      clip_alpha(result.a);
      return result;
    }
  }
  compile("target_ps", "antialised_2d_line_ps");
}