#ifndef DAFX_PACKERS_HLSL
#define DAFX_PACKERS_HLSL

//
// Packers
//
DAFX_INLINE
uint pack_n1f_to_byte( float v )
{
  DAFX_TEST_NORM_FLOAT( v );
#ifdef DAFX_USE_VEC_MATH
  return v_extract_xi(
    v_andi(
      v_cvt_vec4i(
        v_mul( v_splats( v ), v_splats( 255.f ) ) ),
      v_splatsi( 0xff ) ) );
#else
  return min( uint(v * 255.f), 255U );
#endif
}

DAFX_INLINE
float unpack_byte_to_n1f( uint v )
{
  DAFX_TEST_BYTE( v );
#ifdef DAFX_USE_VEC_MATH
  return v_extract_x( v_mul( v_cvt_vec4f( v_splatsi( v ) ), v_splats( 1.f / 255.f ) ) );
#else
  return saturate( v * ( 1.f / 255.f ) );
#endif
}

DAFX_INLINE
uint pack_4b( uint x, uint y, uint z, uint w ) // normalized source!
{
  DAFX_TEST_BYTE( x );
  DAFX_TEST_BYTE( y );
  DAFX_TEST_BYTE( z );
  DAFX_TEST_BYTE( w );
  return x | ( y << 8 ) | z << 16 | w << 24;
}

DAFX_INLINE
uint pack_4b( uint4_cref v )
{
  return pack_4b( v.x, v.y, v.z, v.w );
}

DAFX_INLINE
uint4 unpack_4b( uint v )
{
  uint4 r;
  r.x = v & 0xff;
  r.y = ( v >> 8 ) & 0xff;
  r.z = ( v >> 16 ) & 0xff;
  r.w = ( v >> 24 ) & 0xff;
  return r;
}

DAFX_INLINE
uint pack_n3f_1b_to_uint( float3_cref v, uint w ) // normalized source!
{
  DAFX_TEST_NORM_FLOAT3(v);
#ifdef DAFX_USE_VEC_MATH
  vec4i vi = v_packus16(
   v_packs(
    v_cvt_vec4i( v_mul( v_make_vec4f(v.x,v.y,v.z,0), v_splats(255.f) ) ) ) );
  return ( v_extract_xi(vi) & 0xffffff ) | ( w << 24 );
#else
  return pack_4b( v.x * 255, v.y * 255, v.z * 255, w );
#endif
}

DAFX_INLINE
float3 unpack_uint_to_n3f_1b( uint v, uint1_ref w )
{
#ifdef DAFX_USE_VEC_MATH
  vec4i vi = v_cvt_byte_vec4i( v );
  w = v_extract_wi( vi );
  vec4f vf = v_mul(
    v_cvt_vec4f( vi ), v_splats( 1.f / 255.f ) );
  float3 f3;
  v_stu_p3( &f3.x, vf );
  return f3;
#else
  uint4 c = unpack_4b( v );
  float3 r;
  r.x = c.x * ( 1.f / 255.f );
  r.y = c.y * ( 1.f / 255.f );
  r.z = c.z * ( 1.f / 255.f );
  w = c.w;
  return r;
#endif
}

DAFX_INLINE
uint pack_n4f_to_uint( float4_cref v )
{
  DAFX_TEST_NORM_FLOAT4(v);
#ifdef DAFX_USE_VEC_MATH
  return v_extract_xi(
    v_packus16(
      v_packs(
        v_cvt_vec4i(
          v_mul( v_ldu( &v.x ), v_splats( 255.f ) ) ) ) ) );
#else
  return pack_4b( v.x * 255, v.y * 255, v.z * 255, v.w * 255 );
#endif
}

// DAFX_INLINE
// uint pack_n4f_to_e3dcolor( float4_cref v )
// {
//   return pack_n4f_to_uint( float4( v.z, v.y, v.x, v.w ) );

//   DAFX_TEST_NORM_FLOAT4(v);
// #ifdef DAFX_USE_VEC_MATH
//   vec4i vi = v_packus16( v_packs( v_cvt_vec4i( v_mul( v_make_vec4f(v.z,v.y,v.x,v.w), v_splats(255.f) ) ) ) );
//   return v_extract_xi(vi);
//   // return pack_4b( v.z * 255, v.y * 255, v.x * 255, v.w * 255 );
// }

DAFX_INLINE
float4 unpack_uint_to_n4f( uint v )
{
#ifdef DAFX_USE_VEC_MATH
  vec4f vf = v_mul(
    v_byte_to_float( v ), v_splats( 1.f / 255.f ) );
  float4 f4;
  v_stu( &f4.x, vf );
  return f4;
#else
  uint4 c = unpack_4b( v );
  float4 r;
  r.x = c.x * ( 1.f / 255.f );
  r.y = c.y * ( 1.f / 255.f );
  r.z = c.z * ( 1.f / 255.f );
  r.w = c.w * ( 1.f / 255.f );
  return r;
#endif
}

DAFX_INLINE
float4 unpack_e3dcolor_to_n4f( uint v )
{
  float4 f4;
#if defined(DAFX_USE_VEC_MATH) && _TARGET_SIMD_SSE >= 4 // Note: _mm_cvtepu8_epi32 is SSE4.1
  vec4f vn = v_mul(v_cvti_vec4f(_mm_cvtepu8_epi32(v_seti_x(v))), v_splats(1.f / 255.f));
  v_stu(&f4.x, _mm_shuffle_ps(vn, vn, _MM_SHUFFLE(3, 0, 1, 2))); // BGRA -> RGBA
  return f4;
#endif
  f4 = unpack_uint_to_n4f( v ); //-V779
  return float4( f4.z, f4.y, f4.x, f4.w );
}

DAFX_INLINE
uint pack_s3f_1b_to_uint( float3_cref v, uint w, float1_ref out_scale )
{
  DAFX_TEST_UNNORM_FLOAT3(v);
#ifdef DAFX_USE_VEC_MATH
  vec4f vf = v_make_vec4f( v.x, v.y, v.z, 0 );
  vec3f vl = v_length3( vf );
  out_scale = v_extract_x( vl );
  vf = v_andnot( v_is_unsafe_positive_divisor( vl ), v_div( vf, vl ) );
  vf = v_add(
    v_mul( vf, v_splats( 0.5f ) ),
    v_splats( 0.5 ) );

  vec4i vi = v_packus16(
    v_packs(
      v_cvt_vec4i( v_mul( vf, v_splats(255.f) ) ) ) );

  return ( v_extract_xi(vi) & 0xffffff ) | (w << 24 );
#else
  out_scale = length( v );
  float3 r;
  if (out_scale > 0.0) r = v / out_scale;
  else r = float3(0, 0, 0);

  r.x = r.x * 0.5 + 0.5;
  r.y = r.y * 0.5 + 0.5;
  r.z = r.z * 0.5 + 0.5;
  return pack_n3f_1b_to_uint( r, w );
#endif
}

DAFX_INLINE
float3 unpack_uint_to_s3f_1b( uint v, float scale, uint1_ref w )
{
#ifdef DAFX_USE_VEC_MATH
  vec4i vi = v_cvt_byte_vec4i( v );
  w = v_extract_wi( vi );
  vec4f vf = v_add(
    v_mul(
      v_cvt_vec4f( vi ), v_splats( 2.f / 255.f ) ),
    v_splats( -1.f ) );
  vf = v_mul( v_norm3_safe( vf, v_zero() ), v_splats( scale ) );
  float3 f3;
  v_stu_p3( &f3.x, vf );
  return f3;
#else
  float3 r = unpack_uint_to_n3f_1b( v, w );
  r.x = r.x * 2.f - 1.f;
  r.y = r.y * 2.f - 1.f;
  r.z = r.z * 2.f - 1.f;
  float l = length( r );
  r = l > 0 ? r / l : float3( 0, 0, 0 );
  return r * scale;
#endif
}

DAFX_INLINE
uint pack_n3s_to_uint( float3_cref v ) // normalized source!
{
  DAFX_TEST_UNNORM_FLOAT3(v);
  DAFX_TEST_NORM_FLOAT(fabsf(v.x));
  DAFX_TEST_NORM_FLOAT(fabsf(v.y));
  DAFX_TEST_NORM_FLOAT(fabsf(v.z));
  // sign goes to last byte
  uint bits = 0;
  if ( v.x < 0 )
    bits |= 1;

  if ( v.y < 0 )
    bits |= 2;

  if ( v.z < 0 )
    bits |= 4;

#ifdef DAFX_USE_VEC_MATH
  vec4i vi = v_packus16(
   v_packs(
    v_absi(
     v_cvt_vec4i( v_mul( v_make_vec4f(v.x,v.y,v.z,0), v_splats(255.f) ) ) ) ) );
  return ( v_extract_xi(vi) & 0xffffff ) | ( bits << 24 );
#else
  return pack_4b(
    fabsf( v.x ) * 255,
    fabsf( v.y ) * 255,
    fabsf( v.z ) * 255,
    bits );
#endif
}

DAFX_INLINE
float3 unpack_uint_to_n3s( uint v )
{
#ifdef DAFX_USE_VEC_MATH
  vec4i vi = v_cvt_byte_vec4i( v );
  uint w = v_extract_wi( vi );
  vec4f vf = v_mul( v_cvt_vec4f( vi ), v_splats( 1.f / 255.f ) );

  vf = v_xor( vf, v_and( v_make_vec4f_mask(w), V_CI_SIGN_MASK ) );
  float3 f3;
  v_stu_p3( &f3.x, vf );
  return f3;
#else
  uint4 b = unpack_4b( v );
  float3 r = float3(
    b.x * ( 1.f / 255.f ),
    b.y * ( 1.f / 255.f ),
    b.z * ( 1.f / 255.f ) );

  r.x *= ( b.w & 1 ) ? -1 : 1;
  r.y *= ( b.w & 2 ) ? -1 : 1;
  r.z *= ( b.w & 4 ) ? -1 : 1;

  return r;
#endif
}

#endif