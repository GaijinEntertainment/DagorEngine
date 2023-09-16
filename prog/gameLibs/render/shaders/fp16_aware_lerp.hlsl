#ifndef FP16_AWARE_LERP_INCLUDED
#define FP16_AWARE_LERP_INCLUDED 1

//this lerp allows not to 'stuck' on two close value (closer than 1/threshold)
// that always happen when you use moving average, as history will get close to current color exponentially
// when values are closer than 1% difference(default scale) the weight increases
// default scale was found to be working in clouds

//that is not entirely correct.
// what we want is monotonic in some precision lerp
// there is actually math to do it, but that math is much heavier, as it requires a lot of conversion to fp16
// however, feel free to fix me

float4 fp16_aware_lerp4(float4 a, float4 b, float4 t, float scale = 1./0.01f)
{
  return lerp(a, b, saturate(t * rcp(min(1, 0.00001 + scale*abs(2.f*(a-b)*rcp(max(abs(a)+abs(b), 0.00001)))))));
}
float3 fp16_aware_lerp3(float3 a, float3 b, float3 t, float scale = 1./0.01f)
{
  return lerp(a, b, saturate(t * rcp(min(1, 0.00001 + scale*abs(2.f*(a-b)*rcp(max(abs(a)+abs(b), 0.00001)))))));
}
float2 fp16_aware_lerp2(float2 a, float2 b, float2 t, float scale = 1./0.01f)
{
  return lerp(a, b, saturate(t * rcp(min(1, 0.00001 + scale*abs(2.f*(a-b)*rcp(max(abs(a)+abs(b), 0.00001)))))));
}
float fp16_aware_lerp(float a, float b, float t, float scale = 1./0.01f)//this lerp allows not to 'stuck' on close value
{
  return lerp(a, b, saturate(t * rcp(min(1, 0.00001 + scale*abs(2.f*(a-b)*rcp(max(abs(a)+abs(b), 0.00001)))))));
}

float2 fp16_aware_lerp(float2 a, float2 b, float2 t, float scale = 1./0.01f)//this lerp allows not to 'stuck' on close value
{
  return fp16_aware_lerp2(a,b,t,scale);
}
float3 fp16_aware_lerp(float3 a, float3 b, float3 t, float scale = 1./0.01f)//this lerp allows not to 'stuck' on close value
{
  return fp16_aware_lerp3(a,b,t,scale);
}
float4 fp16_aware_lerp(float4 a, float4 b, float4 t, float scale = 1./0.01f)//this lerp allows not to 'stuck' on close value
{
  return fp16_aware_lerp4(a,b,t,scale);
}

// These functions return the finite one, if exactly one of them is finite, or call lerp otherwise
// This means that if both are NaN then it'll return NaN, but that's unlikely, and it will be filtered out next frame.
#define FILTERED_LERP_IMPL                    \
  bool aFinite = all(isfinite(a));            \
  bool bFinite = all(isfinite(b));            \
  return (aFinite == bFinite) ? fp16_aware_lerp(a, b, t, scale) : (aFinite ? a : b);

float fp16_aware_lerp_filtered(float a, float b, float t, float scale = 1./0.01f)
{
  FILTERED_LERP_IMPL
}
float2 fp16_aware_lerp_filtered(float2 a, float2 b, float2 t, float scale = 1./0.01f)
{
  FILTERED_LERP_IMPL
}
float3 fp16_aware_lerp_filtered(float3 a, float3 b, float3 t, float scale = 1./0.01f)
{
  FILTERED_LERP_IMPL
}
float4 fp16_aware_lerp_filtered(float4 a, float4 b, float4 t, float scale = 1./0.01f)
{
  FILTERED_LERP_IMPL
}
#undef FILTERED_LERP_IMPL

#endif