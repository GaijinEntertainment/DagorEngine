#ifndef _UV_WITH_DERIVATES_
#define _UV_WITH_DERIVATES_ 1

struct UvWithDerivates
{
  float2 uv, uv_ddx, uv_ddy;
};

UvWithDerivates make_uv_with_derivates(float2 uv)
{
  UvWithDerivates result;
  result.uv = uv;
  #if NO_GRADIENTS_IN_SHADER
    result.uv_ddx = 0;
    result.uv_ddy = 0;
  #else
    result.uv_ddx = ddx(uv);
    result.uv_ddy = ddy(uv);
  #endif
  return result;
}

void mul_derivates(inout UvWithDerivates uv, float multiplier)
{
  uv.uv_ddx *= multiplier;
  uv.uv_ddy *= multiplier;
}

#endif