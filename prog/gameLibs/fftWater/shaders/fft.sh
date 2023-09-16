include "shader_global.sh"

float current_water_time = 0;
int fft_size = 128;
texture omega_tex;
texture k_tex;

int water_cascades = 5;
interval water_cascades : two<3, three<4, four<5, five<6, one_to_four;

int fft_spectrum = 0;
interval fft_spectrum : phillips < 1, unified_directional;

hlsl {
  ##if water_cascades == five
  #define FFT_TILE_MUL 3
  #define FFT_INV_TILE_MUL (1./FFT_TILE_MUL)
  ##elif (water_cascades != two)
  #define FFT_TILE_MUL 2
  #define FFT_INV_TILE_MUL (1./FFT_TILE_MUL)
  ##else
  #define FFT_INV_TILE_MUL 1
  #define FFT_TILE_MUL 1
  ##endif
}

macro USE_UPDATE_HT()
  (ps) {
    omega_tex@smp2d = omega_tex;
    k_tex@smp2d = k_tex;
  }
endmacro

texture butterfly_tex;
float butterfly_index = 0;

macro FFT_OFFSETS()
endmacro

shader update_ht
{
  cull_mode=none;
  z_test=false;
  z_write=false;

  hlsl {
    #define AM_TWO_PI      (6.283185307179586)
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float4 omegaTc : TEXCOORD0;
      //float2 kxky : TEXCOORD1;
    };
  }

  (ps) { ht0_scale_dt@f4 = (1.0/(fft_size+1.0), fft_size, current_water_time/6.283185307179586, fft_size); }

  USE_UPDATE_HT()
  hlsl(ps) {
    struct MRT_OUTPUT
    {
      float4 out0 : SV_Target0;
      float4 out1 : SV_Target1;
      float4 out2 : SV_Target2;
    };

    MRT_OUTPUT update_ht_ps(VsOutput IN)
    {
      MRT_OUTPUT result;
      float2 sn,cn;

      float fft_size = ht0_scale_dt.y;

      float2 odt0 = tex2D(omega_tex, IN.omegaTc.xy).xy*ht0_scale_dt.z;
      ##if (water_cascades == two)
      float omegaTcParts = 0;
      float2 omegaTcFrac = IN.omegaTc.zw;
      ##else
      float omegaTcParts = floor(IN.omegaTc.z);//move to interpolant!
      float2 omegaTcFrac = float2(IN.omegaTc.z-omegaTcParts, IN.omegaTc.w);
      ##endif

      float2 kTc = floor(omegaTcFrac*ht0_scale_dt.ww+float2(0.00001,0.00001));
      float2 mkTc = (ht0_scale_dt.yy - kTc)*ht0_scale_dt.x;
      kTc *= ht0_scale_dt.x;

      ##if (water_cascades == two)
      kTc = kTc + float2(0.0001, 0.0001);
      mkTc = mkTc + float2(0.0001, 0.0001);
      ##else
      kTc = float2(kTc.x*FFT_INV_TILE_MUL+FFT_INV_TILE_MUL*omegaTcParts, kTc.y) + float2(0.0001, 0.0001);//0.5*omegaTcParts - hardcoded number of tiles
      mkTc = float2(mkTc.x*FFT_INV_TILE_MUL+FFT_INV_TILE_MUL*omegaTcParts, mkTc.y) + float2(0.0001, 0.0001);
      ##endif
      float2 kxky = omegaTcFrac*fft_size - fft_size*0.5;

      float2 o = frac(odt0);
      sincos( o*AM_TWO_PI, sn, cn);
      #define GET_MK(no, noRT2, noRT, XX, YY)\
      {\
        float2 mk = tex2D(k_tex, mkTc).XX##YY;\
        float2 _k = tex2D(k_tex, kTc).XX##YY;\
        float2 s01 = _k+mk, d01 = _k-mk;\
        float hx = s01.x*cn[no] - s01.y*sn[no];\
        float hy = d01.x*sn[no] + d01.y*cn[no];\
        result.noRT.XX = hx;\
        result.noRT.YY = hy;\
        float s = length(kxky);\
        s = s > 1e-12f ? rcp(s) : 0;\
        hy *= s;\
        hx *= s;\
        result.noRT2.x = -kxky.x*hy;\
        result.noRT2.y =  kxky.x*hx;\
        result.noRT2.z = -kxky.y*hy;\
        result.noRT2.w = +kxky.y*hx;\
      }
      GET_MK(0, out1, out0, x, y);
      GET_MK(1, out2, out0, z, w);

      //height


      return result;
    }
  }
  USE_POSTFX_VERTEX_POSITIONS()

  FFT_OFFSETS()
  hlsl(vs) {
    VsOutput update_ht_vs(uint vertex_id : SV_VertexID)
    {
      VsOutput output;
      float2 pos = getPostfxVertexPositionById(vertex_id);
      output.pos = float4(pos.x, pos.y, 1, 1);
      output.omegaTc.xy = screen_to_texcoords(pos);
      output.omegaTc.zw = float2(output.omegaTc.x*FFT_TILE_MUL, output.omegaTc.y);//hardcoded tile
      //output.kxky = pos.xy*texsz_consts.z;
      return output;
    }
  }
  compile("target_vs", "update_ht_vs");
  compile("target_ps", "update_ht_ps");
}

int fft_source_texture_no = 4;
macro USE_FFT()
  hlsl(ps) {
    Texture2D fft_source_texture0 : register(t4);
    SamplerState fft_source_texture0_samplerstate : register(s4);
    Texture2D fft_source_texture1 : register(t5);
    SamplerState fft_source_texture1_samplerstate : register(s5);
    Texture2D fft_source_texture2 : register(t6);
    SamplerState fft_source_texture2_samplerstate : register(s6);
  }
endmacro

shader fftH, fftV
{
  channel float3 pos = pos;
  cull_mode=none;
  z_test=false;
  z_write=false;

  hlsl {
    #define AM_TWO_PI      (6.283185307179586)
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float3 tc__butterfly_index : TEXCOORD0;
    };
  }

  USE_FFT()

  (ps) { butterfly_tex@smp2d = butterfly_tex; }

  hlsl(ps) {
    struct MRT_OUTPUT
    {
      float4 out0 : SV_Target0;
      float4 out1 : SV_Target1;
      float4 out2 : SV_Target2;
    };

    MRT_OUTPUT fft_ps(VsOutput IN)
    {
      MRT_OUTPUT result;
      float butterfly_index = IN.tc__butterfly_index.z;
      float2 inTc = IN.tc__butterfly_index.xy;
      ##if (water_cascades != two)
      float flooredTcX = floor(inTc.x);
      float fracTcX = inTc.x - flooredTcX;
      ##else
      float fracTcX = inTc.x;
      float flooredTcX = 0;
      ##endif
      ##if shader == fftV
      float4 vIndexAndWeight = tex2D(butterfly_tex, float2(inTc.y, butterfly_index));
      ##else
      float4 vIndexAndWeight = tex2D(butterfly_tex, float2(fracTcX, butterfly_index));
      ##endif
      float2 vIndex = vIndexAndWeight.xy;
      float2 vWeight = vIndexAndWeight.zw;

      ##if shader == fftV
        float tcX = inTc.x*FFT_INV_TILE_MUL;
        float2 vSourceATc = float2(tcX, vIndex.x);
        float2 vSourceBTc = float2(tcX, vIndex.y);
      ##else
        vIndex.xy = (vIndex.xy+float2(flooredTcX,flooredTcX))*FFT_INV_TILE_MUL;
        float2 vSourceATc = float2(vIndex.x, inTc.y);
        float2 vSourceBTc = float2(vIndex.y, inTc.y);
      ##endif
      #define  GET_FFT(no)\
      {\
        float4 vSourceA = tex2D(fft_source_texture##no, vSourceATc);\
        float4 vSourceB = tex2D(fft_source_texture##no, vSourceBTc);\
        float4 vWeightedB;\
        vWeightedB.xz = vWeight.r * vSourceB.xz - vWeight.g * vSourceB.yw;\
        vWeightedB.yw = vWeight.g * vSourceB.xz + vWeight.r * vSourceB.yw;\
        float4 vComplex = vSourceA + vWeightedB;\
        result.out##no = vComplex;\
      }
      GET_FFT(0);
      GET_FFT(1);
      GET_FFT(2);
      return result;
    }
  }
  DECL_POSTFX_TC_VS_RT()
  FFT_OFFSETS()
  hlsl(vs) {
    VsOutput fft_vs(float3 pos:POSITION)
    {
      VsOutput output;
      output.pos = float4(pos.xy, 1, 1);
      output.tc__butterfly_index.xy = pos.xy*RT_SCALE_HALF+float2(0.50001,0.50001);
      ##if water_cascades != two
      output.tc__butterfly_index.x *= FFT_TILE_MUL;
      ##endif
      output.tc__butterfly_index.z = pos.z;
      return output;
    }
  }
  compile("target_vs", "fft_vs");
  compile("target_ps", "fft_ps");
}

float4 choppy_scale0123 = (1, 1, 1, 1);
float4 choppy_scale4567 = (1, 1, 1, 1);
shader fft_displacements
{
  cull_mode=none;
  z_test=false;
  z_write=false;

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float2 index_tc : TEXCOORD0;
      float4 tc01 : TEXCOORD1;
    };
  }
  USE_FFT()

  (ps) {
    choppy_scale0123@f4 = choppy_scale0123;
  }
  if (water_cascades == five)
  {
    (ps) { choppy_scale4567@f4 = choppy_scale4567; }
  }

  hlsl(ps) {
    struct MRT_OUTPUT
    {
      float4 out0 : SV_Target0;
      ##if water_cascades != one_to_four
        float4 out1 : SV_Target1;
        ##if water_cascades != two
          float4 out2 : SV_Target2;
          ##if water_cascades != three
            float4 out3 : SV_Target3;
            ##if water_cascades != four
              float4 out4 : SV_Target4;
            ##endif
          ##endif
        ##endif
      ##endif
    };

    MRT_OUTPUT fft_displacement_ps(VsOutput IN)
    {
      MRT_OUTPUT result;
      int2 index = (int2)floor(IN.index_tc);

      // cos(pi * (m1 + m2))
      float sign_correction = ((index.x+index.y)&1) ? -1.f : 1.f;

      //float2 tc0 = float2(IN.tc.x * 0.5, IN.tc.y);//hardcoded tile
      //float2 tc1 = float2(tc0.x + 0.5, IN.tc.y);
      float2 tc0 = IN.tc01.xy;
      float2 tc1 = IN.tc01.zy;
      float2 tc2 = IN.tc01.wy;
      float4 vSourceA, vSourceB, vSourceC;
      vSourceA = tex2D(fft_source_texture0, tc0);
      vSourceB = tex2D(fft_source_texture1, tc0);
      vSourceC = tex2D(fft_source_texture2, tc0);

      ##if water_cascades != two
        float4 dx,dy,dz;
        float4 vSourceD, vSourceE, vSourceF;
        vSourceD = tex2D(fft_source_texture0, tc1);
        vSourceE = tex2D(fft_source_texture1, tc1);
        vSourceF = tex2D(fft_source_texture2, tc1);
        float4 Dx = float4(vSourceB.x, vSourceC.x, vSourceE.x, vSourceF.x);
        float4 Dy = float4(vSourceB.z, vSourceC.z, vSourceE.z, vSourceF.z);
        float4 Dz = float4(vSourceA.x, vSourceA.z, vSourceD.x, vSourceD.z);
        dx = Dx * choppy_scale0123 * sign_correction;
        dy = Dy * choppy_scale0123 * sign_correction;
      ##else
        float2 dx,dy,dz;
        float2 Dx = float2(vSourceB.x, vSourceC.x);
        float2 Dy = float2(vSourceB.z, vSourceC.z);
        float2 Dz = float2(vSourceA.x, vSourceA.z);
        dx = Dx * choppy_scale0123.xy * sign_correction;
        dy = Dy * choppy_scale0123.xy * sign_correction;
      ##endif

      ##if water_cascades == five
        vSourceA = tex2D(fft_source_texture0, tc2);
        vSourceB = tex2D(fft_source_texture1, tc2);
        vSourceC = tex2D(fft_source_texture2, tc2);
        float2 dx45, dy45, dz45;
        float2 Dx45 = float2(vSourceB.x, vSourceC.x);
        float2 Dy45 = float2(vSourceB.z, vSourceC.z);
        float2 Dz45 = float2(vSourceA.x, vSourceA.z);
        dx45 = Dx45 * choppy_scale4567.xy * sign_correction;
        dy45 = Dy45 * choppy_scale4567.xy * sign_correction;
        dz45 = Dz45 * sign_correction;
      ##endif

      dz = Dz * sign_correction;
      result.out0 = float4(dx.x, dy.x, dz.x, 0);
      ##if water_cascades != one_to_four
        result.out1 = float4(dx.y, dy.y, dz.y, 0);
        ##if water_cascades != two
          result.out2 = float4(dx.z, dy.z, dz.z, 0);
          ##if water_cascades != three
            result.out3 = float4(dx.w, dy.w, dz.w, 0);
            ##if water_cascades != four
              result.out4 = float4(dx45.x, dy45.x, dz45.x, 0);
            ##endif
          ##endif
        ##endif
      ##endif

      return result;
    }
  }
  USE_POSTFX_VERTEX_POSITIONS()

  (vs) { texsz_consts@f4 = (fft_size, 0, 0, 0); }
  hlsl(vs) {
    VsOutput fft_displacement_vs(uint vertex_id : SV_VertexID)
    {
      VsOutput output;
      float2 pos = getPostfxVertexPositionById(vertex_id);
      output.pos = float4(pos.x, pos.y, 1, 1);
      float2 tc = screen_to_texcoords(pos).yx;
      float fft_size = texsz_consts.x;
      output.index_tc = tc*fft_size;
      output.tc01.xy = float2(tc.x * FFT_INV_TILE_MUL, tc.y);
      output.tc01.z = output.tc01.x + FFT_INV_TILE_MUL;
      output.tc01.w = output.tc01.x + FFT_INV_TILE_MUL * 2.0;
      //float2 tc0 = float2(IN.tc.x * 0.5, IN.tc.y);//hardcoded tile
      //float2 tc1 = float2(tc0.x + 0.5, IN.tc.y);
      return output;
    }
  }
  compile("target_vs", "fft_displacement_vs");
  compile("target_ps", "fft_displacement_ps");
}

texture gauss_tex;
float wind_dir_x = 0.6;
float wind_dir_y = 0.8;
float small_waves_fraction = 0;

shader update_ht0
{
  channel float2 pos = pos;
  channel float2 tc[0] = tc[0];
  channel float4 tc[1] = tc[1];
  channel float4 tc[2] = tc[2];
  channel float4 tc[3] = tc[3];
  channel float4 tc[4] = tc[4];
  channel float4 tc[5] = tc[5];
  cull_mode=none;
  z_test=false;
  z_write=false;

  hlsl {
    #define AM_TWO_PI      (6.283185307179586)
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float2 tc : TEXCOORD0;//normal, gauss
      float4 fft_tc : TEXCOORD1;//normal, gauss
      float4 gaussTc_norm : TEXCOORD2;//normal, gauss
      float4 windows : TEXCOORD3;
      float4 ampSpeed : TEXCOORD4;
      float4 windProp : TEXCOORD5;
      //float2 kxky : TEXCOORD1;
    };
  }

  (ps) {
    wind@f4 = (wind_dir_x, wind_dir_y, small_waves_fraction, 0.0);
    gauss_tex@smp2d = gauss_tex;
  }

  hlsl(ps) {
    #include "fft_spectrum.hlsli"

    struct MRT_OUTPUT
    {
      float4 out0 : SV_Target0;
    };

##if fft_spectrum == phillips
    #define calcSpectrum calcPhillips
##elif fft_spectrum == unified_directional
    #define calcSpectrum calcUnifiedDirectional
##endif

    float calcH0( float nr,
                  float2 K,
                  float window_in, float window_out,
                  float2 wind_dir, float v, float dir_depend,
                  float a,
                  float norm,
                  float small_wave_fraction,
                  float wind_align
                  )
    {
      // distance from DC, in wave-numbers

      float amplitude = sqrt(calcSpectrum(K, wind_dir, v, a, dir_depend, small_wave_fraction, wind_align));
      amplitude *= norm;
      amplitude = nr < window_in ? 0.0 : nr >= window_out ? 0.0f : amplitude;

      return amplitude;
    }
    MRT_OUTPUT update_ht0_ps(VsOutput IN)
    {
      MRT_OUTPUT result;

      float2 nxny = IN.tc.xy;

      float2 wind_dir = wind.xy;

      float2 norm = IN.gaussTc_norm.zw;
      float small_waves_fraction = wind.z;

      float2 gauss = tex2D(gauss_tex, IN.gaussTc_norm.xy).xy;
      float nr = length(nxny);

      float amplitude0 = calcH0(nr, IN.fft_tc.xy, IN.windows.x, IN.windows.y, wind_dir, IN.ampSpeed.y, IN.windProp.x, IN.ampSpeed.x, norm.x, small_waves_fraction, IN.windProp.z);
      float amplitude1 = calcH0(nr, IN.fft_tc.zw, IN.windows.z, IN.windows.w, wind_dir, IN.ampSpeed.w, IN.windProp.y, IN.ampSpeed.z, norm.y, small_waves_fraction, IN.windProp.w);

      result.out0.x = amplitude0 * gauss.x;
      result.out0.y = amplitude0 * gauss.y;

      result.out0.z = amplitude1 * gauss.x;
      result.out0.w = amplitude1 * gauss.y;
      
      return result;
    }
  }

  hlsl(vs) {
    struct VSInput
    {
      float2 pos:POSITION;
      float2 tc:TEXCOORD0;
      float4 fft_tc:TEXCOORD1;
      float4 gaussTc_norm:TEXCOORD2;
      float4 windows:TEXCOORD3;
      float4 ampSpeed:TEXCOORD4;
      float4 windProp:TEXCOORD5;
    };
    VsOutput update_ht0_vs(VSInput IN)
    {
      VsOutput output;
      output.pos = float4(IN.pos.xy, 1, 1);
      output.tc = IN.tc;
      output.fft_tc = IN.fft_tc;
      output.gaussTc_norm = IN.gaussTc_norm;
      output.windows = IN.windows;
      output.ampSpeed = IN.ampSpeed;
      output.windProp = IN.windProp;
      //output.kxky = pos.xy*texsz_consts.z;
      return output;
    }
  }
  compile("target_vs", "update_ht0_vs");
  compile("target_ps", "update_ht0_ps");
}

