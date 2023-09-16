// from: https://github.com/BrianSharpe/GPU-Noise-Lib/blob/master/gpu_noise_lib.glsl
float3 interpolation_c2( float3 x ) { return x * x * x * (x * (x * 6.0 - 15.0) + 10.0); }
float3 interpolation_c2_deriv( float3 x ) { return x * x * (x * (x * 30.0 - 60.0) + 30.0); }

void perlin_hash(float3 gridcell, float s, bool3 tile,
          out float4 lowz_hash_0,
          out float4 lowz_hash_1,
          out float4 lowz_hash_2,
          out float4 highz_hash_0,
          out float4 highz_hash_1,
          out float4 highz_hash_2)
{
  const float2 OFFSET = float2( 50.0, 161.0 );
  const float DOMAIN = 69.0;
  const float3 SOMELARGEFLOATS = float3( 635.298681, 682.357502, 668.926525 );
  const float3 ZINC = float3( 48.500388, 65.294118, 63.934599 );

  gridcell.xyz = gridcell.xyz - floor(gridcell.xyz * ( 1.0 / DOMAIN )) * DOMAIN;
  float d = DOMAIN - 1.5;
  float3 gridcell_inc1 = step( gridcell, float3( d,d,d ) ) * ( gridcell + 1.0 );

  #if SHADER_COMPILER_HLSL2021
    gridcell_inc1 = select(tile, gridcell_inc1 % s, gridcell_inc1);
  #else
    gridcell_inc1 = tile ? gridcell_inc1 % s : gridcell_inc1;
  #endif

  float4 P = float4( gridcell.xy, gridcell_inc1.xy ) + OFFSET.xyxy;
  P *= P;
  P = P.xzxz * P.yyww;
  float3 lowz_mod = float3( 1.0 / ( SOMELARGEFLOATS.xyz + gridcell.zzz * ZINC.xyz ) );
  float3 highz_mod = float3( 1.0 / ( SOMELARGEFLOATS.xyz + gridcell_inc1.zzz * ZINC.xyz ) );
  lowz_hash_0 = frac( P * lowz_mod.xxxx );
  highz_hash_0 = frac( P * highz_mod.xxxx );
  lowz_hash_1 = frac( P * lowz_mod.yyyy );
  highz_hash_1 = frac( P * highz_mod.yyyy );
  lowz_hash_2 = frac( P * lowz_mod.zzzz );
  highz_hash_2 = frac( P * highz_mod.zzzz );
}

// from: https://github.com/BrianSharpe/GPU-Noise-Lib/blob/master/gpu_noise_lib.glsl
float perlin(float3 P, float s, bool3 tile) {
  P *= s;

  float3 Pi = floor(P);
  float3 Pi2 = floor(P);
  float3 Pf = P - Pi;
  float3 Pf_min1 = Pf - 1.0;

  float4 hashx0, hashy0, hashz0, hashx1, hashy1, hashz1;
  perlin_hash( Pi2, s, tile, hashx0, hashy0, hashz0, hashx1, hashy1, hashz1 );

  float4 grad_x0 = hashx0 - 0.49999;
  float4 grad_y0 = hashy0 - 0.49999;
  float4 grad_z0 = hashz0 - 0.49999;
  float4 grad_x1 = hashx1 - 0.49999;
  float4 grad_y1 = hashy1 - 0.49999;
  float4 grad_z1 = hashz1 - 0.49999;
  float4 grad_results_0 = rsqrt( grad_x0 * grad_x0 + grad_y0 * grad_y0 + grad_z0 * grad_z0 ) * ( float2( Pf.x, Pf_min1.x ).xyxy * grad_x0 + float2( Pf.y, Pf_min1.y ).xxyy * grad_y0 + Pf.zzzz * grad_z0 );
  float4 grad_results_1 = rsqrt( grad_x1 * grad_x1 + grad_y1 * grad_y1 + grad_z1 * grad_z1 ) * ( float2( Pf.x, Pf_min1.x ).xyxy * grad_x1 + float2( Pf.y, Pf_min1.y ).xxyy * grad_y1 + Pf_min1.zzzz * grad_z1 );

  float3 blend = interpolation_c2( Pf );
  float4 res0 = lerp( grad_results_0, grad_results_1, blend.z );
  float4 blend2 = float4( blend.xy, float2( 1.0 - blend.xy ) );
  float final = dot( res0, blend2.zxzx * blend2.wwyy );
  final *= 1.0/sqrt(0.75);
  return final;
  //return final*0.5+0.5;
  //return ((final * 1.5) + 1.0) * 0.5;
}

float4 perlin_deriv(float3 P, float s, bool3 tile) {
  P *= s;

  //    establish our grid cell and unit position
  float3 Pi = floor(P);
  float3 Pf = P - Pi;
  float3 Pf_min1 = Pf - 1.0;

  //    calculate the hash.
  //    ( various hashing methods listed in order of speed )
  float4 hashx0, hashy0, hashz0, hashx1, hashy1, hashz1;
  perlin_hash( Pi, s, tile, hashx0, hashy0, hashz0, hashx1, hashy1, hashz1 );

  //    calculate the gradients
  float4 grad_x0 = hashx0 - 0.49999;
  float4 grad_y0 = hashy0 - 0.49999;
  float4 grad_z0 = hashz0 - 0.49999;
  float4 grad_x1 = hashx1 - 0.49999;
  float4 grad_y1 = hashy1 - 0.49999;
  float4 grad_z1 = hashz1 - 0.49999;
  float4 norm_0 = rsqrt( grad_x0 * grad_x0 + grad_y0 * grad_y0 + grad_z0 * grad_z0 );
  float4 norm_1 = rsqrt( grad_x1 * grad_x1 + grad_y1 * grad_y1 + grad_z1 * grad_z1 );
  grad_x0 *= norm_0;
  grad_y0 *= norm_0;
  grad_z0 *= norm_0;
  grad_x1 *= norm_1;
  grad_y1 *= norm_1;
  grad_z1 *= norm_1;

  //    calculate the dot products
  float4 dotval_0 = float2( Pf.x, Pf_min1.x ).xyxy * grad_x0 + float2( Pf.y, Pf_min1.y ).xxyy * grad_y0 + Pf.zzzz * grad_z0;
  float4 dotval_1 = float2( Pf.x, Pf_min1.x ).xyxy * grad_x1 + float2( Pf.y, Pf_min1.y ).xxyy * grad_y1 + Pf_min1.zzzz * grad_z1;

  //
  //    NOTE:  the following is based off Milo Yips derivation, but modified for parallel execution
  //    http://stackoverflow.com/a/14141774
  //

  //    Convert our data to a more parallel format
  float4 dotval0_grad0 = float4( dotval_0.x, grad_x0.x, grad_y0.x, grad_z0.x );
  float4 dotval1_grad1 = float4( dotval_0.y, grad_x0.y, grad_y0.y, grad_z0.y );
  float4 dotval2_grad2 = float4( dotval_0.z, grad_x0.z, grad_y0.z, grad_z0.z );
  float4 dotval3_grad3 = float4( dotval_0.w, grad_x0.w, grad_y0.w, grad_z0.w );
  float4 dotval4_grad4 = float4( dotval_1.x, grad_x1.x, grad_y1.x, grad_z1.x );
  float4 dotval5_grad5 = float4( dotval_1.y, grad_x1.y, grad_y1.y, grad_z1.y );
  float4 dotval6_grad6 = float4( dotval_1.z, grad_x1.z, grad_y1.z, grad_z1.z );
  float4 dotval7_grad7 = float4( dotval_1.w, grad_x1.w, grad_y1.w, grad_z1.w );

  //    evaluate common constants
  float4 k0_gk0 = dotval1_grad1 - dotval0_grad0;
  float4 k1_gk1 = dotval2_grad2 - dotval0_grad0;
  float4 k2_gk2 = dotval4_grad4 - dotval0_grad0;
  float4 k3_gk3 = dotval3_grad3 - dotval2_grad2 - k0_gk0;
  float4 k4_gk4 = dotval5_grad5 - dotval4_grad4 - k0_gk0;
  float4 k5_gk5 = dotval6_grad6 - dotval4_grad4 - k1_gk1;
  float4 k6_gk6 = (dotval7_grad7 - dotval6_grad6) - (dotval5_grad5 - dotval4_grad4) - k3_gk3;

  //    C2 Interpolation
  float3 blend = interpolation_c2( Pf );
  float3 blendDeriv = interpolation_c2_deriv( Pf );

  //    calculate final noise + deriv
  float u = blend.x;
  float v = blend.y;
  float w = blend.z;

  float4 result = dotval0_grad0
      + u * ( k0_gk0 + v * k3_gk3 )
      + v * ( k1_gk1 + w * k5_gk5 )
      + w * ( k2_gk2 + u * ( k4_gk4 + v * k6_gk6 ) );

  result.y += dot( float4( k0_gk0.x, k3_gk3.x * v, float2( k4_gk4.x, k6_gk6.x * v ) * w ), float4( blendDeriv.xxxx ) );
  result.z += dot( float4( k1_gk1.x, k3_gk3.x * u, float2( k5_gk5.x, k6_gk6.x * u ) * w ), float4( blendDeriv.yyyy ) );
  result.w += dot( float4( k2_gk2.x, k4_gk4.x * u, float2( k5_gk5.x, k6_gk6.x * u ) * v ), float4( blendDeriv.zzzz ) );

  //    normalize and return
  //result *= 1.1547005383792515290182975610039;        //  (optionally) scale things to a strict -1.0->1.0 range    *= 1.0/sqrt(0.75)
  //return result;
  result *= 1.0/sqrt(0.75);
  return float4(result.x, result.yzw*s);//1.5*0.5
  //return float4(result.x * 0.5 + 0.5, result.yzw*s*0.5);//1.5*0.5
  //return float4(((result.x * 1.5) + 1.0) * 0.5, result.yzw*s*0.75);//1.5*0.5
}

float perlin(float3 P) {
  return perlin(P, 1, false);
}

uint3 hash_int3(uint3 x)
{
  x ^= x >> 16;
  x *= uint(0x7feb352d);
  x ^= x >> 15;
  x *= uint(0x846ca68b);
  x ^= x >> 16;
  return x;
}

float3 voronoi_hash( float3 x, float s) {
  x = x % s;
  x = float3( dot(x, float3(127.1,311.7, 74.7)),
              dot(x, float3(269.5,183.3,246.1)),
              dot(x, float3(113.5,271.9,124.6)));

  uint3 v = hash_int3(uint3(asuint(x.x), asuint(x.y), asuint(x.z)));
  return (v&0xFFFFFF)*(1./16777216);//24 bits, as amount of meanignful bits in mantissa
  //return frac(sin(x) * 43758.5453123);//this is unstable on different GPU
}

float3 voronoi( in float3 x, float s, bool inverted) {
  x *= s;
  x += 0.5;
  float3 p = floor(x);
  float3 f = frac(x);

  float id = 0.0;
  float2 res = float2( 1.0 , 1.0);
  for(int k=-1; k<=1; k++){
    for(int j=-1; j<=1; j++) {
      for(int i=-1; i<=1; i++) {
        float3 b = float3(i, j, k);
        float3 r = float3(b) - f + voronoi_hash(p + b, s);
        float d = dot(r, r);

        if(d < res.x) {
          id = dot(p + b, float3(1.0, 57.0, 113.0));
          res = float2(d, res.x);
        } else if(d < res.y) {
          res.y = d;
        }
      }
    }
  }

  float2 result = res;//sqrt(res);
  id = abs(id);

  if(inverted)
    return float3(1.0 - result, id);
  else
    return float3(result, id);
}

float get_worley_2_octaves(float3 p, float3 offset) {
  float3 xyz = p + offset;

  float worley_value1 = voronoi(xyz, 1.0, true).r;
  float worley_value2 = voronoi(xyz, 2.0, false).r;

  worley_value1 = saturate(worley_value1);
  worley_value2 = saturate(worley_value2);

  float worley_value = worley_value1;
  worley_value = worley_value - worley_value2 * 0.25;

  return worley_value;
}

float get_worley_3_octaves(float3 p, float s) {
  float3 xyz = p;

  float worley_value1 = voronoi(xyz, 1.0 * s, true).r;
  float worley_value2 = voronoi(xyz, 2.0 * s, false).r;
  float worley_value3 = voronoi(xyz, 4.0 * s, false).r;

  worley_value1 = saturate(worley_value1);
  worley_value2 = saturate(worley_value2);
  worley_value3 = saturate(worley_value3);

  float worley_value = worley_value1;
  worley_value = worley_value - worley_value2 * 0.3;
  worley_value = worley_value - worley_value3 * 0.3;

  return worley_value;
}

float normalize_5_octaves(float amplitude_factor = 0.5) {
  return 1./(1. + amplitude_factor*(1 + amplitude_factor*(1 + amplitude_factor*(1 + amplitude_factor))));
}
float get_perlin_5_octaves(float3 p, float s, bool3 tile, float frequency_factor = 2.0, float amplitude_factor = 0.5) {
  float3 xyz = p;

  float f = 1.0;
  float a = 1.0;
  float perlin_value = 0.0;
  perlin_value += a * perlin(xyz, s*f, tile).r; a *= amplitude_factor; f *= (frequency_factor + (tile.x ? 0 : 0.02));
  perlin_value += a * perlin(xyz, s*f, tile).r; a *= amplitude_factor; f *= (frequency_factor + (tile.x ? 0 : 0.03));
  perlin_value += a * perlin(xyz, s*f, tile).r; a *= amplitude_factor; f *= (frequency_factor + (tile.x ? 0 : 0.01));
  perlin_value += a * perlin(xyz, s*f, tile).r; a *= amplitude_factor; f *= (frequency_factor + (tile.x ? 0 : 0.01));
  perlin_value += a * perlin(xyz, s*f, tile).r;
  //perlin_value *= 1./(1 + 0.5 + 0.25 + 0.125 + 0.125*0.5);

  return perlin_value;
}

float get_perlin_3_octaves(float3 p, float s, bool3 tile, float frequency_factor = 2.0, float amplitude_factor = 0.5) {
  float3 xyz = p;

  float f = 1.0;
  float a = 1.0;
  float perlin_value = 0.0;
  perlin_value += a * perlin(xyz, s*f, tile).r; a *= amplitude_factor; f *= (frequency_factor + (tile.x ? 0 : 0.02));
  perlin_value += a * perlin(xyz, s*f, tile).r; a *= amplitude_factor; f *= (frequency_factor + (tile.x ? 0 : 0.03));
  perlin_value += a * perlin(xyz, s*f, tile).r;
  //perlin_value *= 1./(1 + 0.5 + 0.25 + 0.125 + 0.125*0.5);

  return perlin_value;
}

float4 get_perlin_5_octaves_deriv(float3 p, float s, bool3 tile) {
  float3 xyz = p;
  float amplitude_factor = 0.5;
  float frequency_factor = 2.0;

  float f = 1.0;
  float a = 1.0;
  float4 perlin_value = 0.0;
  perlin_value += a * perlin_deriv(xyz, s*f, tile); a *= amplitude_factor; f *= (frequency_factor + (tile.x ? 0 : 0.02));
  perlin_value += a * perlin_deriv(xyz, s*f, tile); a *= amplitude_factor; f *= (frequency_factor + (tile.x ? 0 : 0.03));
  perlin_value += a * perlin_deriv(xyz, s*f, tile); a *= amplitude_factor; f *= (frequency_factor + (tile.x ? 0 : 0.01));
  perlin_value += a * perlin_deriv(xyz, s*f, tile); a *= amplitude_factor; f *= (frequency_factor + (tile.x ? 0 : 0.01));
  perlin_value += a * perlin_deriv(xyz, s*f, tile);
  //perlin_value *= 1/(1 + 0.5 + 0.25 + 0.125 + 0.125*0.5);//so it is normalized value!

  return perlin_value;
}

float get_perlin_5_octaves(float3 p, bool3 tile) {return get_perlin_5_octaves(p, 1., tile);}

float get_perlin_7_octaves(float3 p, float s, float scale_freq = 2.0, float scale_amp = 0.5) {
  float3 xyz = p;
  float f = 1.0;
  float a = 1.0;

  float perlin_value = 0.0;
  perlin_value += a * perlin(xyz, s * f, true).r; a *= scale_amp; f *= scale_freq;
  perlin_value += a * perlin(xyz, s * f, true).r; a *= scale_amp; f *= scale_freq;
  perlin_value += a * perlin(xyz, s * f, true).r; a *= scale_amp; f *= scale_freq;
  perlin_value += a * perlin(xyz, s * f, true).r; a *= scale_amp; f *= scale_freq;
  perlin_value += a * perlin(xyz, s * f, true).r; a *= scale_amp; f *= scale_freq;
  perlin_value += a * perlin(xyz, s * f, true).r; a *= scale_amp; f *= scale_freq;
  perlin_value += a * perlin(xyz, s * f, true).r;
  //perlin_value *= 1/(1 + 0.5 + 0.25 + 0.125 + 0.125*0.5 + 0.125*0.25 + 0.125*0.125);

  return perlin_value;
}

//frostbite worley
/*
float fb_worley_hash(int n)
{
  return frac(sin(float(n) + 1.951) * 43758.5453123);
}

float fb_worley_noise(float3 x)
{
  float3 p = floor(x);
  float3 f = frac(x);

  f = f*f*(float3(3,3,3) - float3(2,2,2) * f);
  float n = p.x + p.y*57.0 + 113.0*p.z;
  return lerp(
    lerp(
        lerp(fb_worley_hash(int(n + 0.00)), fb_worley_hash(int(n + 1.00)), f.x),
        lerp(fb_worley_hash(int(n + 57.0)), fb_worley_hash(int(n + 58.0)), f.x),
        f.y),
    lerp(
        lerp(fb_worley_hash(int(n + 113.0)), fb_worley_hash(int(n + 114.0)), f.x),
        lerp(fb_worley_hash(int(n + 170.0)), fb_worley_hash(int(n + 171.0)), f.x),
        f.y),
    f.z);
}

float fb_worley_cells(float3 p, float cellCount)
{
  float3 pCell = p * cellCount;
  float d = 1.0e10;
  for (int xo = -1; xo <= 1; xo++)
  {
    for (int yo = -1; yo <= 1; yo++)
    {
        for (int zo = -1; zo <= 1; zo++)
        {
            float3 tp = floor(pCell) + float3(xo, yo, zo);

            tp = pCell - tp - fb_worley_noise(tp % cellCount);

            d = min(d, dot(tp, tp));
        }
    }
  }
  d = saturate(d);
  return d;
}*/

float3 curl_noise(float3 pos, float scale, bool3 tile = bool3(true,true, false)) {
  //#define CURL_DIGITAL 1
  #if CURL_DIGITAL
    float e = 0.5/128;//half texel
    float n1, n2;
    float3 derivs;

    n1 = get_perlin_5_octaves(pos.xyz + float3( 0, e, 0), scale, tile);
    n2 = get_perlin_5_octaves(pos.xyz + float3( 0,-e, 0), scale, tile);

    derivs.y = (n1-n2)/(2*e);

    n1 = get_perlin_5_octaves(pos.xyz + float3( 0, 0, e), scale, tile);
    n2 = get_perlin_5_octaves(pos.xyz + float3( 0, 0,-e), scale, tile);

    derivs.z = (n1-n2)/(2*e);

    n1 = get_perlin_5_octaves(pos.xyz + float3( e, 0, 0), scale, tile);
    n2 = get_perlin_5_octaves(pos.xyz + float3(-e, 0, 0), scale, tile);

    derivs.x = (n1-n2)/(2*e);

    derivs/=scale;
  #else//use anlalytical curl noise
    float3 derivs = get_perlin_5_octaves_deriv(pos.xyz, scale, tile).yzw/scale;
  #endif
  return derivs.yzx - derivs.zxy;
}
