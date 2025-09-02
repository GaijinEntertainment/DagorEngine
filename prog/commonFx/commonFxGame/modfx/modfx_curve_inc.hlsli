#ifndef DAFX_MODFX_CURVE_INC_HLSL
#define DAFX_MODFX_CURVE_INC_HLSL

#include "modfx/modfx_consts.hlsli"

DAFX_INLINE
float modfx_calc_curve_weight( uint steps, float life_k, uint_ref k0, uint_ref k1 )
{
  uint s = steps - 1;
  float k = s * life_k;

  k0 = (uint)k;
  k1 = min( k0 + 1, s );
  return k - (float)k0;
}

DAFX_INLINE
float4 modfx_get_e3dcolor_grad_raw( uint arr[MODFX_PREBAKE_GRAD_STEPS_LIMIT], uint steps, float life_k )
{
  uint k0, k1;
  float w = modfx_calc_curve_weight( steps, life_k, k0, k1 );

  float4 v0 = unpack_e3dcolor_to_n4f(arr[k0] );
  float4 v1 = unpack_e3dcolor_to_n4f(arr[k1]);

  return lerp( v0, v1, w );
}

#endif