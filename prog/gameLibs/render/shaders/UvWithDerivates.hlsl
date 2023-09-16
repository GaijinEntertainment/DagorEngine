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
  result.uv_ddx = ddx(uv);
  result.uv_ddy = ddy(uv);
  return result;
}

void mul_derivates(inout UvWithDerivates uv, float multiplier)
{
  uv.uv_ddx *= multiplier;
  uv.uv_ddy *= multiplier;
}

#endif