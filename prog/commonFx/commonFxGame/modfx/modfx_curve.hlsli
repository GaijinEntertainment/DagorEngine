#ifndef DAFX_MODFX_CURVE_HLSL
#define DAFX_MODFX_CURVE_HLSL

#include "modfx/modfx_curve_inc.hlsli"


DAFX_INLINE
float modfx_get_1f_curve( BufferData_cref buf, uint ofs, float life_k )
{
  uint steps = dafx_load_1ui( buf, ofs );

  uint k0, k1;
  float w = modfx_calc_curve_weight( steps, life_k, k0, k1 );

  // 1byte components
  uint w0 = dafx_get_1ui( buf, ofs + k0 / 4 );
  uint w1 = dafx_get_1ui( buf, ofs + k1 / 4 );

  uint4 b0 = unpack_4b( w0 );
  uint4 b1 = unpack_4b( w1 );

  float v0 = unpack_byte_to_n1f( b0[k0 % 4] );
  float v1 = unpack_byte_to_n1f( b1[k1 % 4] );

  return lerp( v0, v1, w );
}

DAFX_INLINE
float4 modfx_get_e3dcolor_grad( BufferData_cref buf, uint ofs, float life_k )
{
  uint steps = dafx_load_1ui( buf, ofs );

  uint k0, k1;
  float w = modfx_calc_curve_weight( steps, life_k, k0, k1 );

  float4 v0 = unpack_e3dcolor_to_n4f( dafx_get_1ui( buf, ofs + k0 ) );
  float4 v1 = unpack_e3dcolor_to_n4f( dafx_get_1ui( buf, ofs + k1 ) );

  return lerp( v0, v1, w );
}

#endif