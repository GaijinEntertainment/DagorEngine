include "shader_global.sh"

float4 gui_text_depth = (1, 1, 0, 0);

float gui_cockpit_depth = 10;

texture mask_tex;
float4 mask_matrix_line_0 = (1, 0, 0, 0);
float4 mask_matrix_line_1 = (0, 1, 0, 0);
int gui_user_const_ps_no = 15 always_referenced;
int gui_tex_mip = 0 always_referenced;

hlsl(ps) {
  float gui_tex_mip:register(c15);
}

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

  dynamic int useColorMatrix = 0;
  interval useColorMatrix: cmOff < 1, cmUsed;
  dynamic float4 colorMatrix0;
  dynamic float4 colorMatrix1;
  dynamic float4 colorMatrix2;
  dynamic float4 colorMatrix3;

  if (fontFxType == undefined) { dont_render; }

  // setup constants 
  cull_mode = none;
  z_write = false;
  z_test = false;

  (vs) {
    viewport@f4 = viewportRect;
    gui_text_depth@f2 = gui_text_depth;//(gui_text_depth.r, gui_text_depth.g, gui_cockpit_depth, 0, 0);
    mask_matrix_line_0@f3 = mask_matrix_line_0;
    mask_matrix_line_1@f3 = mask_matrix_line_1;
  }

  (ps) { mask_tex@smp2d = mask_tex; }

  hlsl {
    #define OGL_SCREEN_POSITION 
    #define OGL_SET_SCREEN_POSITION(OUT, OUTpos) 
    #define OGL_DEPTH_TEST(VSOUT)
  }

  if (hasTexture >= texFontL8)
  {
    blend_src = one; blend_dst = isa;
    (ps) { font@smp2d = tex; }
    if (fontFxType != no)
    {
      (vs) { fontFxOfs@f4 = fontFxOfs; }
      (ps) {
        fontFxOfs@f4 = fontFxOfs;
        fontFxColor@f4 = fontFxColor;
      }
      if (fontFxScale != sharp)
      {
        (vs) { fontFxScale@f1 = (fontFxScale); }
        if (fontFxType == glow) {
          (ps) { scales@f4 = (0.6, 0.25, 0.125, fontFxScale); }
        } else {
          (ps) { scales@f4 = (0.3, 0.25*0.6, 0.125*0.7, fontFxScale); }
        }
      }
    }
    if (fontTex2 != NULL)
    {
      (ps) { fontTex2@smp2d = fontTex2; }
      (vs) {
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
    (ps) { tex@smp2d = tex; }
    if (useColorMatrix == cmUsed)
    {
      (ps) { colorMatrix0@f4 = colorMatrix0; }
      (ps) { colorMatrix1@f4 = colorMatrix1; }
      (ps) { colorMatrix2@f4 = colorMatrix2; }
      (ps) { colorMatrix3@f4 = colorMatrix3; }
    }
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

    ##if mask_tex != NULL
      float2 maskTexcoord : TEXCOORD5;
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
      float2 src_vpos = fpos;

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
      src_vpos += (ofs*fontFxOfs.xy + add)*4.0;

    ##elif hasTexture != texOff
      o.tc = v.tc0/4096.0;
    ##endif
      o.pos = float4((src_vpos*viewport.xy+viewport.zw) * gui_text_depth.y, gui_text_depth.xy);
      o.col = BGRA_SWIZZLE(v.col);

    ##if hasTexture >= texFontL8 && fontTex2 != NULL
      float2 otc = fpos.xy + fontTex2ofs.zw;
      o.f2_tc = (otc.xy*fontTex2rotCCSmS.xy + otc.yx*fontTex2rotCCSmS.zw) * fontTex2ofs.xy;
    ##endif
    ##if mask_tex != NULL
      o.maskTexcoord.x = dot(src_vpos, mask_matrix_line_0.xy) + mask_matrix_line_0.z;
      o.maskTexcoord.y = dot(src_vpos, mask_matrix_line_1.xy) + mask_matrix_line_1.z;
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
        fixed4 result = h4tex2Dlod(tex, float4(input.tc,0,gui_tex_mip));
      ##if useColorMatrix == cmUsed
        result = float4(dot(colorMatrix0,result), dot(colorMatrix1,result), dot(colorMatrix2,result), dot(colorMatrix3,result));
      ##endif
        result *= input.col;
      ##if mask_tex != NULL
        result *= h4tex2D(mask_tex, input.maskTexcoord).r;
      ##endif
        clip_alpha(result.a);
        return result;
      }
    }
    compile("target_ps", "gui_default_ps_1");
  }
  else if (hasTexture >= texFontL8)
  {
    hlsl(ps) {
      half gamma_font(half f)
      {
        return pow(f, 1./1.5);//it is Alexey's Borisov's driven gamma (instead of correct 2.2 or more faster 2)
//        return 1-pow2(1-f);//faster and similair
        //return f;
      }

    ##if fontFxType == no
      fixed4 font_draw_ps(VsOutput v): SV_Target
      {
        OGL_DEPTH_TEST(v)
        fixed font_alpha = gamma_font(fx4tex2D(font,v.tc).r);
      ##if fontTex2 != NULL
        fixed4 t2 = fx4tex2D(fontTex2, v.f2_tc);
        fixed4 result = (fixed4(t2.rgb,1)*font_alpha*v.col*t2.a);
      ##else
        fixed4 result = font_alpha * v.col;
      ##endif
      ##if mask_tex != NULL
        result *= h4tex2D(mask_tex, v.maskTexcoord).r;
      ##endif
        clip_alpha(result.a);
        return result;
      }

    ##else
      half select(half4 tc_xyxy, half4 box_ltrb, half inside, half outside)
      {
        return all(tc_xyxy > box_ltrb ? bool4(1,1,0,0) : bool4(0,0,1,1)) ? inside : outside;
      }
      #define stepCount 3
      float GaussianBlur( float2 centreUV, float pixelXOffset, float4 box)
      {
        float colOut = 0;

        // Kernel width 11 x 11
        float gWeights[stepCount]={0.329654,0.157414,0.0129324};
        float gOffsets[stepCount]={0.622052,2.274,4.14755};

        UNROLL
        for( int i = 0; i < stepCount; i++ )
        {
          float texCoordOffset = gOffsets[i] * pixelXOffset;
          float2 tc;
          float col;
          tc = centreUV + float2(texCoordOffset,0); col  = select(tc.xyxy, box, h4tex2Dlod(font, float4(tc, 0, 0)).x, 0);
          tc = centreUV - float2(texCoordOffset,0); col += select(tc.xyxy, box, h4tex2Dlod(font, float4(tc, 0, 0)).x, 0);
          colOut += gWeights[i] * col;
        }
        return colOut;
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

          half4 result = lerp(saturate(l_shadow) * fontFxColor, f_col, gamma_font(l_font));
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
          ##if (fontFxScale == extraSmooth)
            l_shadow = 0;
            float2 tcShadowOfs = v.tc1.xy-v.tc.xy;
            tc = v.tc.xy + sign(tcShadowOfs)*round(abs(tcShadowOfs)/fontFxOfs.zw)*fontFxOfs.zw;
            #if stepCount != 3
              WE have to keep same kernel in horizontal and vertical pass!!!
            #endif
            float gWeights[stepCount]={0.329654,0.157414,0.0129324};
            float gOffsets[stepCount]={0.622052,2.274,4.14755};//todo: offsets can be premultiplied

            UNROLL
            for( int i = 0; i < stepCount; i++ )
            {
              float texCoordOffset = gOffsets[i]*fontFxOfs.w;
              float col = GaussianBlur(float2(tc.x, tc.y - texCoordOffset), fontFxOfs.z, v.tc2) +
                          GaussianBlur(float2(tc.x, tc.y + texCoordOffset), fontFxOfs.z, v.tc2);
              l_shadow += gWeights[i] * col;
            }
          ##else
            half l_shadow2 = 0, l_shadow3 = 0;
            tc = v.tc1+float2(-0.707, -0.707)*tcScale;
            l_shadow2 += select(tc.xyxy, v.tc2, h4tex2D(font, tc).r, 0);
            tc = v.tc1+float2( 0.707, -0.707)*tcScale;
            l_shadow2 += select(tc.xyxy, v.tc2, h4tex2D(font, tc).r, 0);
            tc = v.tc1+float2(-0.707,  0.707)*tcScale;
            l_shadow2 += select(tc.xyxy, v.tc2, h4tex2D(font, tc).r, 0);
            tc = v.tc1+float2( 0.707,  0.707)*tcScale;
            l_shadow2 += select(tc.xyxy, v.tc2, h4tex2D(font, tc).r, 0);

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
            // or l_shadow2*=0.5; l_shadow3 = 0;
            l_shadow = saturate(l_shadow*scales.x + l_shadow2*scales.y + l_shadow3*scales.z);
          ##endif
        ##endif

        ##if fontTex2 != NULL
          l_shadow *= h4tex2D(fontTex2, v.f2_tc+v.tc1-v.tc).a;
        ##endif

        ##if hasTexture == texFontL8v
          clip(l_font+(1-l_font)*l_shadow*250-0.5);
        ##endif
          //gamma correct lerp
          half4 result = lerp(l_shadow*fontFxColor, f_col, gamma_font(l_font));
        ##if mask_tex != NULL
          result *= h4tex2D(mask_tex, v.maskTexcoord).r;
        ##endif
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
        half4 result = input.col;
      ##if mask_tex != NULL
        result *= h4tex2D(mask_tex, input.maskTexcoord).r;
      ##endif
        return result;
      }
    }
    compile("target_ps", "gui_default_ps_3");
  }
}
