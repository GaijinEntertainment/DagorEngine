include "shader_global.sh"

hlsl(ps) {
  #define INTERNAL_FONT_GAMMA 1.5//it is Alexey's Borisov's driven gamma (instead of correct 2.2 or more faster 2)
}

float4 gui_text_depth = (1, 1, 0, 0);

float gui_cockpit_depth = 10;

shader gui_default
{
  // declaring vars
  dynamic texture tex;
  dynamic float4 viewportRect;
  
  dynamic int transparent = 0;
  interval transparent: transOff < 1, transOn < 2, transNonpremultiplied < 3, transAdditive;

  dynamic int hasTexture = 0;
  interval hasTexture: texOff < 1, texOn < 2, texFontL8 < 3, texFontL8v;

  dynamic int fontFxType = 0;
  interval fontFxType: no < 1, shadow < 2, glow < 3, outline < 4, undefined;
  dynamic float fontFxScale = 0;
  interval fontFxScale: sharp < 0.01, smooth < 1.3, extraSmooth;
  dynamic float4 fontFxOfs;
  dynamic float4 fontFxColor;

  dynamic texture fontTex2;
  dynamic float4 fontTex2ofs;
  dynamic float4 fontTex2rotCCSmS = (1, 1, 0, 0);

  if (fontFxType == undefined) { dont_render; }

  // setup constants 
  cull_mode = none;
  z_write = false;
  z_test = false;


  (vs)
  {
    viewport@f4 = viewportRect;
    gui_text_depth@f2 = (gui_text_depth);//(gui_text_depth.r, gui_text_depth.g, gui_cockpit_depth, 0, 0);
  }
  hlsl {
    #define OGL_SCREEN_POSITION 
    #define OGL_SET_SCREEN_POSITION(OUT, OUTpos) 
    #define OGL_DEPTH_TEST(VSOUT)
  }

  if (hasTexture >= texFontL8)
  {
    blend_src = one; blend_dst = isa;
    (ps)
    {
      font@smp2d = tex;
    }
    if (fontFxType != no)
    {
      (vs){ fontFxOfs@f4 = fontFxOfs; }
      (ps)
      {
        fontFxOfs@f4 = fontFxOfs;
        fontFxColor@f4 = fontFxColor;
      }
      if (fontFxScale != sharp)
      {
        (vs)
        {
          fontFxScale@f1 = (fontFxScale);
        }
        if (fontFxType == glow) {
          (ps){ scales@f4 = (0.6, 0.25, 0.125, fontFxScale); }
        } else {
          (ps){ scales@f4 = (0.3, 0.25*0.6, 0.125*0.7, fontFxScale); }
        }
      }
    }
    if (fontTex2 != NULL)
    {
      (ps){ fontTex2@smp2d = fontTex2; }
      (vs)
      {
        fontTex2ofs@f4 = fontTex2ofs;
        fontTex2rotCCSmS@f4 = fontTex2rotCCSmS;
      }
    }
    NO_ATEST()
  }
  else if (transparent == transOn)
  {
    blend_src = one; blend_dst = isa;
    NO_ATEST()
  }
  else if (transparent == transNonpremultiplied)
  {
    blend_src = sa; blend_dst = isa;
    blend_asrc = 1; blend_adst = isa;
    USE_ATEST_1()
  }
  else if (transparent == transAdditive)
  {
    blend_src = one; blend_dst = one;
    NO_ATEST()
  } else
  {
    NO_ATEST()
  }

  // init channels
  channel short2 pos = pos;
  channel color8 vcol = vcol;
  channel short2 tc = tc mul_4k;
  channel short2 tc[1] = tc[1];

  if (hasTexture == texOn)
  {
    (ps){ tex@smp2d = tex; }
  }

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float4 col: TEXCOORD0;
      OGL_SCREEN_POSITION
    ##if hasTexture != texOff
      float2 tc:TEXCOORD1;
    ##endif

    ##if hasTexture >= texFontL8
      ##if fontFxType != no
        float2 tc1:TEXCOORD2;
        float4 tc2:TEXCOORD3;
        ##if fontTex2 != NULL
          float2 f2_tc:TEXCOORD4;
        ##endif
      ##elif fontTex2 != NULL
        float2 f2_tc:TEXCOORD2;
      ##endif
    ##endif
    };
  }

  hlsl(vs) {
    struct VsInput
    {
      int2 pos: POSITION;
      half4 col: COLOR0;
    ##if hasTexture != texOff
      int2 tc0: TEXCOORD0;
      ##if hasTexture >= texFontL8 && fontFxType != no
      int2 tc1: TEXCOORD1;
      ##endif
    ##endif
    };

    VsOutput gui_main_vs(VsInput v)
    {
      VsOutput o;
      float2 fpos = float2(v.pos);

    ##if hasTexture >= texFontL8 && fontFxType != no
      float2 tc = v.tc0/4096.0;
      float2 tc1 = float2(v.tc1);

      float2 ofs = float2(tc1.x > 0. ? 1. : 0., tc1.y > 0. ? 1. : 0.);
      o.tc2.xy = tc - ofs * tc1 * fontFxOfs.zw;
      o.tc2.zw = o.tc2.xy + abs(tc1.xy)*fontFxOfs.zw;

      ##if fontFxScale == sharp
      float2 add = float2(0., 0.);
      ##else
      float2 add = float2(tc1.x > 0. ? 2. : -2., tc1.y > 0. ? 2. : -2.)*fontFxScale.xx;
      ##endif

      o.tc = tc + (ofs*fontFxOfs.xy + add)*fontFxOfs.zw;
      o.tc1 = tc + ((ofs-1)*fontFxOfs.xy + add)*fontFxOfs.zw;
      o.pos = float4(((fpos+(ofs*fontFxOfs.xy + add)*4.)*viewport.xy+viewport.zw) * gui_text_depth.y, gui_text_depth.xy);
      o.col = BGRA_SWIZZLE(v.col);

    ##else
      o.pos = float4((fpos*viewport.xy+viewport.zw) * gui_text_depth.y, gui_text_depth.xy);
      o.col = BGRA_SWIZZLE(v.col);
    ##if hasTexture != texOff
      o.tc = v.tc0/4096.0;
    ##endif
    ##endif

    ##if hasTexture >= texFontL8 && fontTex2 != NULL
      float2 otc = fpos.xy + fontTex2ofs.zw;
      o.f2_tc = (otc.xy*fontTex2rotCCSmS.xy + otc.yx*fontTex2rotCCSmS.zw) * fontTex2ofs.xy;
    ##endif
      OGL_SET_SCREEN_POSITION(o, o.pos)
      return o;
    }
  }
  compile("target_vs", "gui_main_vs");


  if (hasTexture == texOn)
  {
    hlsl(ps) {

      half4 gui_default_ps_1(VsOutput input) : SV_Target
      {
        OGL_DEPTH_TEST(input)
        fixed4 result = fx4tex2D(tex, input.tc) * input.col;
        clip_alpha(result.a);
        return result;
      }
    }
    compile("target_ps", "gui_default_ps_1");
  }
  else if (hasTexture >= texFontL8)
  {
    hlsl(ps) {

    ##if fontFxType == no
      fixed4 font_draw_ps(VsOutput v): SV_Target
      {
        OGL_DEPTH_TEST(v)
        fixed font_alpha = fx4tex2D(font,v.tc).r;
        fixed gamma_font = pow(font_alpha, 1./INTERNAL_FONT_GAMMA);
      ##if fontTex2 != NULL
        fixed4 t2 = fx4tex2D(fontTex2, v.f2_tc);
        fixed4 result = (fixed4(t2.rgb,1)*fixed4(gamma_font.rrr, font_alpha)*v.col*t2.a);
      ##else
        fixed4 result = fixed4(gamma_font.rrr, font_alpha) * v.col;
      ##endif
        clip_alpha(result.a);
        return result;
      }

    ##else
      half select(half4 tc_xyxy, half4 box_ltrb, half inside, half outside)
      {
##if hardware.ps4 || hardware.ps5
        if (tc_xyxy.x < box_ltrb.x)
          return outside;
        if (tc_xyxy.y < box_ltrb.y)
           return outside;
        if (tc_xyxy.x > box_ltrb.z)
          return outside;
        if (tc_xyxy.y > box_ltrb.w)
           return outside;
        return inside;
##else
        #if SHADER_COMPILER_HLSL2021
          bool isInside = all(select(bool4(tc_xyxy > box_ltrb), float4(1, 1, 0, 0), float4(0, 0, 1, 1)));
        #else
          bool isInside = all(bool4(tc_xyxy > box_ltrb) ? half4(1,1,0,0) : half4(0,0,1,1));
        #endif
        return isInside ? inside : outside;
##endif
      }

      ##if fontFxType == outline
        half4 font_draw_ps(VsOutput v /*HW_USE_SCREEN_POS*/): SV_Target
        {
          // float4 screenpos = GET_SCREEN_POS(v.pos);
          OGL_DEPTH_TEST(v)

          half l_font = select(v.tc.xyxy, v.tc2, fx4tex2D(font,v.tc).r, 0);
          half l_shadow = 0.0;
          ##if fontFxScale != sharp
            float2 tcScale = fontFxOfs.zw * scales.w;
            float2 tc = v.tc1 + float2(-0.707, -0.707) * tcScale;
            l_shadow += select(tc.xyxy, v.tc2, h4tex2D(font, tc).r, 0);
            tc = v.tc1 + float2(-1, 0) * tcScale;
            l_shadow += select(tc.xyxy, v.tc2, h4tex2D(font, tc).r, 0);
            tc = v.tc1 + float2(-0.707, 0.707) * tcScale;
            l_shadow += select(tc.xyxy, v.tc2, h4tex2D(font, tc).r, 0);
            tc = v.tc1 + float2(0, 1) * tcScale;
            l_shadow += select(tc.xyxy, v.tc2, h4tex2D(font, tc).r, 0);
            tc = v.tc1 + float2(0.707, 0.707) * tcScale;
            l_shadow += select(tc.xyxy, v.tc2, h4tex2D(font, tc).r, 0);
            tc = v.tc1 + float2(1, 0) * tcScale;
            l_shadow += select(tc.xyxy, v.tc2, h4tex2D(font, tc).r, 0);
            tc = v.tc1 + float2(0.707, -0.707) * tcScale;
            l_shadow += select(tc.xyxy, v.tc2, h4tex2D(font, tc).r, 0);
            tc = v.tc1 + float2(0, -1) * tcScale;
            l_shadow = saturate(l_shadow + select(tc.xyxy, v.tc2, h4tex2D(font, tc).r, 0));
          ##endif
          ##if fontTex2 != NULL
            half4 f_col = h4tex2D(fontTex2, v.f2_tc);
            f_col = half4(f_col.rgb,1)*v.col*f_col.a;
          ##else
            half4 f_col = v.col;
          ##endif

          half4 result;
          result.rgb = pow(lerp(pow(l_shadow*fontFxColor.rgb,INTERNAL_FONT_GAMMA), pow(f_col.rgb,INTERNAL_FONT_GAMMA), l_font), 1./INTERNAL_FONT_GAMMA);//use gamma = 2. for perfromance
          result.a = lerp(l_shadow*fontFxColor.a, f_col.a, l_font);

          //half4 result = lerp(l_shadow * fontFxColor, f_col, l_font);
          //half gamma_font = pow(l_font, 1/2.2);
          //half4 result = (half4(gamma_font.rrr, l_font) * f_col + (1 - l_font) * saturate(l_shadow) * fontFxColor);
          clip_alpha(result.a);
          return result;
        }
      ##else
        half4 font_draw_ps(VsOutput v): SV_Target
        {
          OGL_DEPTH_TEST(v)
        half l_font = select(v.tc.xyxy, v.tc2, h4tex2D(font, v.tc).r, 0);
        ##if hasTexture == texFontL8v
          if (l_font < 0.5)
            l_font = 0;
        ##endif

          ##if fontTex2 != NULL
            half4 f_col = h4tex2D(fontTex2, v.f2_tc);
            f_col = half4(f_col.rgb, 1) * v.col * f_col.a;
          ##else
            half4 f_col = v.col;
          ##endif


          half l_shadow = select(v.tc1.xyxy, v.tc2, h4tex2D(font, v.tc1).r, 0);
        ##if fontFxScale != sharp
          float2 tc;
          float2 tcScale = fontFxOfs.zw*scales.w;
          half l_shadow2 = 0, l_shadow3 = 0;
          
            tc = v.tc1+float2(-0.707, -0.707)*tcScale;
            l_shadow2 += select(tc.xyxy, v.tc2, h4tex2D(font, tc).r, 0);
            tc = v.tc1+float2( 0.707, -0.707)*tcScale;
            l_shadow2 += select(tc.xyxy, v.tc2, h4tex2D(font, tc).r, 0);
            tc = v.tc1+float2(-0.707,  0.707)*tcScale;
            l_shadow2 += select(tc.xyxy, v.tc2, h4tex2D(font, tc).r, 0);
            tc = v.tc1+float2( 0.707,  0.707)*tcScale;
            l_shadow2 += select(tc.xyxy, v.tc2, h4tex2D(font, tc).r, 0);

            ##if (fontFxScale == extraSmooth)
            tc = v.tc1+float2(-2.0,    0)*tcScale;
            l_shadow3 += select(tc.xyxy, v.tc2, h4tex2D(font, tc).r, 0);
            tc = v.tc1+float2(   0, -2.0)*tcScale;
            l_shadow3 += select(tc.xyxy, v.tc2, h4tex2D(font, tc).r, 0);
            tc = v.tc1+float2( 2.0,    0)*tcScale;
            l_shadow3 += select(tc.xyxy, v.tc2, h4tex2D(font, tc).r, 0);
            tc = v.tc1+float2(   0,  2.0)*tcScale;
            l_shadow3 += select(tc.xyxy, v.tc2, h4tex2D(font, tc).r, 0);

            tc = v.tc1+float2(-1.41, -1.41)*tcScale;
            l_shadow3 += select(tc.xyxy, v.tc2, h4tex2D(font, tc).r, 0);
            tc = v.tc1+float2( 1.41, -1.41)*tcScale;
            l_shadow3 += select(tc.xyxy, v.tc2, h4tex2D(font, tc).r, 0);
            tc = v.tc1+float2(-1.41,  1.41)*tcScale;
            l_shadow3 += select(tc.xyxy, v.tc2, h4tex2D(font, tc).r, 0);
            tc = v.tc1+float2( 1.41,  1.41)*tcScale;
            l_shadow3 += select(tc.xyxy, v.tc2, h4tex2D(font, tc).r, 0);
          ##else
          l_shadow2 *= 0.5;
          ##endif

          l_shadow = saturate(l_shadow*scales.x + l_shadow2*scales.y + l_shadow3*scales.z);
        ##endif

        ##if fontTex2 != NULL
          l_shadow *= h4tex2D(fontTex2, v.f2_tc+v.tc1-v.tc).a;
        ##endif

        ##if hasTexture == texFontL8v
          clip(l_font+(1-l_font)*l_shadow*250-0.5);
        ##endif
          //gamma correct lerp
          half4 result;
          result.rgb = pow(lerp(pow(l_shadow*fontFxColor.rgb, INTERNAL_FONT_GAMMA), pow(f_col.rgb, INTERNAL_FONT_GAMMA), l_font), 1./INTERNAL_FONT_GAMMA);//use gamma = 2. for perfromance
          result.a = lerp(l_shadow*fontFxColor.a, f_col.a, l_font);//use gamma = 2. for perfromance
          //half4 result = lerp(l_shadow*fontFxColor, f_col, l_font);
          clip_alpha(result.a);
          return result;
        }
      ##endif
    ##endif
    }
    compile("target_ps", "font_draw_ps");
  }
  else
  {
    hlsl(ps) {
      half4 gui_default_ps_3(VsOutput input) : SV_Target
      {
        return input.col;
      }
    }
    compile("target_ps", "gui_default_ps_3");
  }
}
