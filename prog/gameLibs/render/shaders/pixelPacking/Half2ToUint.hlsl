#ifndef __HALF2_TO_UINT_INCLUDED__
#define __HALF2_TO_UINT_INCLUDED__

uint packHalf2ToUint(float fval1, float fval2) { return f32tof16(fval1) | (f32tof16(fval2) << 16); }
void unpackHalf2FromUint(uint ival, out float fval1, out float fval2)
{
  fval1 = f16tof32(ival);
  fval2 = f16tof32(ival >> 16);
}

uint2 packHalf2ToUint(float2 fval1, float2 fval2) { return f32tof16(fval1) | (f32tof16(fval2) << 16); }
void unpackHalf2FromUint(uint2 ival, out float2 fval1, out float2 fval2)
{
  fval1 = f16tof32(ival);
  fval2 = f16tof32(ival >> 16);
}

uint3 packHalf2ToUint(float3 fval1, float3 fval2) { return f32tof16(fval1) | (f32tof16(fval2) << 16); }
void unpackHalf2FromUint(uint3 ival, out float3 fval1, out float3 fval2)
{
  fval1 = f16tof32(ival);
  fval2 = f16tof32(ival >> 16);
}

uint4 packHalf2ToUint(float4 fval1, float4 fval2) { return f32tof16(fval1) | (f32tof16(fval2) << 16); }
void unpackHalf2FromUint(uint4 ival, out float4 fval1, out float4 fval2)
{
  fval1 = f16tof32(ival);
  fval2 = f16tof32(ival >> 16);
}

uint packHalf2ToUint(float2 fval) { return f32tof16(fval.x) | (f32tof16(fval.y) << 16); }
float2 unpackHalf2FromUint(uint ival) { return float2(f16tof32(ival), f16tof32(ival >> 16)); }

uint2 packHalf2ToUint(float4 fval) { return f32tof16(fval.xy) | (f32tof16(fval.zw) << 16); }
float4 unpackHalf2FromUint(uint2 ival) { return float4(f16tof32(ival), f16tof32(ival >> 16)); }

#endif //__HALF2_TO_UINT_INCLUDED__