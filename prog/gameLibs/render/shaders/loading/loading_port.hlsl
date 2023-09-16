#define mat2 float2x2
#define mat3 float3x3
#define mat4 float4x4
#define ivec2 int2
#define ivec3 int3
#define ivec4 int4
#define vec2 float2
#define vec3 float3
#define vec4 float4
#define mix lerp
#define fract frac
float mod(float x, float y) { return x - y * floor(x/y); }
float2 mod(float2 x, float2 y) { return x - y * floor(x/y); }
float3 mod(float3 x, float3 y) { return x - y * floor(x/y); }
float4 mod(float4 x, float4 y) { return x - y * floor(x/y); }

float sampleNoise0(float2 tc){return tex2Dlod(noise_64_tex_l8, float4(tc,0,0)).x;}
float noise0( in vec2 x ){return sampleNoise0(x*0.01).x;}
float2 mul_m2(float2 p, mat2 m) {return mul(m, p);}
float2 mul_m2(mat2 m, float2 p) {return mul(p, m);}
float3 mul_m3(float3 p, mat3 m) {return mul(m, p);}
float3 mul_m3(mat3 m, float3 p) {return mul(p, m);}
#ifndef iMouse
  #define iMouse float2(0,0)
#endif