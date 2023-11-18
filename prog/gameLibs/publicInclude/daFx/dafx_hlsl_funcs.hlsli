#ifndef DAFX_HLSL_FUNCS_HLSL
#define DAFX_HLSL_FUNCS_HLSL

#ifdef DAFX_DEF_HLSL
  DAFX_INLINE const float2 float_xy( float3_cref a ) { return float2( a.x, a.y ); }
  DAFX_INLINE const float2 float_xy( float4_cref a ) { return float2( a.x, a.y ); }
  DAFX_INLINE const float2 float_xz( float4_cref a ) { return float2( a.x, a.z ); }
  DAFX_INLINE const float2 float_yw( float4_cref a ) { return float2( a.y, a.w ); }

  DAFX_INLINE const float3 float_xxx( float a ) { return float3( a, a, a ); }
  DAFX_INLINE const float3 float_xyz( float4_cref a ) { return float3( a.x, a.y, a.z ); }
  DAFX_INLINE const float3 float_zzz( float3_cref a ) { return float3( a.z, a.z, a.z ); }
  DAFX_INLINE const float3 float_zzz( float4_cref a ) { return float3( a.z, a.z, a.z ); }
  DAFX_INLINE const float3 float_yzw( float4_cref a ) { return float3( a.y, a.z, a.w ); }
  DAFX_INLINE const float3 float_zxy( float3_cref a ) { return float3( a.z, a.x, a.y ); }

  DAFX_INLINE const float4 float_xxxx( float a ) { return float4( a, a, a, a ); }
  DAFX_INLINE const float4 float_xyz0( float3_cref a ) { return float4( a.x, a.y, a.z, 0.f ); }
  DAFX_INLINE const float4 float_xyz1( float3_cref a ) { return float4( a.x, a.y, a.z, 1.f ); }
  DAFX_INLINE const float4 float_xyz1( float4_cref a ) { return float4( a.x, a.y, a.z, 1.f ); }

  DAFX_INLINE const float4 float_wzyx( float4_cref a ) { return float4( a.w, a.z, a.y, a.x ); }
  DAFX_INLINE const float4 float_zyxw( float4_cref a ) { return float4( a.z, a.y, a.x, a.w ); }
  DAFX_INLINE const float3 float_yzx( float3_cref a ) { return float3( a.y, a.z, a.x ); }

  DAFX_INLINE const float4 float_xyxy( float2_cref a ) { return float4( a.x, a.y, a.x, a.y ); }
  DAFX_INLINE const float4 float_xyxy( float4_cref a ) { return float4( a.x, a.y, a.x, a.y ); }
  DAFX_INLINE const float4 float_zwzw( float4_cref a ) { return float4( a.z, a.w, a.z, a.w ); }
#endif

#ifdef __cplusplus

  // note: always use function instead of defines to avoid mutiple argument calculation

  DAFX_INLINE float step( float y, float x) { return (x >= y) ? 1.f : 0.f; }
  DAFX_INLINE float3 step( const float3 &a, const float3 &b) { return float3(step(a.x, b.x), step(a.y, b.y), step(a.z, b.z)); }
  DAFX_INLINE float frac( float v ) { return v - floor(v); }
  DAFX_INLINE float3 frac( const float3 &v ) { return float3(frac(v.x), frac(v.y), frac(v.z)); }
  DAFX_INLINE float4 mul( const float4 &v, const float4x4 &m ) { return v*m; }
  DAFX_INLINE float3 mul_get_f3( const float4 &v, const float4x4 &m ) { float4 r = v*m; return float3( r.x, r.y, r.z ); }

  DAFX_INLINE float3 float3x3_row( const float3x3 &a, int r) { return a.getcol(r); }
  DAFX_INLINE float4 float4x4_row( const float4x4 &a, int r) { return a.getrow(r); }

  DAFX_INLINE float pow2( float v ) { return v * v; }
  DAFX_INLINE float2 pow2( float2 v ) { return v * v; }
  DAFX_INLINE float3 pow2( float3 v ) { return v * v; }
  DAFX_INLINE float4 pow2( float4 v ) { return v * v; }
  DAFX_INLINE float pow4( float v ) { v *= v; return v * v; }

  DAFX_INLINE int2 int_xy( const int3 & a) { return int2( a.x, a.y ); }

  DAFX_INLINE float4 lerp( const float4 &a, const float4 &b, const float4 &c)
  {
    return float4(
      lerp(a.x, b.x, c.x),
      lerp(a.y, b.y, c.y),
      lerp(a.z, b.z, c.z),
      lerp(a.w, b.w, c.w));
  }

  DAFX_INLINE float rsqrt_hlsl( float a ) {return 1.f/fastsqrt(a);}
  DAFX_INLINE float4 rsqrt_hlsl(const float4 &a)
  {
    return float4(rsqrt_hlsl(a.x),rsqrt_hlsl(a.y),rsqrt_hlsl(a.z),rsqrt_hlsl(a.w));
  }

  #define float4x4_to_3x3(from, to) {to.setcol(0, float_xyz((from).getcol(0))); to.setcol(1, float_xyz((from).getcol(1))); to.setcol(2, float_xyz((from).getcol(2)));}
  #define countbits __popcount
#else
  #define fabsf abs
  float rsqrt_hlsl(float a) {return rsqrt(a);}
  float4 rsqrt_hlsl(float4 a) {return rsqrt(a);}

  #define mul_get_f3(a,b) mul(a,b).xyz
  #define float3x3_row(a,r) a[r]
  #define float4x4_row(a,r) a[r]

  #define int_xy(a) a.xy

  #define float4x4_to_3x3(from, to) from = float3x3((to)[0].xyz, (to)[1].xyz, (to)[2].xyz)
#endif

#endif