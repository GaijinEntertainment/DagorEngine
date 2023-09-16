include "shader_global.sh"
include "water_foam_trail_inc.sh"
include "snoise.sh"

texture foam_tex;
texture ship_bow_foam;
texture water_foam_obstacle;
texture perlin_noise3d;

float current_water_time = 0;

float wake_mask_size = 0;
float4 wake_mask_params0 = (0,0,0,0);
float4 wake_mask_params1 = (0,0,0,0);

float water_foam_trail_gen;
float4 water_foam_trail_params;

float4 water_foam_obstacle_mat0;
float4 water_foam_obstacle_mat1;
float4 water_foam_obstacle_params;

int water_foam_obstacle_pass = 0;
interval water_foam_obstacle_pass: first < 1, second;

int water_foam_obstacle_tex_size = 0;
interval water_foam_obstacle_tex_size: size_256 < 1, size_512 < 2, size_1024 < 3, size_2048 < 4, size_4096;

shader water_foam_trail
{
  blend_src = one;
  blend_dst = sa;
  blend_asrc = one;
  blend_adst = sa;

  (ps) { water_foam_trail_mask@smp2d = water_foam_trail_mask; }

  INIT_WATER_FOAM_TRAIL()
  USE_MODULATED_FOAM()
  USE_WATER_FOAM_TRAIL()

  INIT_HDR_STCODE()
  USE_HDR_PS()

  cull_mode = none;
  z_test = false;
  z_write = false;

  channel float4 pos = pos;
  channel float4 tc[0] = tc[0];
  channel float4 tc[1] = tc[1];

  (vs) {
    globtm@f44 = globtm;
    params@f4 = (water_foam_trail_params.x, water_foam_trail_params.y, water_foam_trail_params.z, water_foam_trail_gen);
  }
  (ps) { params@f4 = (water_foam_trail_params.x, water_foam_trail_params.y, water_foam_trail_params.z, water_foam_trail_gen); }

  hlsl {

    // #define WATER_FOAM_TRAIL_DEBUG_WIRE

    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float4 grd : TEXCOORD0;
      float4 prm : TEXCOORD1;

      #ifdef WATER_FOAM_TRAIL_DEBUG_WIRE
      float2 wire : TEXCOORD2;
      #endif

      float2 locTc : TEXCOORD3;
    };
  }
  hlsl(vs) {
    struct VsInput
    {
      float4 pos : POSITION;  // xy:pos, z:gen, w:dist
      float4 dir : TEXCOORD0; // xy:dir, z:side + alpha, w: dot(vel, dir)
      float4 prm : TEXCOORD1; // x:foam mul, y:insat mul, z:1/gen mul, w: step (debug)
    };
  }

  hlsl(vs) {
    #define fadein_rcp params.x
    #define fadeout_rcp params.y
    #define fwd_expand params.z
    #define current_gen params.w
    #define dist_remap 0.015f

    VsOutput foam_vs(VsInput input)
    {
      VsOutput output;

      float2 pos = float2( input.pos.x, input.pos.y );
      float gen = input.pos.z;
      float side = sign( input.dir.z );
      float alpha = abs( input.dir.z );

      float step = sign( input.prm.w );
      float finalized_gen = abs( input.prm.w ) - 1.f;

      float gen_delta = ( current_gen - gen ) * input.prm.z;
      float fadein = saturate( gen_delta * fadein_rcp );
      float fadeout = 1.f - saturate( gen_delta * fadeout_rcp );

      float width = lerp( 1.f, fwd_expand, ( 1.f - fadeout ) * input.dir.w );
      pos += input.dir.xy * width * 1.5;

      if ( finalized_gen > 0.01 )
        alpha *= 1.f - ( current_gen - finalized_gen ) * input.prm.z * 0.025;

      output.pos = mul( float4( pos.x, 0.f, pos.y, 1.f ), globtm );
      output.grd.x = saturate( side );
      output.grd.y = input.pos.w * dist_remap;
      output.grd.z = alpha;
      output.grd.w = side;

      output.prm.xy = input.prm.xy;
      output.prm.z = gen;
      output.prm.w = input.dir.w;

      #ifdef WATER_FOAM_TRAIL_DEBUG_WIRE
      output.wire.x = side;
      output.wire.y = step;
      #endif

      output.locTc = pos.xy;

      return output;
    }
  }

  if (compatibility_mode == compatibility_mode_off)
  {
    (ps) { foam_tex@smp2d = foam_tex; }
  }

  hlsl(ps)
  {
    #define fadein_rcp params.x
    #define fadeout_rcp params.y
    #define current_gen params.w

    #define disc_v 0.001f

    float4 foam_ps(VsOutput input) : SV_Target0
    {
      float gen_delta = ( current_gen - input.prm.z );
      float fadein = saturate( gen_delta * fadein_rcp );
      float fadeout = 1.f - saturate( gen_delta * fadeout_rcp );
      float alpha = fadein * fadeout * input.grd.z;
      float turn_k = input.prm.w;

      if ( alpha < disc_v )
        discard;

      float2 tc = float2( input.grd.x, -input.grd.y );
      float4 src = tex2D( water_foam_trail_mask, tc );

      float3 foam_mat = lerp( float3( 0.5f, 0.5f, 0.f ), float3( 0.5f, 0.75f, 1.f ), input.prm.x );

      // mixing
      float3 dst;
      float grad = 1.f - abs( tc.x * 2.f - 1.f );
      // short range
      dst.x = saturate( ( src.y + src.x ) * pow2( grad * 2.f ) ) * saturate( pow2( alpha + 0.25f ) );
      // medium range
      dst.y = src.y * src.x * 2.f * lerp( pow2( src.w ), 1.f, saturate( pow2( alpha + 0.1f ) ) );
      // long range
      dst.z = pow4( src.w ) * 1.5f * pow2( saturate( turn_k + 0.1f ) );

      float res = dot( dst, foam_mat ) * alpha;

#ifdef WATER_FOAM_TRAIL_DEBUG_WIRE
      float xx = pow( abs( input.wire.x ), 128 );
      float yy = pow( abs( input.wire.y ), 128 );
      res += ( xx + yy ).xxxx * 0.5;
#endif

##if compatibility_mode == compatibility_mode_off
      float foamLowFreq = tex2D(foam_tex, input.locTc.xy * 0.1).r;
      res = get_modulated_foam(res * 1.5, foamLowFreq);
##endif

      return half4(pack_hdr(res.xxx).rgb, 1.0 - res);
    }
  }
  compile( "target_vs", "foam_vs" );
  compile( "target_ps", "foam_ps" );
}

shader water_foam_obstacle_apply
{
  cull_mode = none;
  z_test = false;
  z_write = false;

  if (water_foam_obstacle_pass == second)
  {
    blend_src = one;
    blend_dst = sa;
    blend_asrc = one;
    blend_adst = sa;
  }
  else
  {
    blend_src = one;
    blend_dst = one;
  }

  (ps) {
    tex@smp2d = water_foam_obstacle;
    perlin_noise3d@smp3d = perlin_noise3d;

    water_foam_trail_mask@smp2d = water_foam_trail_mask;
    water_foam_obstacle_params@f4 = water_foam_obstacle_params;
    current_water_time@f1 = (current_water_time);
    wake_mask_params0@f4 = (wake_mask_params0.x, wake_mask_params0.y, wake_mask_params0.z, wake_mask_size);
    wake_mask_params1@f4 = wake_mask_params1;
  }
  (vs) {
    water_foam_obstacle_mat0@f4 = water_foam_obstacle_mat0;
    water_foam_obstacle_mat1@f4 = water_foam_obstacle_mat1;
    globtm@f44 = globtm;
  }

  if ( ship_bow_foam != NULL )
  {
    (ps) { ship_bow@smp2d = ship_bow_foam; }
  }

  hlsl {
    struct VsOutput
    {
      VS_OUT_POSITION(pos)
      float4 texcoord : TEXCOORD0;
    };
  }

  USE_POSTFX_VERTEX_POSITIONS()

  INIT_HDR_STCODE()
  USE_HDR_PS()

  hlsl(vs)
  {
    VsOutput target_vs(uint vertexId : SV_VertexID)
    {
      VsOutput output;
      float2 pos = getPostfxVertexPositionById(vertexId);
      float3 p3 = float3( pos.xy, 0.f );
      float4 p4 = float4( p3.xyz, 1.f );
      float2 wp;

      ##if water_foam_obstacle_pass == first
      output.pos = p4;
      ##else
      wp.x = dot( p4, water_foam_obstacle_mat0 );
      wp.y = dot( p4, water_foam_obstacle_mat1 );
      output.pos = mul( float4( wp.x, 0.f, wp.y, 1.f ), globtm );
      ##endif

      output.texcoord.xy = screen_to_texcoords( pos );
      output.texcoord.z = dot( float4( p3, 0.f ), water_foam_obstacle_mat0 );
      output.texcoord.w = dot( float4( p3, 0.f ), water_foam_obstacle_mat1 );

      return output;
    }
  }

  hlsl(ps)
  {
    float4 target_ps(VsOutput input) : SV_Target
    {
      ##if water_foam_obstacle_tex_size == size_256
      float tx = 1.f / 128.f;
      #define first -1
      #define last 1
      ##elif water_foam_obstacle_tex_size == size_512
      float tx = 1.f / 256.f;
      #define first -2
      #define last 2
      ##elif water_foam_obstacle_tex_size == size_1024
      float tx = 1.f / 512.f;
      #define first -4
      #define last 4
      ##elif water_foam_obstacle_tex_size == size_2048
      float tx = 1.f / 1024.f;
      #define first -8
      #define last 8
      ##elif water_foam_obstacle_tex_size == size_4096
      float tx = 1.f / 2024.f;
      #define first -16
      #define last 16
      ##endif

      float res = 0.f;
      UNROLL
      for ( int i = first ; i < last ; ++i )
      {
##if water_foam_obstacle_pass == first
        float2 mask = 1.f - tex2D( tex, input.texcoord.xy + float2( tx * i, 0.f ) ).rg;
        res += mask.r * mask.g; // AND op
##else
        float2 ttc = input.texcoord.xy + float2( 0.f, tx * i );
        res += tex2D( tex, ttc ).r;
##endif
      }

      res *= rcp((float)(last - first)) * ( 0.5f + water_foam_obstacle_params.y );

      #undef first
      #undef last

##if water_foam_obstacle_pass == first
      float alpha = 1.f;
##else
      float alpha = dot( normalize( input.texcoord.zw ), -water_foam_obstacle_params.zw );
      alpha = 1.f - pow4( pow2( saturate( alpha ) ) * water_foam_obstacle_params.y );
##endif

      res *= alpha;

##if water_foam_obstacle_pass == second && ship_bow_foam != NULL

      float turn = saturate( abs( wake_mask_params0.y ) * 20 - 19.f );
      float spd = saturate( ( wake_mask_params0.z - 5.f ) * 0.15f );

      float2 tc = -input.texcoord.zw;

      float orig_size = 1.f / wake_mask_params0.w;
      float reg_size = orig_size * lerp( 0.75f, 0.25f, saturate( orig_size * 0.01f ) );

      tc *= rcp( reg_size );

      float2 ltc;
      ltc.x = tc.x * wake_mask_params1.x + tc.y * wake_mask_params1.y;
      ltc.y = tc.y * wake_mask_params1.x - tc.x * wake_mask_params1.y;
      ltc.x = -ltc.x;

      const float ofs = 0.15f;
      ltc.x += ( ( reg_size - orig_size ) * 0.5f ) * rcp( reg_size );
      ltc.x -= ofs * ( ( reg_size - orig_size ) * rcp( reg_size ) );

      float kk = pow4( 1.f - ltc.x );
      ltc += float2( 0.5f, 0.5f );

      float perlT = current_water_time * 0.1 + ltc.x - abs(ltc.y - 0.5);
      float2 perlTc = ltc * 4;
      float perl = dot(float3(1, 1, 1), tex3Dlod(perlin_noise3d, float4(frac(perlT), frac(perlTc), 0)).xyz) - 1.5;
      perl += dot(float3(1, 1, 1), tex3Dlod(perlin_noise3d, float4(frac(perlT * 2.71828), frac(perlTc * 2.71828), 0)).xyz) - 1.5;

      ltc.x += (0.01 + 0.03 * abs(ltc.y - 0.5)) * perl;

      const float sinFreq = 5.;
      const float tc_to_phase = 50.;

      const float start_point = 0.3355;
      float distort = start_point - wake_mask_params0.x * wake_mask_params0.w;

      ltc.x += distort * ( 1.f - abs( ltc.y - 0.5 ) * 2.f ) * pow2( saturate( ( ltc.x - 0.5 ) * 2.f ) );

      #define F(xy, phase, sinAmp)  saturate(10 * abs(xy.y - 0.5)) * sinAmp * float2(cos(sinFreq * current_water_time * phase + (xy.x - sign(xy.y - 0.5) * xy.y) * tc_to_phase / phase), sin(sinFreq * current_water_time * phase + (xy.x - sign(xy.y - 0.5) * xy.y) * tc_to_phase / phase))

      ltc = saturate( ltc );

      float t0 = tex2D( ship_bow, ltc + F(ltc, 1.0, 0.003) ).x;
      float t1 = tex2D( ship_bow, ltc + F(ltc, 1.31415, 0.003) ).x;
      float resWake = (t0 + t1) * 0.75f;
      //resWake = pow2(smoothstep(0, 1, resWake));

      float fade = pow4( saturate( wake_mask_params1.z + 0.01f ) );
      fade = fade + pow4( saturate( 1.f - ( ltc.x - 0.5f ) * 2.f ) ) * 0.5f;
      fade = saturate( fade );

      float mul = saturate(turn * spd * fade);
      resWake *= mul * saturate((mul - 2 * abs(ltc.y - 0.5)) * 5);


      //float perlMask = saturate(perl + 1.1 - 2. * abs(ltc.y - 0.5));
      //resWake *= perlMask;

      res += resWake;

##endif

##if water_foam_obstacle_pass == second
      return half4(pack_hdr(res.xxx).rgb, 1.0 - res);
##else
      return res.xxxx;
##endif
    }
  }

  compile("target_vs", "target_vs");
  compile("target_ps", "target_ps");
}